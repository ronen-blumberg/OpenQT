#!/usr/bin/env python3
"""
OpenQT host helper daemon.

Bridges the DOS editor (running in DOSBox-X) to host-side language tools:
  * SPELL    - spell check (English via aspell, Hebrew via hspell/aspell-he)
  * TTS      - read-aloud via espeak-ng (en/he/ar/ru)
  * XLATE    - English -> Hebrew translation via deep-translator (online)
  * RECSTART/RECSTOP/RECCANCEL - dictation (speech-to-text, en/he) via arecord + faster-whisper
  * CLIP     - Smart Paste: host clipboard (UTF-8) -> editor codepage bytes
  * CLIPSET  - Copy to Host: editor codepage bytes -> host clipboard (UTF-8)
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
# Platform detection
# ---------------------------------------------------------------------------
# The bridge protocol and all codepage tables are OS-independent; only three
# things touch the host OS -- clipboard read, audio playback, and mic capture --
# and each picks an implementation from these flags. The daemon runs natively on
# Linux, Windows, and macOS (point OQT_BRIDGE at the same folder DOSBox-X mounts
# as C:\OPENQT\BRIDGE on whichever host).
IS_WINDOWS = sys.platform.startswith("win")
IS_MAC = sys.platform == "darwin"
IS_LINUX = not IS_WINDOWS and not IS_MAC

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


def _win_clipboard_get():
    """Read CF_UNICODETEXT from the Windows clipboard via ctypes -- no pip dep
    and proper Unicode (PowerShell Get-Clipboard mangles encoding)."""
    import ctypes
    from ctypes import wintypes
    CF_UNICODETEXT = 13
    u32 = ctypes.windll.user32
    k32 = ctypes.windll.kernel32
    u32.GetClipboardData.restype = wintypes.HANDLE
    k32.GlobalLock.restype = ctypes.c_void_p
    if not u32.OpenClipboard(0):
        raise RuntimeError("cannot open Windows clipboard")
    try:
        h = u32.GetClipboardData(CF_UNICODETEXT)
        if not h:
            return ""                       # clipboard empty or not text
        p = k32.GlobalLock(h)
        if not p:
            return ""
        try:
            return ctypes.c_wchar_p(p).value or ""
        finally:
            k32.GlobalUnlock(h)
    finally:
        u32.CloseClipboard()


def _win_clipboard_set(text):
    """Write CF_UNICODETEXT to the Windows clipboard via ctypes -- the write-side
    mirror of _win_clipboard_get(), no pip dep and proper Unicode."""
    import ctypes
    from ctypes import wintypes
    CF_UNICODETEXT = 13
    GMEM_MOVEABLE = 0x0002
    u32 = ctypes.windll.user32
    k32 = ctypes.windll.kernel32
    k32.GlobalAlloc.restype = wintypes.HGLOBAL
    k32.GlobalLock.restype = ctypes.c_void_p
    if not u32.OpenClipboard(0):
        raise RuntimeError("cannot open Windows clipboard")
    try:
        u32.EmptyClipboard()
        buf = ctypes.create_unicode_buffer(text)    # includes the NUL terminator
        size = ctypes.sizeof(buf)
        h = k32.GlobalAlloc(GMEM_MOVEABLE, size)
        if not h:
            raise RuntimeError("GlobalAlloc failed")
        p = k32.GlobalLock(h)
        if not p:
            raise RuntimeError("GlobalLock failed")
        ctypes.memmove(p, buf, size)
        k32.GlobalUnlock(h)
        if not u32.SetClipboardData(CF_UNICODETEXT, h):
            raise RuntimeError("SetClipboardData failed")
        # the clipboard now owns h -- must NOT free it
    finally:
        u32.CloseClipboard()


def _run_capture(cmd, env=None):
    """Run a clipboard-reader command and return its UTF-8 stdout."""
    try:
        r = subprocess.run(cmd, capture_output=True, timeout=5, env=env)
    except Exception as e:
        raise RuntimeError("clipboard read error: %s" % e)
    if r.returncode != 0:
        raise RuntimeError("clipboard read failed: %s"
                           % r.stderr.decode("utf-8", "replace")[:120])
    return r.stdout.decode("utf-8", "replace")


def _run_feed(cmd, text, env=None):
    """Run a clipboard-writer command, feeding 'text' as UTF-8 on stdin.
    stdout/stderr go to DEVNULL rather than pipes on purpose: tools like xclip
    and wl-copy fork a background process to *serve* the selection, and that
    child would hold a captured pipe open indefinitely -- hanging us until the
    timeout even though the write already succeeded."""
    try:
        r = subprocess.run(cmd, input=text.encode("utf-8"),
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=5, env=env)
    except Exception as e:
        raise RuntimeError("clipboard write error: %s" % e)
    if r.returncode != 0:
        raise RuntimeError("clipboard write failed (exit %d)" % r.returncode)


def read_host_clipboard():
    """Return the host clipboard as a Unicode string, or raise RuntimeError.

    Platform-native first (no install needed): ctypes on Windows, pbpaste on
    macOS, wl-paste/xclip/xsel on Linux. pyperclip is used as a fallback if it
    happens to be installed."""
    if IS_WINDOWS:
        return _win_clipboard_get()
    if IS_MAC:
        if _which("pbpaste"):
            return _run_capture(["pbpaste"])
        raise RuntimeError("pbpaste not found")
    # Linux / other X11 / Wayland
    if _which("wl-paste"):
        cmd = ["wl-paste", "-n"]
    elif _which("xclip"):
        cmd = ["xclip", "-selection", "clipboard", "-o"]
    elif _which("xsel"):
        cmd = ["xsel", "-b", "-o"]
    else:
        try:                                # last resort: optional pip package
            import pyperclip
            return pyperclip.paste() or ""
        except Exception:
            raise RuntimeError(
                "no clipboard tool (install xclip, xsel, or wl-clipboard)")
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    return _run_capture(cmd, env=env)


def write_host_clipboard(text):
    """Put a Unicode string on the host clipboard, or raise RuntimeError. The
    write-side mirror of read_host_clipboard(): ctypes on Windows, pbcopy on
    macOS, wl-copy/xclip/xsel on Linux (pyperclip as a last resort). The Linux
    tools ship in the same packages as their -paste counterparts (wl-clipboard,
    xclip, xsel)."""
    if IS_WINDOWS:
        _win_clipboard_set(text)
        return
    if IS_MAC:
        if _which("pbcopy"):
            _run_feed(["pbcopy"], text)
            return
        raise RuntimeError("pbcopy not found")
    # Linux / other X11 / Wayland
    if _which("wl-copy"):
        cmd = ["wl-copy"]
    elif _which("xclip"):
        cmd = ["xclip", "-selection", "clipboard", "-i"]
    elif _which("xsel"):
        cmd = ["xsel", "-b", "-i"]
    else:
        try:                                # last resort: optional pip package
            import pyperclip
            pyperclip.copy(text)
            return
        except Exception:
            raise RuntimeError(
                "no clipboard tool (install xclip, xsel, or wl-clipboard)")
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    _run_feed(cmd, text, env=env)


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
_whisper = None       # lazily loaded faster-whisper model


class Recorder:
    """Cross-platform 16 kHz / mono / 16-bit mic capture for dictation.

    Prefers the `sounddevice` pip package (PortAudio: Windows, macOS, Linux --
    no device-name guessing), recording into memory and writing a WAV with the
    stdlib `wave` module on stop. Falls back to a CLI recorder subprocess
    (arecord on Linux, sox `rec` on macOS/Linux) when sounddevice is absent."""

    SR = 16000

    def __init__(self):
        self.mode = None        # "sd" | "sub" | None
        self.path = None
        self._stream = None
        self._buf = None
        self._proc = None

    def active(self):
        if self.mode == "sd":
            return self._stream is not None
        if self.mode == "sub":
            return self._proc is not None and self._proc.poll() is None
        return False

    def start(self, path):
        self.cancel()                       # drop any prior capture
        self.path = path
        try:
            import sounddevice as sd         # PortAudio: truly cross-platform
        except Exception:
            sd = None
        if sd is not None:
            self._buf = []
            # RawInputStream gives raw int16 bytes -> no numpy needed.
            def _cb(indata, frames, time_info, status):
                self._buf.append(bytes(indata))
            self._stream = sd.RawInputStream(
                samplerate=self.SR, channels=1, dtype="int16", callback=_cb)
            self._stream.start()
            self.mode = "sd"
            return
        cmd = self._sub_cmd(path)
        if cmd is None:
            raise RuntimeError(
                "no mic capture: pip install sounddevice "
                "(or install arecord / sox)")
        self._proc = subprocess.Popen(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.mode = "sub"

    def _sub_cmd(self, path):
        if _which("arecord"):               # Linux/ALSA
            return ["arecord", "-q", "-f", "S16_LE", "-r", str(self.SR),
                    "-c", "1", "-t", "wav", path]
        if _which("rec"):                   # sox (macOS/Linux)
            return ["rec", "-q", "-r", str(self.SR), "-c", "1", "-b", "16",
                    path]
        if IS_MAC and _which("ffmpeg"):     # macOS AVFoundation default mic
            return ["ffmpeg", "-y", "-f", "avfoundation", "-i", ":default",
                    "-ar", str(self.SR), "-ac", "1", path]
        return None

    def stop(self):
        """Finalize the recording and return the WAV path (or None)."""
        if self.mode == "sd":
            try:
                self._stream.stop()
                self._stream.close()
            finally:
                self._stream = None
            import wave
            with wave.open(self.path, "wb") as w:
                w.setnchannels(1)
                w.setsampwidth(2)            # int16
                w.setframerate(self.SR)
                w.writeframes(b"".join(self._buf or []))
            self._buf = None
        elif self.mode == "sub":
            if self._proc and self._proc.poll() is None:
                self._proc.terminate()
                try:
                    self._proc.wait(timeout=3)
                except Exception:
                    pass
            self._proc = None
        path = self.path
        self.mode = None
        return path

    def cancel(self):
        if self.mode == "sd" and self._stream is not None:
            try:
                self._stream.stop()
                self._stream.close()
            except Exception:
                pass
            self._stream = None
            self._buf = None
        elif self.mode == "sub" and self._proc and self._proc.poll() is None:
            try:
                self._proc.terminate()
            except Exception:
                pass
            self._proc = None
        self.mode = None


_recorder = Recorder()


def _play_wav_async(wav):
    """Start playing a WAV file and return a killable Popen so Stop Speech can
    terminate it. Picks a player per OS: PowerShell SoundPlayer on Windows
    (always present, no install), afplay on macOS, and the first available of
    paplay/aplay/ffplay/play on Linux."""
    if IS_WINDOWS:
        # SoundPlayer.PlaySync blocks for the clip's duration; terminating the
        # PowerShell process stops playback immediately.
        ps = "(New-Object Media.SoundPlayer '%s').PlaySync()" % wav
        return subprocess.Popen(
            ["powershell", "-NoProfile", "-NonInteractive", "-Command", ps],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if IS_MAC and _which("afplay"):
        return subprocess.Popen(["afplay", wav],
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
    for p in ("paplay", "aplay", "ffplay", "play"):
        if _which(p):
            if p == "ffplay":
                args = ["ffplay", "-nodisp", "-autoexit", "-loglevel",
                        "quiet", wav]
            else:
                args = [p, wav]
            return subprocess.Popen(args, stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
    raise RuntimeError("no audio player (install pulseaudio-utils / alsa-utils)")


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
    if not _which("espeak-ng"):
        return "ERR", b"espeak-ng not installed"
    voice = ESPEAK_VOICE.get(lang, "en-us")
    wav = os.path.join(WORK, "tts.wav")
    r = subprocess.run(["espeak-ng", "-v", voice, "-w", wav, text],
                       capture_output=True)
    if r.returncode != 0 or not os.path.exists(wav):
        return "ERR", b"espeak-ng failed: " + r.stderr[:200]
    try:
        _playing = _play_wav_async(wav)
    except RuntimeError as e:
        return "ERR", str(e).encode("ascii", "replace")
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
    """Spell-check the document, picking the engine by the editor's current
    language (the F4 cycle): English -> aspell, Hebrew -> hspell. Arabic and
    Russian have no bundled engine, so they report a friendly message."""
    if lang == "HE":
        return _spell_hebrew(payload)
    if lang in ("AR", "RU"):
        return "ERR", (b"Spell check is only available for English and Hebrew "
                       b"(switch language with F4).")
    return _spell_english(payload)


def _parse_ispell(out_lines, skeletons):
    """Parse ispell '-a' pipe-mode output (aspell and hspell both speak it) into
    (line, col, word, suggs) tuples. `out_lines` and `skeletons` must be in the
    same string space (ASCII for aspell, Unicode Hebrew for hspell) so the
    column lookup matches. Returns words/suggestions as strings in that space."""
    findings = []
    line_idx = -1            # the engine emits a banner line first, then blocks
    saw_banner = False
    for out in out_lines:
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
    return findings


def _spell_serialize(findings):
    """findings: list of (line:int, col:int, word:bytes, suggs:[bytes]).
    Emit 'SPELL N\\n' then one 'line<TAB>col<TAB>word<TAB>s1,s2,...' record per
    finding as raw bytes -- numbers/separators are ASCII, but the word and
    suggestions stay in their native codepage bytes (CP862 Hebrew survives,
    where a plain .encode('ascii') would mangle it to '?')."""
    out = bytearray()
    out += ("SPELL %d\n" % len(findings)).encode("ascii")
    for i, (ln, col, word, suggs) in enumerate(findings):
        if i:
            out += b"\n"
        out += ("%d\t%d\t" % (ln, col)).encode("ascii")
        out += word + b"\t" + b",".join(suggs)
    return "OK", bytes(out)


def _spell_english(payload):
    """English spell check via aspell. Each byte maps to one ASCII skeleton
    column so aspell offsets index straight back into the document line."""
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
    findings = [(ln, col, w.encode("ascii", "replace"),
                 [s.encode("ascii", "replace") for s in sg])
                for (ln, col, w, sg) in _parse_ispell(proc.stdout.splitlines(),
                                                       skeletons)]
    return _spell_serialize(findings)


def _hebrew_skeleton(raw_line):
    """One Unicode char per source byte (Hebrew CP862 -> Unicode letter, ASCII
    kept, format/other bytes -> space) so hspell's character offsets map 1:1
    back to the document line's CP862 byte index. hspell silently ignores Latin
    runs, so embedded English in a Hebrew document is never flagged."""
    out = []
    for b in raw_line:
        if 0x20 <= b < 0x7F:
            out.append(chr(b))
        elif b in CP862:
            out.append(chr(CP862[b]))
        else:
            out.append(" ")
    return "".join(out)


def _aspell_has_hebrew():
    """True if aspell has its Hebrew ('he') dictionary installed."""
    try:
        out = subprocess.run(["aspell", "dicts"],
                             capture_output=True, text=True).stdout
    except Exception:
        return False
    return any(ln.strip() == "he" for ln in out.splitlines())


def _hebrew_engine():
    """Pick the Hebrew spell engine: prefer hspell (real morphology, Linux-only),
    fall back to aspell's 'he' dictionary (cross-platform -- works wherever aspell
    and the Hebrew word list install, including Windows/macOS). Both speak the
    ispell '-a' protocol in ISO-8859-8 and report 1-based character offsets, so the
    rest of the pipeline is identical. Returns the argv list, or None if neither."""
    if _which("hspell"):
        return ["hspell", "-a"]
    if _which("aspell") and _aspell_has_hebrew():
        return ["aspell", "-a", "--lang=he", "--encoding=iso-8859-8"]
    return None


def _spell_hebrew(payload):
    """Hebrew spell check. Both supported engines (hspell, aspell+he) use the
    ispell '-a' protocol in ISO-8859-8 -- logical order, the order OpenQT already
    stores -- so words/suggestions come back as CP862 bytes ready for the editor
    to render and insert directly."""
    engine = _hebrew_engine()
    if engine is None:
        hint = (b"Hebrew spell check needs the aspell Hebrew dictionary "
                b"(install aspell's 'he' dict), or a Linux host with hspell."
                if (IS_WINDOWS or IS_MAC) else
                b"Hebrew spell check not installed -- run: sudo apt install "
                b"hspell  (or: sudo apt install aspell-he)")
        return "ERR", hint
    raw_lines = payload.split(b"\n")
    skeletons = [_hebrew_skeleton(rl) for rl in raw_lines]
    feed = "".join("^" + s + "\n" for s in skeletons).encode("iso-8859-8", "replace")
    try:
        proc = subprocess.run(engine, input=feed, capture_output=True)
    except Exception as e:
        return "ERR", ("%s failed: %s" % (engine[0], e)).encode()
    out_lines = [ln.decode("iso-8859-8", "replace")
                 for ln in proc.stdout.split(b"\n")]
    findings = [(ln, col, hebrew_to_oqt(w), [hebrew_to_oqt(s) for s in sg])
                for (ln, col, w, sg) in _parse_ispell(out_lines, skeletons)]
    return _spell_serialize(findings)


def _locate(skeleton, word, reported):
    """Robustly map aspell's offset to a 0-based column, tolerating the
    1-based / ^-prefix ambiguity by verifying the word actually sits there."""
    for cand in (reported - 1, reported, reported - 2):
        if 0 <= cand and skeleton[cand:cand + len(word)] == word:
            return cand
    p = skeleton.find(word)
    return p if p >= 0 else max(0, reported - 1)


def handle_recstart(lang, payload):
    path = os.path.join(WORK, "dictation.wav")
    try:
        _recorder.start(path)
    except RuntimeError as e:
        return "ERR", str(e).encode("ascii", "replace")
    return "OK", b"recording"


def handle_reccancel(lang, payload):
    _recorder.cancel()
    return "OK", b"cancelled"


def handle_recstop(lang, payload):
    global _whisper
    if not _recorder.active():
        return "ERR", b"not recording"
    rec_path = _recorder.stop()
    if _whisper is None:
        try:
            from faster_whisper import WhisperModel
            _whisper = WhisperModel(WHISPER_MODEL, device="cpu",
                                    compute_type="int8")
        except Exception as e:
            return "ERR", ("faster-whisper not ready: %s" % e).encode()[:300]
    whisper_lang = "he" if lang == "HE" else "en"
    try:
        segments, _info = _whisper.transcribe(rec_path, language=whisper_lang)
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


def handle_clipset(lang, payload):
    """Copy to Host: convert the editor's raw codepage bytes (selection or whole
    document) to UTF-8 and put them on the host clipboard."""
    text = oqt_to_unicode(payload, lang)
    if not text:
        return "ERR", b"nothing to copy"
    try:
        write_host_clipboard(text)
    except RuntimeError as e:
        return "ERR", str(e).encode("utf-8")[:200]
    return "OK", ("%d chars copied" % len(text)).encode("ascii", "replace")


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
    "CLIPSET": handle_clipset,
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
    print("[oqt-helper] platform: %s" % sys.platform, flush=True)
    print("[oqt-helper] watching %s" % BRIDGE_DIR, flush=True)
    try:
        import sounddevice  # noqa: F401
        have_sd = True
    except Exception:
        have_sd = False
    print("[oqt-helper] tools: aspell=%s espeak-ng=%s mic=%s" % (
        _which("aspell"), _which("espeak-ng"),
        "sounddevice" if have_sd else (_which("arecord") or _which("rec"))),
        flush=True)

    def _bye(*_a):
        stop_playback()
        _recorder.cancel()
        print("\n[oqt-helper] bye", flush=True)
        sys.exit(0)
    signal.signal(signal.SIGINT, _bye)
    # SIGTERM exists on Windows but registering a handler can raise on some
    # builds; it's not essential there (Ctrl+C / window close suffice).
    try:
        signal.signal(signal.SIGTERM, _bye)
    except (ValueError, OSError, AttributeError):
        pass

    while True:
        if os.path.exists(REQ):
            process_request()
        time.sleep(POLL_SEC)


if __name__ == "__main__":
    main()
