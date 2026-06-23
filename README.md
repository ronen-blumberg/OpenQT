# OpenQT

A Hebrew / Arabic / **Russian** / English word processor for DOS, plus
the converter utilities `oqt2word` (export to UTF-8) and `txt2oqt`
(import from UTF-8). The editor itself is a QText 5.5 clone with full
BiDi (right-to-left) support, inline formatting, undo/redo, password
encryption, Arabic positional shaping on the DOS screen, and **true
trilingual editing** — Hebrew, Arabic, and English can all coexist on
the same line of the same document, simultaneously visible on the VGA
screen. Russian (CP866) is supported as its own session — see
[Russian mode](#russian-mode) for why it doesn't combine with the
trilingual screen font.

> **Author:** Ronen Blumberg  
> **License:** Public Domain  
> **Current version:** OpenQT 3.6.0  
> **Platform:** Real-mode 16-bit DOS, or 32-bit DOS via DOS4GW (bundled).

---

## Table of contents

- [What's in this repository](#whats-in-this-repository)
- [Quick start](#quick-start)
- [Building from source](#building-from-source)
- [OpenQT — the editor](#openqt--the-editor)
  - [Launchers](#launchers)
  - [Trilingual mode (Hebrew + Arabic + English)](#trilingual-mode-hebrew--arabic--english)
  - [Russian mode](#russian-mode)
  - [Host-assisted features (speech, spell check, translation)](#host-assisted-features-speech-spell-check-translation)
  - [Keyboard reference](#keyboard-reference)
  - [Docs viewer (Help → Docs…)](#docs-viewer-help--docs)
  - [Hebrew typing layout](#hebrew-typing-layout)
  - [Arabic typing layout](#arabic-typing-layout)
  - [Russian typing layout](#russian-typing-layout)
  - [BiDi and visual order](#bidi-and-visual-order)
  - [Arabic positional shaping](#arabic-positional-shaping)
  - [File format](#file-format)
  - [Encryption](#encryption)
- [oqt2word — export to UTF-8](#oqt2word--export-to-utf-8)
- [txt2oqt — import from UTF-8](#txt2oqt--import-from-utf-8)
- [txt2rtf — export to RTF (Hebrew + English)](#txt2rtf--export-to-rtf-hebrew--english)
- [Round-trip workflow](#round-trip-workflow)
- [Other tools](#other-tools)
- [Version history](#version-history)
- [Troubleshooting](#troubleshooting)

---

## What's in this repository

| File | Purpose |
|---|---|
| `openqt.c` / `openqt.exe` | The editor itself. ~3000 lines, single translation unit. |
| `oqt2word.c` / `oqt2word.exe` | Convert OpenQT files to UTF-8 (for Word / LibreOffice). |
| `txt2oqt.c` / `txt2oqt.exe` | Convert UTF-8 (or legacy DOS) text into OpenQT format. |
| `txt2rtf.c` / `txt2rtf.exe` | Convert OpenQT (Hebrew + English) to RTF, preserving bold / underline / bold+underline. |
| `oqt2qt.c` / `oqt2qt.exe` | Convert OpenQT to QText 5.5 file format. |
| `qt2oqt.c` / `qt2oqt.exe` | Convert QText 5.5 to OpenQT (visual → logical reorder). |
| `arabvga.c` / `ARABVGA.EXE` | Uploads CP864 Arabic font into VGA character generator. Supports `/P` for partial overlay (used by trilingual mode). |
| `rusvga.c` / `RUSVGA.EXE` | Uploads CP866 Russian font into VGA character generator. New in 3.3. |
| `make_rusvga.py` | Helper script that regenerates `rusvga.c` (combines HEBVGA's CP437 base with Cyrillic glyphs from the Linux X11 8x13 font). |
| `OPENQT.HLP`, `OPENQTH.HLP`, `OPENQTA.HLP`, `OPENQTR.HLP` | Per-language user guides loaded by the in-editor Docs viewer (Alt+H → Docs). Each one is plain text in OpenQT's mixed-codepage byte format, matched to a single VGA font. The viewer auto-picks the file that matches the active input language so every byte renders correctly. |
| `make_hlp.py` | Helper script that regenerates the four `*.HLP` files from a single UTF-8 source (Unicode → OpenQT byte mapping). |
| `host_helper/` | Linux-host daemon that powers the Tools-menu speech / spell-check / translation features. New in 3.4. See [Host-assisted features](#host-assisted-features-speech-spell-check-translation). |
| `HEBVGA.COM` | Uploads CP862 Hebrew font into VGA character generator. |
| `OQT.BAT`, `OQTH.BAT`, `OQTA.BAT`, `OQTR.BAT` | Launchers (see below). |
| `DOS4GW.EXE` | DOS extender for 32-bit builds. |
| `_arabic_font/` | Reference NLS files (no longer needed at runtime). |

---

## Quick start

The compiled editor and tools (`openqt.exe`, the converters, `HEBVGA.COM`,
`ARABVGA.EXE`, `DOS4GW.EXE`) ship in the repo, so you can run it without
building. Mount the cloned folder as `C:\OPENQT` in DOSBox (or DOSBox-X):

```
mount c /path/to/this/repo
c:
cd \OPENQT
```

Then, in that DOS environment:

```
OQT 3 mydocument.txt  # Hebrew + Arabic + English (trilingual)
OQTA mydocument.txt   # Arabic + English
OQTH mydocument.txt   # Hebrew + English
OQTR mydocument.txt   # Russian + English (CP866)
```

Inside the editor:

- **F4** — cycle input language: English → Hebrew → Arabic → Russian → English …
- **F5** — toggle right-to-left layout
- **F2** — save
- **F3** — open another file
- **F1** — quick help screen
- **Alt+H → Docs...** — open the full trilingual user guide (`OPENQT.HLP`)
- **Alt+X** — exit

To bring text out for editing in LibreOffice / Word:

```
oqt2word mydocument.txt mydocument-utf8.txt
```

To bring text back from LibreOffice / Word (UTF-8):

```
txt2oqt mydocument-utf8.txt mydocument.txt
```

---

## Building from source

All programs are C, single-file translation units, designed for the
**Open Watcom** compiler (`wcl` / `wcl386`). The build is performed in
DOS (or under DOSBox); the source tree itself can live on Linux but the
compiled binaries only run under DOS.

### Editor (preferred 32-bit DOS4GW build)

```
wcl386 -bt=dos -l=dos4g openqt.c -fe=openqt.exe
```

### Editor (16-bit real-mode build)

```
wmake               # default target — 16-bit openqt.exe
wmake openqt32.exe  # 32-bit DOS4GW build
wmake both          # build both
wmake clean
```

### Converters

```
wcl386 -bt=dos -l=dos4g oqt2word.c  -fe=oqt2word.exe
wcl386 -bt=dos -l=dos4g txt2oqt.c   -fe=txt2oqt.exe
wcl386 -bt=dos -l=dos4g txt2rtf.c   -fe=txt2rtf.exe
wcl386 -bt=dos -l=dos4g oqt2qt.c    -fe=oqt2qt.exe
wcl386 -bt=dos -l=dos4g qt2oqt.c    -fe=qt2oqt.exe
```

### Arabic font uploader (16-bit, must be real-mode)

```
wcl -bt=dos -ml -ox arabvga.c -fe=arabvga.exe
```

The build embeds the 4 KB CP864 font into the binary, so no external
CPI file is needed at runtime. The resulting `ARABVGA.EXE` is ~24 KB.

**Important:** If a stale `ARABVGA.COM` exists from earlier builds,
delete it before testing. DOS resolves `ARABVGA` (no extension) as
`.COM` → `.EXE` → `.BAT`, so a leftover `.COM` will shadow the new
`.EXE` and ignore the new `/P` flag.

`ARABVGA` flags:

| Flag | Meaning |
|---|---|
| (none) | Full 256-character upload (Arabic-only mode). |
| `/P` | Partial upload — only 0xA0–0xFE. Use after HEBVGA in trilingual mode. |
| `/F=path /CP=N` | Load codepage N from a CPI file instead of the embedded font. |
| `/?` | Show help. |

### Russian font uploader (16-bit, must be real-mode)

```
wcl -bt=dos -ml -ox rusvga.c -fe=rusvga.exe
```

`rusvga.c` is **generated** by `make_rusvga.py` (Python on the host) —
the script extracts HEBVGA.COM's CP437 base font and overlays Cyrillic
+ CP866-specific glyphs from the Linux X11 fixed-misc 8x13 raster font
(from `xfonts-base`), padded into the 8x16 VGA cell. Re-run
`make_rusvga.py` if you tweak the font; do not hand-edit the embedded
`fontbuf[]` array in `rusvga.c`. The resulting `RUSVGA.EXE` is ~24 KB.

`RUSVGA` flags:

| Flag | Meaning |
|---|---|
| (none) | Full 256-character upload (Russian-only mode). |
| `/F=path /CP=N` | Load codepage N from a CPI file instead of the embedded font. |
| `/?` | Show help. |

There is **no `/P` partial-upload mode** because Russian (CP866) doesn't
combine with Hebrew (CP862) or Arabic (CP864) — see
[Russian mode](#russian-mode).

There are no tests and no CI — this is a personal project.

---

## OpenQT — the editor

OpenQT is a full-screen modal text editor styled after QText 5.5. It
draws directly to VGA text-mode memory at 0xB8000 and uses BIOS
interrupts for input, so it has zero dependencies beyond standard DOS.
The whole runtime fits in roughly 75 KB.

Four input languages share the editor: English, Hebrew, Arabic, and
Russian. English, Hebrew, and Arabic coexist in the same document on
the same line — BiDi reordering takes care of display order while the
file stores the text in its natural typing (logical) order. Russian
runs as its own session (English + Russian only) because CP866 byte
ranges overlap CP862 Hebrew and CP864 Arabic; see
[Russian mode](#russian-mode).

Key features:

- Full BiDi: Hebrew and Arabic right-to-left, English and digits
  left-to-right, all interleaved.
- Inline **bold** and **underline** formatting.
- Up to **30 000 lines** per document.
- Undo and redo, 250 levels deep.
- Search, search-and-replace, with all three scripts.
- Block (column / range) operations: copy, cut, paste, delete.
- Word wrap at column 71.
- Save reminder every 10 minutes (optional).
- Page counter (assumes 60 lines per page; up to 500 pages).
- **Password protection** — files saved encrypted, header `OQT-ENC1`.
- **CP864 Arabic positional shaping** on the DOS screen (since v3.1+).

### Launchers

The launchers handle VGA character-generator setup. The Hebrew CP862
font and the Arabic CP864 font live in **disjoint byte ranges**
(0x80–0x9A vs. 0xA0–0xFE), so they can both be loaded simultaneously to
get a single font containing all three scripts — see the trilingual
section below for the technique.

| Launcher | What it does |
|---|---|
| `OQT 3 file.txt` | **Trilingual** — Hebrew + Arabic + English on one screen. Loads HEBVGA (full CP862 font), then `ARABVGA /P` (overlays just 0xA0–0xFE with Arabic letters), then OpenQT with `/A`. |
| `OQTH file.txt` | Hebrew + English only. Runs `HEBVGA`, then OpenQT. |
| `OQTA file.txt` | Arabic + English only. Runs `ARABVGA`, then OpenQT with `/A`. |
| `OQTR file.txt` | Russian + English only (CP866). Runs `RUSVGA`, then OpenQT with `/R`. |
| `OQT H file.txt` | Dispatcher form — same as `OQTH`. |
| `OQT A file.txt` | Dispatcher form — same as `OQTA`. |
| `OQT R file.txt` | Dispatcher form — same as `OQTR`. |
| `OQT file.txt` | Default — Hebrew + English (legacy behavior). |

The `/A` flag tells OpenQT that CP864 Arabic glyphs are loaded into the
0xC0–0xDF range where CP437 puts box-drawing characters. The editor
responds by drawing dialog/menu frames with ASCII characters (`+`, `-`,
`|`) instead of CP437 box characters, so dialogs render correctly even
though their byte slots have been replaced with Arabic letters.

### Trilingual mode (Hebrew + Arabic + English)

OpenQT supports editing Hebrew, Arabic, and English **all at once** on
the same screen, in the same document, on the same line. This is what
`OQT 3` enables.

#### How it works

The Hebrew CP862 font puts letters at bytes **0x80–0x9A** (27 slots:
22 letters + 5 sofit forms). The Arabic CP864 font puts letters at
bytes **0xA0–0xFE** (95 slots). These ranges don't overlap. So if we
arrange for the VGA character generator to have CP862 Hebrew at
0x80–0x9A *and* CP864 Arabic at 0xA0–0xFE *at the same time*, the
editor can write either set of bytes and they all render correctly.

`HEBVGA.COM` performs a full 256-character upload — it lays down a
complete CP862 base font (Hebrew letters in their range, CP437 ASCII
and box-drawing in the rest). `ARABVGA.EXE` originally also did a full
256-character upload, which would trample HEBVGA's Hebrew if run
afterwards. Version 3.2 added a `/P` (partial) flag that uploads
**only** the 95 Arabic slots at 0xA0–0xFE. The trilingual launcher
chains them in this order:

```
HEBVGA          ; full upload — CP862 Hebrew + CP437 base
ARABVGA /P      ; partial upload — overlays Arabic at 0xA0-0xFE only
OPENQT /A ...   ; ASCII boxes (CP864 letters now at 0xC0-0xDF)
```

The result: a single VGA font with **all three scripts visible**.
Pressing **F4** inside the editor cycles `LANG_ENG` → `LANG_HEB` →
`LANG_ARA`, and the bytes you type land in the right ranges.

#### What you can do with it

Mix all three scripts freely on one line. The BiDi engine handles
direction switching automatically — Hebrew and Arabic flow
right-to-left within their runs, English and digits flow left-to-right
within theirs, and the runs are placed correctly relative to each
other in the paragraph's overall direction (set by F5 or by the
dominant script on the line).

For example, after `OQT 3`:

```
F4 → English   type:   See the word
F4 → Hebrew    type:   שלום
F4 → Arabic    type:   مرحبا
F4 → English   type:   in three scripts.
```

…produces a single line with all three scripts visible, each in their
correct direction, with the Arabic shaped cursively (v3.2 shaping
applies).

#### Trilingual file format

When you save a trilingual document, the file is a **mixed CP862 +
CP864 byte stream**:

| Byte range | Meaning |
|---|---|
| 0x00–0x7F | ASCII (English, digits, punctuation) |
| 0x80–0x9A | Hebrew letters (CP862) |
| 0xA0–0xFE | Arabic letters (CP864 abstract isolated form) |
| 0x02 / 0x03 / 0x04 | Inline format toggles (bold / underline / both) |
| 0xAF / 0xAE | QText 5.5 format markers (legacy, recognized on read) |

This is **not a standard codepage** — it's an OpenQT-specific dual
encoding. Other DOS programs reading the file with only CP862 loaded
will see Hebrew correctly but Arabic as box-drawing/symbol gibberish,
and vice versa with CP864. To share trilingual files outside OpenQT,
convert to UTF-8 with `oqt2word`.

The two converters (`oqt2word` and `txt2oqt`) **handle trilingual files
without any extra flag**. Each byte is classified by range:

- 0x00–0x7F → ASCII passthrough
- 0x80–0x9A → CP862 Hebrew → U+05D0–05EA
- 0xA0–0xFE → CP864 Arabic → U+0621–064A

So a trilingual `.txt` round-trips cleanly through UTF-8.

#### Caveats

- All caveats from Arabic-only mode apply (CP864 glyphs at 0xC0–0xDF
  clash with CP437 box-drawing — handled by `/A` flag and ASCII
  dialogs).
- Cursor positioning, BiDi reordering, search, replace, block ops,
  format toggles, and undo/redo all work transparently across all
  three scripts.
- The Israeli Hebrew keyboard layout and the Microsoft Arabic 101
  layout overlap on every key. F4 chooses which mapping is active for
  the next keystrokes.

### Russian mode

OpenQT 3.3 added a **Russian session** — English + Russian only,
cannot mix with Hebrew or Arabic in the same screen or document.

#### Why it's a separate session

The trilingual trick works because CP862 Hebrew (letters at 0x80–0x9A)
and CP864 Arabic (letters at 0xA0–0xFE) live in **disjoint byte
ranges**. CP866 Russian doesn't: Cyrillic uppercase sits at 0x80–0x9F
(overlapping Hebrew), Cyrillic lowercase at 0xA0–0xAF + 0xE0–0xEF
(overlapping Arabic). The same 8-bit slot would have to hold a Hebrew
letter, an Arabic letter, *and* a Cyrillic letter at the same time —
impossible without switching the document buffer to UTF-8 (which would
be a much larger rewrite of OpenQT's BiDi, shaping, and storage paths).

So Russian is its own thing. CP437 box-drawing at 0xB0–0xDF is intact
in CP866 (CP866 is just CP437 with Cyrillic substituted at 0x80–0xAF
and 0xE0–0xFE), so `OPENQT /R` does **not** need the `/A` ASCII-boxes
fallback that Arabic mode does.

#### Launching

```
OQTR mydoc.txt
```

does the equivalent of:

```
RUSVGA          ; full upload — CP437 base + Cyrillic at 0x80-0xAF, 0xE0-0xFE
OPENQT /R ...   ; sets input_lang=LANG_RUS and rtl_mode=0 at startup
```

Inside the editor, **F4** cycles through Eng → Heb → Ara → Rus. The
direction (`rtl_mode`) auto-flips with the language: RTL for Hebrew /
Arabic, LTR for English / Russian. F5 still overrides manually.

#### What works in Russian mode

- All 33 Russian letters (uppercase + lowercase) plus Ё/ё via the
  standard JCUKEN keyboard layout.
- ASCII English passthrough — type English freely, no F10
  embedded-LTR dance needed.
- Bold, underline, undo/redo, search/replace, block ops, encryption —
  all behave identically to English mode.
- Box-drawing dialogs render correctly (CP866 keeps CP437 at
  0xB0–0xDF) — no `/A` ASCII fallback needed.

#### What doesn't

- A document opened in Russian mode and re-opened in Hebrew/Arabic
  mode renders letter bytes as the wrong script. The 8-bit storage
  doesn't carry script tags.
- BiDi reordering is bypassed for Russian text (Russian is LTR), so
  inserting Russian into a Hebrew/Arabic document via byte-paste won't
  round-trip — the converter `/R` flag is the right route for taking
  Russian text out to UTF-8.

### Host-assisted features (speech, spell check, translation, paste)

New in 3.4 (Paste Host added in 3.5). Items in the **Tools** menu (**Alt+T**)
hand work off to a small helper program running on the Linux host, because DOS
itself has no microphone input, no speech engine, no modern network access, and
no UTF-8 clipboard:

| Tools item | What it does | Engine (host) |
|---|---|---|
| **Spell Check (Eng)** | Walks the English words in the selection (or whole document), highlights each unknown word, and offers numbered suggestions — press **1–9** to replace, **S** to skip, **Esc** to stop. | `aspell` |
| **Read Aloud** | Speaks the selection (or whole document) through the host's speakers. The voice follows the current input language (F4): English, Hebrew, Arabic, or Russian. | `espeak-ng` |
| **Stop Speech** | Stops playback. | — |
| **Translate to Hebrew** | Translates English text (selection or document) and inserts the Hebrew at the cursor. **Needs an internet connection.** | online MT |
| **Dictate (Speech)** | Records from the host microphone (Enter to transcribe, Esc to cancel) and inserts the recognised English text at the cursor. | `whisper` |
| **Paste Host** (**Alt+V**) | Inserts the host clipboard at the cursor as clean text for the current F4 language — strips niqqud / Arabic harakat, folds smart quotes and dashes to ASCII. Use this instead of DOSBox-X's Ctrl+F6 for Hebrew/Arabic/Russian, which can't represent vowel points and re-maps pasted punctuation. | `xclip` |

**First-time setup (after cloning):** the Python virtualenv and the ~1.5 GB
speech model are not in the repo — create the venv from
`host_helper/requirements.txt` and pre-download the multilingual `medium`
Whisper model. See [`host_helper/README.md`](host_helper/README.md) for the
exact commands. (Skip this if you only want the editor — the helper is optional.)

**To use them, start the helper on the host first** and leave it running in its
own terminal, then launch OpenQT as usual:

```bash
./host_helper/run_helper.sh
```

The editor and the helper exchange files in `C:\OPENQT\BRIDGE`; all the
encoding conversion happens on the host, so your documents keep OpenQT's normal
byte format. If the helper isn't running, those four menu items simply report
"Helper not responding" and the rest of the editor is unaffected. Full setup
and details are in [`host_helper/README.md`](host_helper/README.md).

### Keyboard reference

#### Files and editing

| Key | Action |
|---|---|
| **F1** | Help screen |
| **F2** | Save current file |
| **F3** | Open another file (file dialog) |
| **Ctrl+S** | Save (no dialog) |
| **Ctrl+N** | New document |
| **Ctrl+Z** | Undo |
| **Ctrl+Y** | Redo |
| **Insert** | Toggle insert / overwrite |
| **Delete** | Delete character under cursor |
| **Backspace** | Delete character to the left |

#### Cursor movement

| Key | Action |
|---|---|
| Arrow keys | Move 1 character / 1 line |
| **Ctrl+←** / **Ctrl+→** | Move 5 characters at a time |
| **Home** / **End** | Start / end of line |
| **PgUp** / **PgDn** | Page up / down |
| **Ctrl+Home** / **Ctrl+End** | Start / end of document |
| **Ctrl+G** | Go to line number |

#### Language and direction

| Key | Action |
|---|---|
| **F4** | Cycle input language: English → Hebrew → Arabic → Russian → … (auto-flips RTL/LTR to match) |
| **F5** | Toggle RTL layout (right-to-left paragraph direction) |
| **F10** | Toggle "embedded LTR" — type English/digits inline within an RTL paragraph without leaving the current input language |

#### Search and replace

| Key | Action |
|---|---|
| **F7** | Find |
| **F8** | Replace |
| **Ctrl+L** | (some builds) — depends on version |

#### Formatting

| Key | Action |
|---|---|
| **Ctrl+B** | Toggle bold for next typed text |
| **Ctrl+U** | Toggle underline |
| **Alt+L** | Toggle underline (alternate) |
| **Alt+U** | Toggle bold + underline together |

#### Block operations

| Key | Action |
|---|---|
| **F9** | Paste block at cursor |
| **Alt+V** | Paste host clipboard (via helper; strips niqqud, see Tools) |
| **Alt+B** | Open block menu (start, end, copy, cut, paste, delete) |

#### Menus

| Key | Menu |
|---|---|
| **Alt+F** | File |
| **Alt+E** | Edit |
| **Alt+S** | Search |
| **Alt+B** | Block |
| **Alt+O** | Options |
| **Alt+T** | Tools |
| **Alt+H** | Help (submenu: Help / Docs… / About…) |
| **Alt+X** | Exit |

### Docs viewer (Help → Docs…)

Open with **Alt+H → Docs…**. The viewer loads one of four `.HLP`
files from disk, chosen by the current input language (so the loaded
VGA font matches the file's content), and shows it in a paged
read-only dialog over the editor. Your current document is untouched
— ESC closes the viewer and returns to exactly where you were.

| Active input language | File loaded |
|---|---|
| Hebrew (LANG_HEB) | `OPENQTH.HLP` (Hebrew + English) |
| Arabic (LANG_ARA) | `OPENQTA.HLP` (Arabic + English) |
| Russian (LANG_RUS) | `OPENQTR.HLP` (Russian + English) |
| English / default (LANG_ENG) | `OPENQT.HLP` (Heb + Ara + English — trilingual, intended for OQT 3 sessions) |

If the language-specific file is missing, the viewer falls back to
`OPENQT.HLP` (and then to `..\OPENQT.HLP`, `\OPENQT\`, `C:\OPENQT\`).

Each `.HLP` is **plain text in OpenQT's mixed-codepage byte encoding**
— the same on-disk format as documents in that script. The split is
necessary because Russian (CP866) byte ranges overlap Hebrew (CP862)
and Arabic (CP864): a unified quad-script file would render gibberish
in whichever sections didn't match the loaded font. Each per-language
file is guaranteed to use only bytes the active session can render.

The viewer:

- Runs each line through the editor's `bidi_reorder()` and Arabic
  positional-shaping pipeline, so Hebrew goes right-to-left, Arabic
  goes right-to-left and shapes cursively (v3.2 rules), and English /
  digits stay left-to-right within their runs.
- Forces RTL paragraph direction internally (so mixed-script docs
  always read correctly) and restores the original `rtl_mode` on exit.
- Pages with **Up / Down**, **PgUp / PgDn**, **Home / End**.
- Closes with **ESC** or **Q**.

#### File search order

`show_docs()` looks for `OPENQT.HLP` in this order:

1. Current working directory
2. `..\OPENQT.HLP` (parent directory — handles being launched from
   `ARABIC\` or any subfolder)
3. `\OPENQT\OPENQT.HLP` on the current drive
4. `C:\OPENQT\OPENQT.HLP` (absolute fallback)

If none are found, you get a "OPENQT.HLP not found" dialog with a
hint to run `rescan` in DOSBox (which refreshes the drive cache when
you've added the file from the host side).

#### Editing the docs

The Python helper `make_hlp.py` regenerates **all four** `.HLP` files
from one source. The script holds four UTF-8 string blocks
(`HEADER`, `HEBREW_SECTION`, `ARABIC_SECTION`, `RUSSIAN_SECTION`) and
assembles them into the right combinations:

| File | Sections |
|---|---|
| `OPENQT.HLP` | header + Hebrew + Arabic |
| `OPENQTH.HLP` | header + Hebrew |
| `OPENQTA.HLP` | header + Arabic |
| `OPENQTR.HLP` | header + Russian |

Edit the relevant block (each language can be typed naturally as
UTF-8), then:

```
python3 make_hlp.py
```

…rewrites all four files with the correct per-codepage byte mapping.
No recompile of `openqt.exe` is needed — the viewer reads the
appropriate file at runtime each time you open it.

### Hebrew typing layout

CP862 Hebrew letters are at 0x80–0x9A. The keyboard layout is the
standard Israeli PC layout:

| Key | Letter | | Key | Letter | | Key | Letter |
|---|---|---|---|---|---|---|---|
| `t` | א Alef | | `c` | ב Bet | | `d` | ג Gimel |
| `s` | ד Dalet | | `v` | ה He | | `u` | ו Vav |
| `z` | ז Zayin | | `j` | ח Het | | `y` | ט Tet |
| `h` | י Yod | | `l` | ך / כ Kaf (sofit at end of word) | | `f` | ל Lamed |
| `k` | ם / מ Mem | | `n` | ן / נ Nun | | `b` | ס Samekh |
| `g` | ע Ayin | | `;` | ף / פ Pe | | `m` | ץ / צ Tsadi |
| `e` | ק Qof | | `r` | ר Resh | | `a` | ש Shin |
| `,` | ת Tav | | | | | | |

Final-letter substitution (sofit) is automatic at end-of-word.

### Arabic typing layout

CP864 Arabic letters are scattered across 0xA1–0xFD, with the standard
Microsoft Arabic 101 keyboard layout. Letters:

| Key | Letter | Key | Letter | Key | Letter |
|---|---|---|---|---|---|
| `a` | ش Sheen | `b` | لآ Lam-Alef-Madda | `c` | ؤ Waw-Hamza |
| `d` | ي Yeh | `e` | ث Theh | `f` | ب Beh |
| `g` | ل Lam | `h` | ا Alef | `i` | ه Heh |
| `j` | ت Teh | `k` | ن Noon | `l` | م Meem |
| `m` | ة Teh-Marbuta | `n` | ى Alef-Maksura | `o` | خ Khah |
| `p` | ح Hah | `q` | ض Dad | `r` | ق Qaf |
| `s` | س Seen | `t` | ف Feh | `u` | ع Ain |
| `v` | ر Reh | `w` | ص Sad | `x` | ء Hamza |
| `y` | غ Ghain | `z` | ئ Yeh-Hamza | `[` | ج Jeem |
| `]` | د Dal | `` ` `` | ذ Thal | | |

Punctuation and digits:

| Key | Result | | Key | Result |
|---|---|---|---|---|
| `0`–`9` | ٠–٩ Arabic-Indic digits | | `'` | ط Tah |
| `,` | و Waw | | `;` | ك Kaf |
| `.` | ز Zain | | `/` | ظ Zah |
| `(` | `)` (mirrored) | | `)` | `(` (mirrored) |

### Russian typing layout

CP866 Cyrillic letters live at 0x80–0xAF and 0xE0–0xF1. The keyboard
layout is the standard **JCUKEN** layout (the layout printed on
Soviet/Russian PC keycaps). Both letter cases are mapped — unlike
Hebrew or Arabic, Russian has case.

Letters (lowercase shown; Shift gives uppercase):

| Key | Letter | Key | Letter | Key | Letter |
|---|---|---|---|---|---|
| `q` | й (short i) | `w` | ц (tse) | `e` | у (u) |
| `r` | к (ka) | `t` | е (ye) | `y` | н (en) |
| `u` | г (ge) | `i` | ш (sha) | `o` | щ (shcha) |
| `p` | з (ze) | `[` | х (kha) | `]` | ъ (hard sign) |
| `a` | ф (ef) | `s` | ы (yeru) | `d` | в (ve) |
| `f` | а (a) | `g` | п (pe) | `h` | р (er) |
| `j` | о (o) | `k` | л (el) | `l` | д (de) |
| `;` | ж (zhe) | `'` | э (e) | `z` | я (ya) |
| `x` | ч (che) | `c` | с (es) | `v` | м (em) |
| `b` | и (i) | `n` | т (te) | `m` | ь (soft sign) |
| `,` | б (be) | `.` | ю (yu) | `` ` `` | ё (yo) |

Punctuation and digit keys:

| Key | Result |
|---|---|
| `0`–`9` | digits 0–9 (passthrough — CP866 has no Arabic-Indic digits) |
| `/` | `.` (period) — JCUKEN swap |
| `?` | `,` (comma) — JCUKEN swap |
| `~` | Ё (uppercase Yo) |

### BiDi and visual order

The file always stores text in **logical order** — the order you typed
the letters. At display time, OpenQT runs a BiDi reordering algorithm
to place the bytes correctly on screen:

1. Strip inline format codes from the line.
2. If RTL paragraph: reverse the entire line.
3. Walk the reversed line and re-reverse each English / digit run so
   numbers and Latin words read left-to-right within the larger RTL
   flow.
4. Apply Arabic positional shaping (since v3.1; see below).

The cursor position maps (`log_to_vis`, `vis_to_log`) keep the cursor
in the right place even when the visual order doesn't match the typing
order.

### Arabic positional shaping

In version 3.1 and 3.2 OpenQT performs **proper Arabic shaping** on the
DOS screen. Each Arabic letter is rendered in the appropriate
positional form (isolated, initial, medial, final) based on its
neighbors, so the screen looks cursive — joined, not just adjacent
isolated glyphs.

The shaping is purely a display-time transformation: the file always
stores the abstract isolated-form CP864 byte. So save/load and the
converters (oqt2word, txt2oqt, qt2oqt) are unaffected.

What's covered:

- All dual-joining letters (Beh, Teh, Theh, Jeem, Hah, Khah, Seen,
  Sheen, Sad, Dad, Feh, Qaf, Kaf, Lam, Meem, Noon, Heh, Yeh) take
  CP864 initial form when followed by another joining letter.
- Heh, Ain, Ghain take their separate **medial** form when in the
  middle of a chain.
- Yeh, Ain, Ghain, Alef, Alef-Madda, Alef-Hamza-above, Alef-Maksura,
  and the Lam-Alef-Madda ligature take their separate **final** form
  when joined from the right.
- Other right-joining-only letters (Dal, Thal, Reh, Zain, Waw,
  Teh-Marbuta) keep their isolated glyph in all positions — CP864
  doesn't have separate final-form bytes for them and the isolated
  glyph already reads correctly in joined contexts.

What's not (yet) covered:

- True plain Lam-Alef ligature (لا without madda) — rendered as
  Lam-initial + Alef-final, which the font draws acceptably but not as
  a single fused glyph. The Lam-Alef-Madda ligature (typed via 'B')
  does work as a fused glyph.
- Tah and Zah have no separate initial/medial bytes in CP864, so they
  appear with their isolated glyph in joined contexts (a small visual
  imperfection).
- Yeh-Hamza has only the initial-form glyph in CP864 — it appears
  with that glyph even in isolation.

In **LibreOffice and Word**, the `oqt2word` export produces UTF-8 in
the basic Arabic block (U+0621–U+064A), which means the *receiving*
renderer does its own shaping. The result is fully correct Arabic
typography regardless of OpenQT's on-screen limitations.

### File format

OpenQT files are plain DOS text files (CR/LF line endings) using a
**mixed CP862 + CP864 byte encoding** specific to OpenQT. The two
codepages put their letters in disjoint byte ranges, which is what lets
all three scripts coexist in a single file:

- ASCII / Latin: 0x00–0x7F (shared baseline).
- Hebrew letters: **CP862** range 0x80–0x9A.
- Arabic letters: **CP864**, scattered across 0xA0–0xFE.
- Inline formatting: in-band toggle bytes (see below).

A document with only Hebrew is a standard CP862 file. A document with
only Arabic is a standard CP864 file (in the abstract-isolated form
the editor uses for storage). A document with **both** Hebrew and
Arabic is an OpenQT-specific dual encoding — readable cleanly by
OpenQT and by `oqt2word` / `txt2oqt`, but not by tools that expect a
single standard codepage.

Documents authored in Russian mode are a separate beast: they are
**standard CP866** byte streams. CP866 byte ranges *overlap* both
CP862 Hebrew (at 0x80–0x9A) and CP864 Arabic (at 0xA0–0xFE), so a
CP866 file cannot also contain Hebrew or Arabic letters — and the
external converters need an explicit `/R` flag to know to interpret
the bytes as CP866 rather than as the default mixed CP862+CP864
trilingual encoding.

Inline formatting toggle bytes:

| Byte | Meaning |
|---|---|
| `0x02` (FMT_BOLD) | Toggle bold on / off |
| `0x03` (FMT_UNDERLINE) | Toggle underline on / off |
| `0x04` (FMT_BOLDUNDER) | Toggle bold + underline together |
| `0xAF` (FMT_START) / `0xAE` (FMT_END) | QText 5.5 compatibility markers (still recognized on read) |

A byte sequence like `\x02hello\x02` displays "hello" in bold. The
first `\x02` switches bold on; the second switches it off.

### Encryption

If you save a file with a password, the on-disk format is prepended
with the magic header `OQT-ENC1` (8 bytes) and the rest of the file is
encrypted byte-by-byte. OpenQT detects the magic header on load and
prompts for the password automatically.

Caveats:

- The encryption is intended as a deterrent against casual reading on
  shared machines, not as serious cryptography. Don't trust it for
  anything sensitive.
- Encrypted files must be opened in OpenQT — `oqt2word`, `txt2oqt` etc.
  do not handle the encrypted format.

---

## oqt2word — export to UTF-8

`oqt2word` converts an OpenQT text file into a **UTF-8** plain-text
file suitable for opening in Microsoft Word, LibreOffice, Notepad++,
or any modern Unicode-aware editor.

### Usage

```
oqt2word [/R] <input.openqt> <output.utf8>
```

Examples:

```
oqt2word mydoc.txt mydoc-w.txt        # default: Hebrew + Arabic + English
oqt2word /R rusdoc.txt rusdoc-w.txt   # Russian (CP866) input
```

### What it converts

| OpenQT bytes | UTF-8 output |
|---|---|
| 0x80–0x9A (CP862 Hebrew) | U+05D0–U+05EA basic Hebrew block |
| CP864 Arabic letters (any positional form) | basic Arabic block U+0621–U+064A |
| CP864 Arabic-Indic digits 0xB0–0xB9 | U+0660–U+0669 |
| Lam-Alef ligature bytes 0xF9 / 0xFA | two codepoints: U+0644 + U+0622 (or +0627) |
| ASCII | passes through |
| Format bytes (0x02 / 0x03 / 0x04) | dropped |
| QText markers (0xAF / 0xAE) | dropped |

A UTF-8 BOM (`EF BB BF`) is written at the start of the output so Word
and other editors recognize the encoding immediately.

### `/R` (Russian) mode

When `/R` is given, every high byte (0x80–0xFF) is routed through a
single CP866 → Unicode table — Cyrillic letters → U+0410–U+044F,
Yo / Ye / Yi / Short-U → their codepoints, the box-drawing block
0xB0–0xDF → U+2500.. block, the misc tail (degree, sqrt, No.,
currency, etc.) → their Unicode equivalents. This flag is **required**
for any document written in Russian mode (`OQTR`, `OQT R`, or
`OPENQT /R`) — without it, the converter would mis-classify Cyrillic
bytes as Hebrew or Arabic because the byte ranges overlap.

`/R` is also the only mode that round-trips box-drawing
characters: in default Hebrew/Arabic mode, bytes at 0xB0–0xDF are
interpreted as part of the Arabic letter range and emitted as Arabic
codepoints — perfect for Arabic text, but lossy for any document that
relies on box-drawing as box-drawing.

### Why basic Arabic block?

The OpenQT file stores positional-form CP864 bytes that have already
been shaped by the editor's display logic. We could map them to the
**Arabic Presentation Forms-B** block (U+FE70–U+FEFF) and preserve the
exact shape, but doing so would prevent LibreOffice/Word from doing
its own shaping — the letters would appear as *isolated forms only*,
not joined.

By mapping to the basic block, the receiving renderer takes care of
shaping with proper ligatures, kashida, and font-specific joining
rules. This is the single biggest win of using `oqt2word` instead of
just renaming the file.

### Round-trip

You can edit in LibreOffice and bring the text back via `txt2oqt`
(below). The round-trip preserves the abstract letter sequence; the
specific positional forms picked by OpenQT and by LibreOffice may
differ since they're chosen by each renderer independently, but the
underlying *letters* are the same.

---

## txt2oqt — import from UTF-8

`txt2oqt` is the inverse of `oqt2word`: it takes a **UTF-8** text file
(produced by LibreOffice / Word / any modern editor) and converts it
into OpenQT format. It also still supports the original use case of
converting legacy DOS-encoded Hebrew text files.

### Usage

```
txt2oqt <input.txt> <output.openqt> [/V] [/D] [/U] [/R]
```

Flags:

| Flag | Meaning |
|---|---|
| `/V` | Keep visual order (legacy DOS mode only). |
| `/D` | Force legacy DOS encoding mode (skip BOM auto-detect). |
| `/U` | Force UTF-8 mode (skip BOM auto-detect). |
| `/R` | Target OpenQT Russian (CP866) instead of Hebrew/Arabic. |

By default, `txt2oqt` looks for a UTF-8 BOM at the start of the input.
If found, it switches to UTF-8 mode and consumes the BOM. If not
found, it falls through to legacy DOS mode.

### UTF-8 mode (default)

| Input | Output |
|---|---|
| ASCII | passes through |
| Hebrew U+05D0–U+05EA | CP862 0x80–0x9A |
| Arabic basic block U+0621–U+064A | CP864 abstract isolated bytes |
| Arabic-Indic digits U+0660–U+0669 | CP864 0xB0–0xB9 |
| Arabic punctuation (، ؛ ؟) | CP864 0xAC, 0xBB, 0xBF |
| Tatweel U+0640 | CP864 0xE0 |
| Lam-Alef Presentation Forms (U+FEF5–U+FEFC) | CP864 0xF9 |
| Other Presentation Forms-B (U+FE80–U+FEFC) | folded back to base letter, then CP864 |
| BOM (U+FEFF) | dropped |
| Other / unrepresentable | silently skipped |

Note: U+0625 (Alef-Hamza-below) is mapped to CP864 0xC3
(Alef-Hamza-above) because the embedded CP864 font does not contain a
distinct glyph for the below variant. This is a closest-match
fallback.

### `/R` (Russian) mode

Adds a Cyrillic + CP866-extras lookup that runs *before* the default
Hebrew/Arabic resolution. With `/R`:

| Input | Output |
|---|---|
| Cyrillic uppercase U+0410–U+042F | CP866 0x80–0x9F |
| Cyrillic lowercase a..p (U+0430–U+043F) | CP866 0xA0–0xAF |
| Cyrillic lowercase r..ya (U+0440–U+044F) | CP866 0xE0–0xEF |
| Yo (U+0401 / U+0451) | CP866 0xF0 / 0xF1 |
| Ukrainian Ye, Yi; Belarusian Short U | CP866 0xF2–0xF7 |
| Box-drawing U+2500..U+25A0 | CP866 0xB0–0xDF (preserved exactly) |
| Misc graphics tail (`°`, `∙`, `·`, `√`, `№`, `¤`, `■`, nbsp) | CP866 0xF8–0xFF |
| Hebrew / Arabic codepoints | dropped |

Order reversal in legacy DOS mode is also disabled under `/R` (Russian
DOS files are always logical = visual since Russian is LTR). Combine
with `/U` if your input lacks a UTF-8 BOM.

### Markdown formatting

Within UTF-8 input, two-character markdown markers are translated into
OpenQT inline format toggles:

| Markdown | OpenQT |
|---|---|
| `**bold**` | `0x02 ... 0x02` (FMT_BOLD toggle) |
| `__underline__` | `0x03 ... 0x03` (FMT_UNDERLINE toggle) |

Single `*` and single `_` are passed through as ordinary ASCII. To
include a literal `**` or `__` in the output, you'd need to manually
edit them out — there's no escape syntax.

### Legacy DOS mode

If the input has no UTF-8 BOM (or you pass `/D`), `txt2oqt` runs the
original DOS-text behavior, now extended to handle Arabic CP864
letters as well as Hebrew CP862:

- Bytes are kept as-is (no encoding conversion).
- Lines containing right-to-left letters are reversed visual → logical
  unless `/V` is given.
- English / digit runs within reversed lines are re-reversed back to
  LTR.
- Lines are word-wrapped at column 71.

Use `/V` for files that were produced in *visual* order and that you
intend to view in OpenQT's RTL mode without re-conversion.

### Examples

```
# UTF-8 from LibreOffice
txt2oqt mydoc-utf8.txt mydoc.txt

# Legacy DOS Hebrew TXT (visual order) -> OpenQT logical order
txt2oqt /D oldfile.txt mydoc.txt

# Legacy DOS file but keep visual order (open in OpenQT RTL mode)
txt2oqt /D /V oldfile.txt mydoc.txt

# Russian UTF-8 from LibreOffice -> OpenQT CP866 (open via OQTR)
txt2oqt /R rusdoc-utf8.txt rusdoc.txt
```

---

## txt2rtf — export to RTF (Hebrew + English)

`txt2rtf` converts an OpenQT document into a **Rich Text Format** (`.rtf`)
file that opens with full formatting in Microsoft Word, LibreOffice and
WordPad. Unlike `oqt2word` (which produces plain UTF-8 and therefore drops
all bold/underline), `txt2rtf` keeps the inline formatting — this is the
tool to use when you want the *styled* document, not just the text.

It is intentionally scoped to **Hebrew + English** files (the `OQT` / `OQT H`
launchers). Arabic (CP864) and Russian (CP866) documents are not handled;
use `oqt2word` for those.

```
txt2rtf <input.openqt> <output.rtf>
```

Example:

```
txt2rtf mydoc.txt mydoc.rtf      # then open mydoc.rtf in Word / LibreOffice
```

What it preserves and how:

- **Bold, underline, and bold+underline** runs map to RTF `\b` / `\ul`
  (and both together). The formatting follows OpenQT's own toggle model
  exactly, including the fact that styling **resets at the start of each
  line**, so the RTF matches what you see on screen.
- **Hebrew** letters are written as Unicode escapes (U+05D0–U+05EA) with a
  Hebrew-charset font, and Hebrew runs are tagged right-to-left so Word /
  LibreOffice perform their own bidirectional layout — the same logical-order
  storage OpenQT uses, no manual reordering. A line containing any Hebrew
  becomes an RTL paragraph; English-only lines stay left-to-right.
- The default fonts are **David** (Hebrew) and **Arial** (Latin); change the
  font names in your word processor after opening if you prefer others.

Encrypted documents are rejected with a message — open the file in OpenQT,
save it **without** a password, then run `txt2rtf`.

---

## Round-trip workflow

The full intended workflow for editing OpenQT documents in modern
tools and bringing them back:

```
   +-------------+    oqt2word     +-------------+
   |   OpenQT    |  ------------>  | LibreOffice |
   |  (DOS box)  |   (UTF-8 .txt)  | / Word /    |
   |             |                 | Notepad++   |
   |             |  <-----------   |             |
   +-------------+    txt2oqt      +-------------+
                    (UTF-8 .txt
                     with optional
                     **bold** /
                     __underline__)
```

Step-by-step:

1. Type your document in OpenQT under Arabic or Hebrew mode. Save with
   F2.
2. Exit DOS, run `oqt2word doc.txt doc-utf8.txt` in any environment.
3. Open `doc-utf8.txt` in LibreOffice (or Word). The text will display
   with proper Arabic shaping, Hebrew right-to-left, and English in
   normal LTR — all handled by the modern renderer.
4. Edit freely. When done, **save as plain text, UTF-8 encoded** (in
   LibreOffice: *Save As* → choose "Text (.txt)" → tick "Use Unicode
   (UTF-8)").
5. Run `txt2oqt doc-utf8.txt doc.txt`. The file is now OpenQT-ready.
6. Reopen in OpenQT — the text appears with the same content,
   re-shaped on screen by OpenQT's display engine.

If you want to add bold or underline to text in LibreOffice and have
it carry through, use markdown markers (`**word**` for bold,
`__word__` for underline) since LibreOffice's plain-text export
discards rich-text formatting. The editor's bold and underline state
is encoded in OpenQT as in-band toggle bytes that markdown markers
convert to directly.

---

## Other tools

The repository also includes two converters for the **QText 5.5** file
format. These are useful if you have legacy QText files or want
OpenQT files to interoperate with QText.

| Tool | Usage |
|---|---|
| `qt2oqt input.qt output.txt` | QText 5.5 → OpenQT (visual → logical reorder) |
| `oqt2qt input.txt output.qt` | OpenQT → QText 5.5 (rewraps to QText's 71-column line, embeds `0xAF` / `0xAE` markers) |

QText 5.5 stores Hebrew in CP862 like OpenQT, but in **visual order**
(the order letters appear on screen, not the order they were typed).
The reorder logic is the inverse of OpenQT's BiDi pass.

---

## Version history

### Version 3.6 (current)

- **New converter `txt2rtf`** — exports an OpenQT **Hebrew + English**
  document to **RTF** (`.rtf`), preserving **bold**, **underline** and
  **bold+underline** runs so the styled text opens correctly in Microsoft
  Word, LibreOffice and WordPad. Hebrew is emitted as Unicode with
  right-to-left runs (logical order, like the editor), and the styling
  follows OpenQT's own toggle model including the per-line reset. Encrypted
  files are rejected with guidance. Like the other tools it is a 32-bit
  DOS/4GW build. See [txt2rtf — export to RTF](#txt2rtf--export-to-rtf-hebrew--english).

### Version 3.5

- **Smart Paste from the host clipboard** — a new Tools-menu item
  ("Paste Host", also bound to **Alt+V**) that inserts the host
  clipboard at the cursor. Unlike DOSBox-X's own clipboard paste
  (Ctrl+F6), which converts the clipboard to the active DOS code page
  and so mangles anything outside the 27 plain CP862 letters (vowel
  points / niqqud, smart quotes) and re-maps pasted ASCII through the
  Hebrew keyboard layout (e.g. `.` → ץ), Smart Paste reads the
  clipboard as full UTF-8 on the host: it strips niqqud and Arabic
  harakat, folds smart quotes/dashes to ASCII, maps Hebrew / Arabic /
  Russian letters to the right codepage (following the editor's current
  F4 language), and the editor inserts the bytes directly — never
  through the keyboard layout — so punctuation stays literal. Runs
  through the same `host_helper/` daemon as the other Tools features;
  needs `xclip` (or `xsel` / `wl-clipboard`) on the host.

### Version 3.4

- **Host-assisted language features** in the Tools menu (Alt+T):
  English spell check, Read Aloud, Stop Speech, Translate to Hebrew,
  and Dictate (speech-to-text). These run on the Linux/DOSBox host via
  a helper daemon (`host_helper/`) — the microphone, speakers, speech
  engine, and network all live host-side, which is what makes
  dictation and real translation possible at all from a DOS program.
  The editor talks to the daemon through a file handshake in
  `C:\OPENQT\BRIDGE`; all UTF-8 conversion is done host-side, so
  `openqt.c` still only ever handles its native CP862/CP864/CP866
  bytes. The features are inert (no errors, just a "Helper not
  responding" notice) when the daemon isn't running. See
  [Host-assisted features](#host-assisted-features-speech-spell-check-translation).

### Version 3.3

- **Russian (CP866) input mode**. F4 now cycles English → Hebrew →
  Arabic → Russian (and auto-flips the paragraph direction to match —
  RTL for HEB / ARA, LTR for ENG / RUS, F5 still overrides). New
  `RUSVGA.EXE` uploads a CP866 8x16 font (CP437 base extracted from
  `HEBVGA.COM`, Cyrillic glyphs from the X11 `xfonts-base` 8x13 raster
  font, padded into the VGA 8x16 cell). New `OQTR.BAT` launcher and
  new `OPENQT /R` command-line flag. Russian is its own session — its
  byte ranges overlap Hebrew + Arabic and cannot share the trilingual
  screen font.
- **Converter `/R` flag** on both `oqt2word` and `txt2oqt`. Required
  for round-tripping Russian documents to/from UTF-8 — without it the
  converters would mis-classify Cyrillic bytes as Hebrew or Arabic.
  Box-drawing chars at 0xB0–0xDF round-trip as themselves under `/R`,
  not as Arabic letters.
- **BiDi engine** gates Arabic positional shaping behind
  `input_lang != LANG_RUS` (otherwise Cyrillic bytes would be
  shape-substituted into other CP866 bytes — typically box-drawing).
  Russian text is treated as LTR regardless of `doc.rtl_mode` so a
  document still in RTL mode displays Russian correctly.

### Version 3.2

- **Trilingual editing on one screen**. Hebrew, Arabic, and English
  can now coexist in the same document, on the same line, all visible
  at once. Enabled by a new `/P` (partial-upload) flag in `ARABVGA`
  that overlays only the 95 Arabic slots at 0xA0–0xFE on top of an
  already-loaded CP862 font, so HEBVGA's Hebrew at 0x80–0x9A stays
  intact. Driven by a new `OQT 3 file.txt` launcher path.
- **In-editor Docs viewer**. New "Docs…" item in the Help menu
  (Alt+H) opens `OPENQT.HLP` — a paged read-only viewer that runs
  each line through the same BiDi + Arabic-shaping pipeline as the
  editor, so the trilingual user guide displays natively in Hebrew,
  Arabic, and English. The `.HLP` file is plain text in OpenQT's
  mixed CP862+CP864 encoding; a Python helper (`make_hlp.py`)
  regenerates it from UTF-8 source.
- **Full 4-form Arabic positional shaping**. Yeh, Ain, Ghain, Heh now
  pick CP864 final / medial form bytes when the context calls for them;
  Alef-Maksura and the Lam-Alef-Madda ligature also pick up final-form
  joining. Same display-only architecture as 3.1 — file storage is
  unchanged.
- **`txt2oqt` rewrite**: now accepts UTF-8 input by default with
  Hebrew, Arabic, and English support, plus markdown bold and underline
  markers. The original legacy DOS mode is preserved (auto-detected
  via missing BOM, or via `/D` flag) and now handles Arabic CP864
  alongside Hebrew CP862. Trilingual files round-trip cleanly: each
  byte is classified by range, no extra flag needed.
- **`oqt2word`** updated to emit basic Arabic block (U+06xx) instead
  of Presentation Forms-B, so LibreOffice / Word can shape and join
  the letters with their own typography engine. Also handles
  trilingual files transparently: Hebrew bytes go to U+05D0–05EA and
  Arabic bytes go to U+0621–064A in the same output stream.
- **`ARABVGA` rebuilt** from current `arabvga.c` source — embeds the
  CP864 font directly in the binary (no runtime CPI dependency), uses
  Watcom's `REGPACK` API for the BIOS font-upload call, and adds the
  new `/P` partial-overlay mode plus `/?` help text.

### Version 3.1

- First Arabic shaping pass on the DOS screen. Dual-joining letters
  switch to CP864 initial form when followed by a joining letter, and
  Alef variants switch to final form when joined from the right.

### Version 3.0

- Arabic input added (third language, alongside Hebrew and English).
- CP864 encoding, Microsoft Arabic 101 keyboard layout.
- BiDi extended to classify Arabic letters as right-to-left.
- ARABVGA font uploader with embedded CP864 font (no external CPI
  files needed).
- F4 cycles through three languages instead of two.
- Document limit raised to 30 000 lines.
- Password encryption added.

Earlier versions (1.0–2.x) were Hebrew-only.

---

## Troubleshooting

**"OPENQT.HLP not found" when I open Help → Docs…**  
Two common causes. (1) DOSBox's drive cache is stale — if you added
or regenerated `OPENQT.HLP` on the host while DOSBox was already
running, run `rescan` inside DOSBox to invalidate its cached
directory listing, then try Docs… again. (2) Your DOS working
directory isn't where the file lives. The viewer searches current
dir, parent dir, `\OPENQT\`, and `C:\OPENQT\` in that order — if
none match, copy `OPENQT.HLP` next to your working file or `cd
\OPENQT` before launching `OQT 3`. To regenerate the file from its
UTF-8 source: `python3 make_hlp.py` on the host.

**Tri-script `OQT 3` shows Hebrew correctly but Arabic as gibberish
(or vice-versa).**  
Almost always means an **old `ARABVGA.COM`** is still being picked up
instead of the new `ARABVGA.EXE`. The pre-3.2 ARABVGA did a full
256-character upload regardless of arguments — running it second
trampled HEBVGA's Hebrew, running it first got trampled by HEBVGA.
Verify with `ARABVGA /?` in DOSBox: the output should be the multi-line
help text mentioning `/P -> partial upload, 0xA0-0xFE only`. If you see
"loaded embedded CP864 Arabic font" instead, the stale binary is
running. Delete `ARABVGA.COM`, rebuild from `arabvga.c`, and re-run.

**The editor opens in Hebrew when I run `OQTA`.**  
Most likely there are two `OPENQT.EXE` files in the directory and DOS
is picking the older one (case-insensitive lookup). Delete any stale
copy and rebuild. The current binary should be ~75 KB.

**Arabic letters appear as isolated glyphs, not connected.**  
Confirm the binary is from version 3.1 or later — the editor header
in `openqt.c` line 4 should say `Version 3.1` or `Version 3.2`. Older
binaries don't run the shaping pass.

**Menus and dialog boxes look corrupted under `OQTA`.**  
This is partly expected: CP864 reuses the byte range 0xC0–0xDF where
CP437 normally has box-drawing characters. The launcher passes `/A` to
`OPENQT`, which switches dialog drawing to ASCII characters to avoid
the clash. If you see Arabic letters in dialog frames, the `/A` flag
isn't reaching the binary — check that you launched via `OQTA.BAT` or
`OQT A`, not `OPENQT` directly.

**LibreOffice shows the Arabic text correctly, but OpenQT's screen
looks slightly off in some words.**  
This is the v3.2 limitation: a few letters (Tah, Zah, Yeh-Hamza, plain
Lam-Alef without madda) lack separate positional-form glyphs in CP864.
The export to UTF-8 is fine — the on-screen rendering is just a font
limitation. Adding custom glyphs via a modified font upload would fix
this.

**`txt2oqt` reads my UTF-8 file as legacy DOS.**  
The file might lack a BOM. Either save it from LibreOffice with the
"Use Unicode (UTF-8)" *and* "Include BOM" options checked, or pass
`/U` to `txt2oqt` to force UTF-8 mode.

**Typing Hebrew on Linux works fine but my keys do nothing in DOSBox.**  
Make sure your DOSBox keyboard layout is set to a US layout (or
whatever maps physical keys to ASCII unchanged). OpenQT does its own
mapping from ASCII to Hebrew/Arabic via `heb_map[]` and `arab_map[]`,
so it expects ASCII-style keystrokes coming in.
