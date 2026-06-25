# OpenQT host helper — speech, spell check, and translation

These editor features run **on the host** (Linux, Windows, or macOS), not inside
DOS, because DOS/DOSBox has no microphone input, no speech engine, and no network
TLS. The daemon is cross-platform — it picks a per-OS implementation for the only
three things that touch the host (clipboard read, audio playback, mic capture):

| Tools-menu item (Alt+T) | Host tool | Notes |
|---|---|---|
| **Spell Check (Eng)** | `aspell` | Interactive, English. Walks flagged words; pick a suggestion (1-9), S=skip, Esc=stop. |
| **Read Aloud** | `espeak-ng` + OS player | Speaks the selection (or whole document) on the host speakers. English/Hebrew/Arabic/Russian by current input language (F4). Playback: PowerShell (Windows) / afplay (macOS) / paplay·aplay·ffplay (Linux). |
| **Stop Speech** | — | Stops playback. |
| **Translate to Hebrew** | `deep-translator` (Google, online) | Translates selection/document English→Hebrew, inserts the Hebrew at the cursor. Needs internet. |
| **Dictate (Speech)** | `sounddevice` (or `arecord`/`rec`) + `faster-whisper` | Records from the host mic, transcribes, inserts at the cursor. |
| **Smart Paste / Paste Host** | clipboard reader | Reads the host clipboard as UTF-8 and converts to OpenQT codepage bytes. Windows: ctypes (built in); macOS: `pbpaste`; Linux: `wl-paste`/`xclip`/`xsel`. |

## How it works

`openqt.exe` and the daemon talk through files in `../BRIDGE/` (which is on the
DOSBox `C:` mount = `C:\OPENQT\BRIDGE`). The editor writes `REQ.TXT`; the daemon
processes it and overwrites `RESP.TXT` in place; the editor polls for it. **All
codepage↔UTF-8 conversion happens in the daemon** (using the exact tables from
`oqt2word.c`/`txt2oqt.c`), so the editor only ever handles its native
CP862/CP864/CP866 bytes.

## Setup (do this after cloning)

Neither the Python virtualenv nor the speech model is committed to the repo —
recreate both on the host:

```bash
# Linux / macOS
python3 -m venv host_helper/venv
host_helper/venv/bin/pip install -r host_helper/requirements.txt
```

```bat
rem Windows
python -m venv host_helper\venv
host_helper\venv\Scripts\pip install -r host_helper\requirements.txt
```

`aspell` and `espeak-ng` come from the system package manager, not pip
(see `requirements.txt` for per-OS install hints). Mic capture uses the
`sounddevice` pip package on every OS (with an `arecord`/`sox` fallback on
Linux); audio playback and clipboard read need nothing extra (built-in OS
facilities). Spell Check and Read Aloud degrade gracefully — if `aspell` or
`espeak-ng` isn't on PATH the menu item just reports "not installed" and the
rest of the editor is unaffected.

**Dictation model.** Dictate uses faster-whisper. It **must be a multilingual
model** — an English-only `.en` model cannot transcribe Hebrew at all. The
practical minimum for usable Hebrew on CPU is `medium` (~1.5 GB). To avoid a
first-run download stalling the editor's 300 s record timeout, pre-download it
into `host_helper/models/faster-whisper-medium/`; `run_helper.sh` auto-detects
that folder and loads from local disk (no Hugging Face Hub access at runtime):

```bash
mkdir -p host_helper/models/faster-whisper-medium
cd host_helper/models/faster-whisper-medium
base=https://huggingface.co/Systran/faster-whisper-medium/resolve/main
for f in model.bin config.json tokenizer.json vocabulary.txt; do
  curl -L -C - -o "$f" "$base/$f"
done
```

On Windows, do the same with PowerShell (`curl` there is an alias for
`Invoke-WebRequest`):

```powershell
mkdir host_helper\models\faster-whisper-medium
cd host_helper\models\faster-whisper-medium
$base = "https://huggingface.co/Systran/faster-whisper-medium/resolve/main"
foreach ($f in "model.bin","config.json","tokenizer.json","vocabulary.txt") {
  Invoke-WebRequest "$base/$f" -OutFile $f
}
```

Use `curl -C -` (resumable) rather than the Hub downloader, which rate-limits
and uses a non-resumable `.incomplete` temp name. There is no
`preprocessor_config.json` in that repo, so skip it. If the folder is absent,
the daemon falls back to downloading `medium` from the Hub on first use.
`run_helper.bat` auto-detects the same `models\faster-whisper-medium\` folder.

## Running

Start the daemon on the host **before** launching the editor, and leave it
running in its own terminal:

```bash
./host_helper/run_helper.sh        # Linux / macOS
```

```bat
host_helper\run_helper.bat         rem Windows
```

Then launch OpenQT as usual (e.g. `OQT 3`). The four features appear under the
**Tools** menu (Alt+T). If the daemon is not running, those items show
"Helper not responding"; everything else in the editor works normally.

Override the bridge folder or whisper model with env vars if needed:

```bash
OQT_BRIDGE=/some/path OQT_WHISPER_MODEL=large-v3 ./host_helper/run_helper.sh
```

(Keep `OQT_WHISPER_MODEL` a multilingual model — `small`/`medium`/`large-v3`,
not a `.en` variant — or Hebrew dictation breaks.)

## Diagnostics

`../bridge_test.c` is a standalone PING round-trip checker. Build it
(`wcl -bt=dos -ml -ox bridge_test.c -fe=BTEST16.EXE`) and run it inside DOSBox
with the daemon up; it writes PASS/FAIL to `BRIDGE\TESTOUT.TXT`.
