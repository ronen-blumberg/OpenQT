#!/usr/bin/env python3
"""
OpenQT host helper daemon.

Bridges the DOS editor (running in DOSBox-X) to host-side language tools:
  * SPELL    - English spell check via aspell
  * TTS      - read-aloud via espeak-ng (en/he/ar/ru)
  * XLATE    - English -> Hebrew translation via deep-translator (online)
  * RECSTART/RECSTOP/RECCANCEL - dictation (speech-to-text, en/he) via arecord + faster-whisper
  * PING     - round-trip self-test

It watches a shared folder (the DOSBox C: mount) for a request file written by
openqt.exe, runs the matching tool, and writes a response file the editor reads
back. ALL codepage<->UTF-8 conversion lives here, so openqt.c only ever deals
with its native mixed CP862/CP864/CP866 bytes.

Protocol (files in BRIDGE_DIR):
  REQ.TXT   (written by editor; editor writes REQ.TMP then renames)
      line 1: command keyword
      line 2: language hint (EN|HE|AR|RU)
      line 3: "---"
      rest  : raw OpenQT payload bytes
  RESP.TXT  (editor pre-creates with "WAIT\n"; we overwrite IN PLACE)
      line 1: "OK" or "ERR"
      rest  : raw OpenQT result bytes
      final : a line ".EOF." marking completion (editor polls for it)
"""

import os
import sys
import time
import signal
import subprocess
import tempfile
import unicodedata

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BRIDGE_DIR = os.environ.get(
    "OQT_BRIDGE", "/home/ronen/dos/eini2/OPENQT/BRIDGE")
REQ = os.path.join(BRIDGE_DIR, "REQ.TXT")
RESP = os.path.join(BRIDGE_DIR, "RESP.TXT")
WORK = os.path.join(BRIDGE_DIR, "_work")
POLL_SEC = 0.15
# Multilingual model (NOT a ".en" model) so dictation can do Hebrew as well as
# English. The editor's current input language (EN|HE) selects the Whisper
# decode language and the codepage the result is returned in. run_helper.sh
# overrides this with the locally pre-downloaded "medium" model dir when present
# (medium gives noticeably better Hebrew accuracy than small).
WHISPER_MODEL = os.environ.get("OQT_WHISPER_MODEL", "medium")

ESPEAK_VOICE = {"EN": "en-us", "HE": "he", "AR": "ar", "RU": "ru"}

# ---------------------------------------------------------------------------
# Codepage tables (ported verbatim from oqt2word.c / txt2oqt.c)
# ---------------------------------------------------------------------------
# CP862 Hebrew 0x80..0x9A -> Unicode
CP862 = {0x80 + i: 0x05D0 + i for i in range(27)}

# CP866 high half 0x80..0xFF -> Unicode (Russian session)
CP866 = [
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
    0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x2116, 0x00A4, 0x25A0, 0x00A0,
]

LAMALEF_PAIR = 0xFFFE   # 0xF9/0xFA -> U+0644 U+0622
LAMALEF_PLAIN = 0xFFFD  # 0xFE      -> U+0644 U+0627
# CP864 Arabic 0xA0..0xFF -> Unicode (0 = skip)
CP864 = [
    0x00A0, 0x00AD, 0x0622, 0x00A3, 0x00A4, 0x0623, 0, 0,
    0x0627, 0x0628, 0x062A, 0x062B, 0x060C, 0x062C, 0x062D, 0x062E,
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667,
    0x0668, 0x0669, 0x0641, 0x061B, 0x0633, 0x0634, 0x0635, 0x061F,
    0x00A2, 0x0621, 0x0622, 0x0623, 0x0624, 0x0639, 0x0626, 0x0627,
    0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637,
    0x0638, 0x0639, 0x063A, 0x00A6, 0x00AC, 0x00F7, 0x00D7, 0x0639,
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647,
    0x0648, 0x0649, 0x064A, 0x0636, 0x0639, 0x063A, 0x063A, 0x0645,
    0x0651, 0x0651, 0x0646, 0x0647, 0x0647, 0x0649, 0x064A, 0x063A,
    0x0642, LAMALEF_PAIR, LAMALEF_PAIR, 0x0644, 0x0643, 0x064A,
    LAMALEF_PLAIN, 0,
]

# Format toggle bytes used inline by OpenQT - stripped before sending to tools
FMT_BYTES = {0x02, 0x03, 0x04, 0xAE, 0xAF}

# Reverse map for translation result: Unicode Hebrew -> CP862 byte
UNI_TO_CP862 = {0x05D0 + i: 0x80 + i for i in range(27)}

# Reverse maps for Smart Paste (Unicode -> OpenQT byte) built from the tables
# above. setdefault keeps the first (canonical) byte when a codepoint repeats.
UNI_TO_CP864 = {}
for _i, _cp in enumerate(CP864):
    if _cp and _cp not in (LAMALEF_PAIR, LAMALEF_PLAIN):
        UNI_TO_CP864.setdefault(_cp, 0xA0 + _i)
UNI_TO_CP866 = {}
for _i, _cp in enumerate(CP866):
    UNI_TO_CP866.setdefault(_cp, 0x80 + _i)

# Punctuation that has no codepage equivalent -> nearest ASCII (None = drop).
# Smart quotes/dashes, NBSP, and Hebrew punctuation marks are the common
# offenders in copied web text.
PUNCT_MAP = {
    0x2018: 0x27, 0x2019: 0x27, 0x201A: 0x27, 0x201B: 0x27,   # ' variants
    0x2032: 0x27, 0x05F3: 0x27,                               # prime, geresh
    0x201C: 0x22, 0x201D: 0x22, 0x201E: 0x22, 0x201F: 0x22,   # " variants
    0x2033: 0x22, 0x05F4: 0x22, 0x00AB: 0x22, 0x00BB: 0x22,   # dprime, gershayim, guillemets
    0x2013: 0x2D, 0x2014: 0x2D, 0x2015: 0x2D, 0x2212: 0x2D,   # dashes / minus
    0x05BE: 0x2D, 0x00AD: 0x2D,                               # maqaf, soft hyphen
    0x00A0: 0x20, 0x2007: 0x20, 0x202F: 0x20,                 # NBSP variants
    0x2026: None,                                             # ellipsis (expanded below)
    0x200E: None, 0x200F: None, 0x200B: None,                 # LRM / RLM / ZWSP
    0x200C: None, 0x200D: None, 0xFEFF: None,                 # ZWNJ / ZWJ / BOM
}


def unicode_to_oqt(text, lang):
    """Convert an arbitrary Unicode string (e.g. the host clipboard) to raw
    OpenQT bytes for the editor's current language. Niqqud / Arabic harakat and
    any other combining marks are stripped (CP862/864 have no glyphs for them),
    smart punctuation is folded to ASCII, and Hebrew/Arabic/Russian letters map
    to their codepage bytes. Unmappable codepoints are dropped. This is exactly
    what makes pasted vocalized Hebrew come out clean."""
    text = unicodedata.normalize("NFKD", text)
    out = bytearray()
    for ch in text:
        cp = ord(ch)
        if cp == 0x0A:
            out.append(0x0A); continue
        if cp == 0x0D:                      # CR -> drop (LF carries the break)
            continue
        if cp == 0x09:
            out.append(0x09); continue
        if cp == 0x2026:                    # ellipsis -> ...
            out += b"..."; continue
        if cp in PUNCT_MAP:
            m = PUNCT_MAP[cp]
            if m is not None:
                out.append(m)
            continue
        if unicodedata.combining(ch):       # niqqud, harakat, accents -> drop
            continue
        if cp < 0x20:
            continue
        if cp < 0x80:                       # ASCII passes through verbatim
            out.append(cp); continue
        if lang == "RU":
            if cp in UNI_TO_CP866:
                out.append(UNI_TO_CP866[cp])
        else:                               # HE / AR / EN (and trilingual)
            if cp in UNI_TO_CP862:
                out.append(UNI_TO_CP862[cp])
            elif cp in UNI_TO_CP864:
                out.append(UNI_TO_CP864[cp])
        # anything else: silently dropped
    return bytes(out)


def read_host_clipboard():
    """Return the host clipboard as a Unicode string, or raise RuntimeError."""
    if _which("wl-paste"):
        cmd = ["wl-paste", "-n"]
    elif _which("xclip"):
        cmd = ["xclip", "-selection", "clipboard", "-o"]
    elif _which("xsel"):
        cmd = ["xsel", "-b", "-o"]
    else:
        raise RuntimeError("no clipboard tool (install xclip, xsel, or wl-clipboard)")
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    try:
        r = subprocess.run(cmd, capture_output=True, timeout=5, env=env)
    except Exception as e:
        raise RuntimeError("clipboard read error: %s" % e)
    if r.returncode != 0:
        raise RuntimeError("clipboard read failed: %s"
                           % r.stderr.decode("utf-8", "replace")[:120])
    return r.stdout.decode("utf-8", "replace")


def oqt_to_unicode(data, lang):
    """Convert raw OpenQT bytes to a Unicode string for a host tool."""
    out = []
    for b in data:
        if b in (0x0A, 0x09):          # keep LF / TAB
            out.append(chr(b))
            continue
        if b < 0x20 or b in FMT_BYTES:  # drop control / format toggles
            continue
        if b < 0x80:                    # ASCII
            out.append(chr(b))
            continue
        if lang == "RU":
            out.append(chr(CP866[b - 0x80]))
        elif b <= 0x9A:                 # Hebrew (CP862)
            out.append(chr(CP862[b]))
        else:                           # Arabic (CP864)
            cp = CP864[b - 0xA0]
            if cp == LAMALEF_PAIR:
                out.append("لآ")
            elif cp == LAMALEF_PLAIN:
                out.append("لا")
            elif cp:
                out.append(chr(cp))
    return "".join(out)


def hebrew_to_oqt(text):
    """Convert a Hebrew/ASCII Unicode string to raw OpenQT (CP862) bytes."""
    out = bytearray()
    for ch in text:
        cp = ord(ch)
        if cp == 0x0A:
            out.append(0x0A)
        elif cp < 0x80:
            out.append(cp)
        elif cp in UNI_TO_CP862:
            out.append(UNI_TO_CP862[cp])
        # other codepoints (vowel points, etc.) are dropped
    return bytes(out)


def ascii_skeleton_line(raw_line):
    """One ASCII byte per source byte (high/format bytes -> space) so aspell
    column offsets map 1:1 back to the document line's byte index."""
    out = []
    for b in raw_line:
        out.append(chr(b) if 0x20 <= b < 0x7F else " ")
    return "".join(out)


# ---------------------------------------------------------------------------
# Feature handlers  ->  return (status_str, payload_bytes)
# ---------------------------------------------------------------------------
_playing = None       # current TTS playback subprocess
_recording = None     # current arecord subprocess
_rec_path = None
_whisper = None       # lazily loaded faster-whisper model


def stop_playback():
    global _playing
    if _playing and _playing.poll() is None:
        try:
            _playing.terminate()
        except Exception:
            pass
    _playing = None


def handle_ping(lang, payload):
    return "OK", b"PONG: " + payload


def handle_tts(lang, payload):
    global _playing
    stop_playback()
    text = oqt_to_unicode(payload, lang).strip()
    if not text:
        return "ERR", b"nothing to read"
    voice = ESPEAK_VOICE.get(lang, "en-us")
    wav = os.path.join(WORK, "tts.wav")
    r = subprocess.run(["espeak-ng", "-v", voice, "-w", wav, text],
                       capture_output=True)
    if r.returncode != 0 or not os.path.exists(wav):
        return "ERR", b"espeak-ng failed: " + r.stderr[:200]
    player = "paplay" if _which("paplay") else "aplay"
    _playing = subprocess.Popen([player, wav],
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
    return "OK", b"speaking"


def handle_ttsstop(lang, payload):
    stop_playback()
    return "OK", b"stopped"


def handle_xlate(lang, payload):
    text = oqt_to_unicode(payload, "EN").strip()
    if not text:
        return "ERR", b"nothing to translate"
    try:
        from deep_translator import GoogleTranslator
    except Exception as e:
        return "ERR", ("deep-translator not installed: %s" % e).encode()
    try:
        # translate line by line to preserve paragraph breaks
        out_lines = []
        for line in text.split("\n"):
            if line.strip():
                out_lines.append(
                    GoogleTranslator(source="en", target="iw").translate(line))
            else:
                out_lines.append("")
        heb = "\n".join(out_lines)
    except Exception as e:
        return "ERR", ("translate failed: %s" % e).encode()[:300]
    return "OK", hebrew_to_oqt(heb)


def handle_spell(lang, payload):
    """Run aspell over each document line; report misspellings with a 0-based
    column offset into the original line and comma-joined suggestions."""
    if not _which("aspell"):
        return "ERR", b"aspell not installed"
    raw_lines = payload.split(b"\n")
    skeletons = [ascii_skeleton_line(rl) for rl in raw_lines]
    # Feed all lines to one aspell -a process; "^" forces check mode per line.
    feed = "".join("^" + s + "\n" for s in skeletons)
    try:
        proc = subprocess.run(
            ["aspell", "-a", "--lang=en", "--encoding=utf-8"],
            input=feed, capture_output=True, text=True)
    except Exception as e:
        return "ERR", ("aspell failed: %s" % e).encode()
    findings = []
    line_idx = -1            # aspell emits a banner line first, then blocks
    saw_banner = False
    for out in proc.stdout.splitlines():
        if not saw_banner:
            saw_banner = True       # first line is the @(#) version banner
            continue
        if out == "":
            line_idx += 1           # blank line terminates one input line
            continue
        if out and out[0] in "&#":
            parts = out.split()
            word = parts[1]
            if out[0] == "&":       # "& word N off: s1, s2, ..."
                rep_off = int(parts[3].rstrip(":"))
                rest = out.split(":", 1)[1].strip() if ":" in out else ""
                suggs = [s.strip() for s in rest.split(",") if s.strip()][:9]
            else:                   # "# word off"  (no suggestions)
                rep_off = int(parts[2])
                suggs = []
            col = _locate(skeletons[line_idx + 1] if line_idx + 1 < len(skeletons) else "",
                          word, rep_off)
            findings.append((line_idx + 1, col, word, suggs))
    # serialize: line<TAB>col<TAB>word<TAB>s1,s2,...
    lines = ["%d\t%d\t%s\t%s" % (ln, col, w, ",".join(s))
             for (ln, col, w, s) in findings]
    body = ("SPELL %d\n" % len(findings)) + "\n".join(lines)
    return "OK", body.encode("ascii", "replace")


def _locate(skeleton, word, reported):
    """Robustly map aspell's offset to a 0-based column, tolerating the
    1-based / ^-prefix ambiguity by verifying the word actually sits there."""
    for cand in (reported - 1, reported, reported - 2):
        if 0 <= cand and skeleton[cand:cand + len(word)] == word:
            return cand
    p = skeleton.find(word)
    return p if p >= 0 else max(0, reported - 1)


def handle_recstart(lang, payload):
    global _recording, _rec_path
    if not _which("arecord"):
        return "ERR", b"arecord not installed"
    if _recording and _recording.poll() is None:
        _recording.terminate()
    _rec_path = os.path.join(WORK, "dictation.wav")
    _recording = subprocess.Popen(
        ["arecord", "-q", "-f", "S16_LE", "-r", "16000", "-c", "1",
         "-t", "wav", _rec_path],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return "OK", b"recording"


def handle_reccancel(lang, payload):
    global _recording
    if _recording and _recording.poll() is None:
        _recording.terminate()
    _recording = None
    return "OK", b"cancelled"


def handle_recstop(lang, payload):
    global _recording, _whisper
    if not (_recording and _rec_path):
        return "ERR", b"not recording"
    _recording.terminate()
    try:
        _recording.wait(timeout=3)
    except Exception:
        pass
    _recording = None
    if _whisper is None:
        try:
            from faster_whisper import WhisperModel
            _whisper = WhisperModel(WHISPER_MODEL, device="cpu",
                                    compute_type="int8")
        except Exception as e:
            return "ERR", ("faster-whisper not ready: %s" % e).encode()[:300]
    whisper_lang = "he" if lang == "HE" else "en"
    try:
        segments, _info = _whisper.transcribe(_rec_path, language=whisper_lang)
        text = "".join(seg.text for seg in segments).strip()
    except Exception as e:
        return "ERR", ("transcription failed: %s" % e).encode()[:300]
    if lang == "HE":
        # Hebrew text -> CP862 bytes (same path the translate feature uses)
        return "OK", hebrew_to_oqt(text)
    # English text -> plain ASCII bytes for direct insertion
    return "OK", text.encode("ascii", "replace")


def handle_clip(lang, payload):
    """Smart Paste: read the host clipboard, convert UTF-8 (incl. vocalized
    Hebrew) to clean OpenQT codepage bytes for the editor's current language."""
    try:
        text = read_host_clipboard()
    except RuntimeError as e:
        return "ERR", str(e).encode("utf-8")[:200]
    data = unicode_to_oqt(text, lang)
    if not data:
        return "ERR", b"clipboard has no pasteable text"
    return "OK", data


HANDLERS = {
    "PING": handle_ping,
    "TTS": handle_tts,
    "TTSSTOP": handle_ttsstop,
    "XLATE": handle_xlate,
    "SPELL": handle_spell,
    "RECSTART": handle_recstart,
    "RECSTOP": handle_recstop,
    "RECCANCEL": handle_reccancel,
    "CLIP": handle_clip,
}


# ---------------------------------------------------------------------------
# Bridge plumbing
# ---------------------------------------------------------------------------
def _which(prog):
    from shutil import which
    return which(prog) is not None


def write_resp(status, payload):
    """Overwrite RESP.TXT IN PLACE (same inode) so the DOSBox directory cache
    keeps showing the file the editor pre-created. Single write, then close."""
    data = status.encode("ascii") + b"\n" + payload + b"\n.EOF.\n"
    with open(RESP, "wb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())


def process_request():
    try:
        with open(REQ, "rb") as f:
            raw = f.read()
    except FileNotFoundError:
        return
    # split header (cmd / lang / "---") from payload
    try:
        head, payload = raw.split(b"\n---\n", 1)
    except ValueError:
        # request not fully written yet; leave it and retry next tick
        return
    hlines = head.split(b"\n")
    cmd = hlines[0].decode("ascii", "replace").strip().upper()
    lang = (hlines[1].decode("ascii", "replace").strip().upper()
            if len(hlines) > 1 else "EN")
    try:
        os.remove(REQ)
    except OSError:
        pass
    handler = HANDLERS.get(cmd)
    if not handler:
        write_resp("ERR", ("unknown command: %s" % cmd).encode())
        return
    print("[oqt-helper] %s (lang=%s, %d bytes)" % (cmd, lang, len(payload)),
          flush=True)
    try:
        status, out = handler(lang, payload)
    except Exception as e:
        status, out = "ERR", ("handler crashed: %s" % e).encode()[:300]
    write_resp(status, out)
    print("[oqt-helper]   -> %s (%d bytes)" % (status, len(out)), flush=True)


def main():
    os.makedirs(BRIDGE_DIR, exist_ok=True)
    os.makedirs(WORK, exist_ok=True)
    print("[oqt-helper] watching %s" % BRIDGE_DIR, flush=True)
    print("[oqt-helper] tools: aspell=%s espeak-ng=%s arecord=%s" % (
        _which("aspell"), _which("espeak-ng"), _which("arecord")), flush=True)

    def _bye(*_a):
        stop_playback()
        if _recording and _recording.poll() is None:
            _recording.terminate()
        print("\n[oqt-helper] bye", flush=True)
        sys.exit(0)
    signal.signal(signal.SIGINT, _bye)
    signal.signal(signal.SIGTERM, _bye)

    while True:
        if os.path.exists(REQ):
            process_request()
        time.sleep(POLL_SEC)


if __name__ == "__main__":
    main()
