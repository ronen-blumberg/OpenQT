# OpenQT host helper — speech, spell check, and translation

These four editor features run **on the Linux host**, not inside DOS, because
DOS/DOSBox has no microphone input, no speech engine, and no network TLS:

| Tools-menu item (Alt+T) | Host tool | Notes |
|---|---|---|
| **Spell Check (Eng)** | `aspell` | Interactive, English. Walks flagged words; pick a suggestion (1-9), S=skip, Esc=stop. |
| **Read Aloud** | `espeak-ng` | Speaks the selection (or whole document) on the host speakers. English/Hebrew/Arabic/Russian by current input language (F4). |
| **Stop Speech** | — | Stops playback. |
| **Translate to Hebrew** | `deep-translator` (Google, online) | Translates selection/document English→Hebrew, inserts the Hebrew at the cursor. Needs internet. |
| **Dictate (Speech)** | `arecord` + `faster-whisper` | Records from the host mic, transcribes English, inserts at the cursor. |

## How it works

`openqt.exe` and the daemon talk through files in `../BRIDGE/` (which is on the
DOSBox `C:` mount = `C:\OPENQT\BRIDGE`). The editor writes `REQ.TXT`; the daemon
processes it and overwrites `RESP.TXT` in place; the editor polls for it. **All
codepage↔UTF-8 conversion happens in the daemon** (using the exact tables from
`oqt2word.c`/`txt2oqt.c`), so the editor only ever handles its native
CP862/CP864/CP866 bytes.

## Setup (already done once)

```bash
python3 -m venv host_helper/venv
host_helper/venv/bin/pip install -r host_helper/requirements.txt
```

`aspell`, `espeak-ng`, and `arecord` come from the system. The whisper model
(`base.en`, ~145 MB) downloads on first dictation and is then cached.

## Running

Start the daemon on the host **before** launching the editor, and leave it
running in its own terminal:

```bash
./host_helper/run_helper.sh
```

Then launch OpenQT as usual (e.g. `OQT 3`). The four features appear under the
**Tools** menu (Alt+T). If the daemon is not running, those items show
"Helper not responding"; everything else in the editor works normally.

Override the bridge folder or whisper model with env vars if needed:

```bash
OQT_BRIDGE=/some/path OQT_WHISPER_MODEL=small.en ./host_helper/run_helper.sh
```

## Diagnostics

`../bridge_test.c` is a standalone PING round-trip checker. Build it
(`wcl -bt=dos -ml -ox bridge_test.c -fe=BTEST16.EXE`) and run it inside DOSBox
with the daemon up; it writes PASS/FAIL to `BRIDGE\TESTOUT.TXT`.
