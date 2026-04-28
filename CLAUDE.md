# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

OpenQT is a DOS Hebrew/English/Arabic word processor (a QText 5.5 clone) written in C and built with the **Watcom C/C++** compiler. The repository is not a git repo; it is a working directory checked out on Linux but the program itself only runs under DOS (real-mode 16-bit, or 32-bit via DOS4GW). `DOS4GW.EXE` is bundled.

The author is Ronen Blumberg. Versioning is tracked **only** in the `openqt.c` header comment — currently **Version 3.2**. `config.h` and `readme.txt` lag far behind (1.0); the new `README.md` (~31 KB) is the user-facing doc and is current. Treat `openqt.c` and `README.md` as the source of truth when older files disagree.

## Build commands

Open Watcom is installed natively on the Linux host at `/opt/watcom/binl64/` — `wcl`, `wcl386`, `wmake`, etc. all run directly from a Linux shell. The binaries they produce are DOS executables, run inside DOSBox.

```bash
PATH=/opt/watcom/binl64:$PATH WATCOM=/opt/watcom INCLUDE=/opt/watcom/h \
  wcl -bt=dos -ml -ox arabvga.c -fe=arabvga.exe
```

Main editor (32-bit, DOS4GW — preferred):
```
wcl386 -bt=dos -l=dos4g openqt.c -fe=openqt.exe
```

Via the makefile (defaults to 16-bit real-mode build):
```
wmake              # 16-bit openqt.exe
wmake openqt32.exe # 32-bit DOS4GW build
wmake both
wmake clean
```

Helper utilities (each is a standalone single-file program with its own `*-watcom-compile.txt`):
```
wcl386 -bt=dos -l=dos4g oqt2qt.c   -fe=oqt2qt.exe   # OpenQT -> QText 5.5
wcl386 -bt=dos -l=dos4g qt2oqt.c   -fe=qt2oqt.exe   # QText 5.5 -> OpenQT
wcl386 -bt=dos -l=dos4g oqt2word.c -fe=oqt2word.exe # OpenQT -> UTF-8
wcl386 -bt=dos -l=dos4g txt2oqt.c  -fe=txt2oqt.exe  # UTF-8 (or legacy DOS) -> OpenQT
wcl       -bt=dos -ml -ox arabvga.c -fe=arabvga.exe # 16-bit Arabic font uploader
```

There are no tests, no lint config, and no CI. After rebuilding any tool, the user re-tests inside DOSBox manually.

`OPENQT.HLP` (the trilingual docs viewed by `Help → Docs…` inside the editor) is regenerated from a UTF-8 source string via:
```
python3 make_hlp.py     # writes OPENQT.HLP in OpenQT mixed CP862+CP864 byte encoding
```
No `openqt.exe` rebuild is needed when only the docs change — the viewer reads the file at runtime.

### DOS file-resolution gotcha

DOS resolves a bare command like `ARABVGA` in this order: `.COM` → `.EXE` → `.BAT`. **Always delete stale `.COM` builds before testing a new `.EXE` build of the same tool** — otherwise the old binary keeps running and silently ignores any new flags. We hit this twice: once with `OPENQT.EXE` (uppercase) shadowing the lowercase build (so `/A` was ignored), and once with the 4 KB `ARABVGA.COM` shadowing a fresh `ARABVGA.EXE` (so `/P` was ignored). DOSBox also caches its drive index — after replacing a binary on the host, the user typically runs `rescan` inside DOSBox to invalidate the cache.

### Launchers (BAT files at repo root)

| BAT | Mode | Font setup |
|---|---|---|
| `OQT 3 file` | **Trilingual** (Hebrew + Arabic + English) | `HEBVGA` then `ARABVGA /P` then `OPENQT /A` |
| `OQT H file` / `OQTH.BAT` | Hebrew + English | `HEBVGA` then `OPENQT` |
| `OQT A file` / `OQTA.BAT` | Arabic + English | `ARABVGA` then `OPENQT /A` |
| `OQT file` | Default (Hebrew) | same as `OQT H` |

Trilingual mode works because the CP862 Hebrew font (letters at 0x80–0x9A) and the CP864 Arabic font (letters at 0xA0–0xFE) write to **disjoint byte ranges**. `HEBVGA.COM` does a full 256-character upload (giving Hebrew at 0x80–0x9A plus a CP437 base everywhere else). `ARABVGA /P` then does a *partial* upload of just 0xA0–0xFE, overlaying Arabic letters on top without disturbing Hebrew. The `/P` flag was added in v3.2; older `ARABVGA` binaries don't have it. The `_arabic_font/` directory holds the original MS-DOS Arabic NLS files (VIDEO.CPI etc.) — kept as historical reference, no longer needed at runtime.

## Architecture

### One big file
Almost all editor functionality lives in **`openqt.c`** (~120 KB, ~3000 lines). It is a single translation unit with internal forward declarations near the top (search for `/* Forward declarations */` around line 340). The companion headers `config.h`, `hebrew.h`, and `bidi.h` are **not included by openqt.c** — openqt.c re-defines its own constants, its own `heb_map[128]` table, and its own inline BiDi implementation. Treat `config.h`/`hebrew.h`/`bidi.h` as historical reference; the live values are the `#define`s and tables inside `openqt.c`. `openqt.c` raises `MAX_LINES` to 30000 (vs. 5000 in `config.h`) and adds undo/encryption/formatting/Arabic-shaping state not present in those headers. `bidi.h` ends with an "INTEGRATION GUIDE FOR OPENQT.C" block; `BIDI_PATCH.txt` is an older patch describing how those changes were folded in. Both are documentation, not active code.

### Runtime model
- **Direct hardware access.** The editor writes to VGA text-mode memory at `0xB8000` (`video_mem` in openqt.c). Uses `conio.h`/`dos.h`/`i86.h` and INT 10h/16h BIOS calls. There is no abstraction layer — porting off DOS would mean rewriting the I/O.
- **Single global `Document doc`** holds the buffer (`char **lines`), per-character formatting (`unsigned char **styles`), cursor/scroll position, mode flags (`input_lang`, `rtl_mode`, `embedded_ltr`, `insert_mode`, `word_wrap`, `show_ruler`), block selection, clipboard, encryption state, and password.
- **Three input languages.** `doc.input_lang` is one of `LANG_ENG=0`, `LANG_HEB=1`, `LANG_ARA=2`. F4 cycles. `is_rtl_input()` returns true for HEB/ARA. Global `g_ascii_boxes` is set when the editor was launched with `/A` — dialogs/menus then draw with ASCII characters because CP864 letters at 0xC0–0xDF clash with CP437 box-drawing.
- **Undo stack** is a fixed array `UndoItem undo_stack[MAX_UNDO]` (250 entries) with `undo_pos` / `undo_count` / `redo_count`. Operation types are encoded as ints (0=insert_char, 1=delete_char, 2=insert_line, 3=delete_line, 4=backspace).

### Hebrew + Arabic + BiDi + shaping
- **Hebrew encoding**: IBM **CP862**, letters at 0x80–0x9A. `heb_map[128]` is the standard Israeli keyboard layout. `apply_final_letters()` does sofit substitution on word boundaries via `heb_final[256]`.
- **Arabic encoding**: IBM **CP864**. The user-typeable abstract isolated-form bytes span the full range 0xA9–0xFD via `arab_map[128]` (the Microsoft Arabic 101 layout) — including 0xA9, 0xAB, 0xBC, 0xBD, 0xC1–0xC9, 0xCF–0xD2, 0xD7–0xD8, 0xDF, 0xE8–0xE9, 0xEB, 0xEE, 0xEF, 0xF2–0xF3, 0xF8, 0xF9 (Lam-Alef-Madda ligature, single keystroke), 0xFB–0xFD. Don't trust the older `is_arabic_char` range comment in older docs that says "0xC1–0xDA + 0xE0–0xE9 + 0xFE" — the actual letter range is wider.
- **BiDi**: `get_bidi_type()` classifies both Hebrew and Arabic ranges as `BIDI_R`. The reordering algorithm doesn't distinguish scripts. **File storage is logical order** (typing order); display reorders to visual via `bidi_reorder()`. `log_to_vis[]` / `vis_to_log[]` map cursor positions.
- **Arabic positional shaping (v3.1/3.2)** lives in three tables and one function near `init_arabic_shaping()` and `shape_arabic_inplace()`:
  - `arab_join[256]`: joining class per CP864 byte (0=none, 1=right-only, 2=dual).
  - `arab_initial[256]` / `arab_medial[256]` / `arab_final[256]`: per-byte CP864 byte for each positional form, or `0` (medial/final) / self (initial) when the form doesn't exist in CP864.
  - `shape_arabic_inplace()` is called from inside `bidi_reorder()` on the temp `clean[]` buffer (after format-stripping, before LTR/RTL flip). It snapshots the original bytes, then walks them in logical order and substitutes positional-form bytes in place — length-preserving, so cursor maps and per-char attributes stay aligned. The shaping is **display-only**: the document buffer keeps abstract isolated bytes, so save/load and the converters are unaffected.
- **What CP864 has and doesn't have**: dual-joining letters (Beh, Teh, Lam, Meem, Noon, Yeh, etc.) have iso + initial bytes only — initial doubles as medial. A few letters (Heh, Yeh, Ain, Ghain, Alef variants, Alef-Maksura, Lam-Alef-Madda) have additional final and/or medial bytes. Tah/Zah/Yeh-Hamza have only one form for all positions. True plain Lam-Alef ligature (without madda) is not in this font — Lam-initial + Alef-final is used instead.
- **Embedded font detail**: in the FreeDOS `ega17.cpi` font that `arabvga.c` embeds, byte **0xC5 carries the Ain-final glyph**, NOT Alef-Hamza-below as the standard CP864 documentation claims. This affects round-trip mappings — `oqt2word.c` maps 0xC5 → U+0639 (Ain) and `txt2oqt.c` maps U+0625 (Alef-Hamza-below) → 0xC3 (Alef-Hamza-above) as a closest-match fallback.

### File format and converters

**Trilingual on-disk format**: a mixed CP862 + CP864 byte stream specific to OpenQT. ASCII at 0x00–0x7F (shared), Hebrew at 0x80–0x9A (CP862), Arabic at 0xA0–0xFE (CP864). Other DOS programs reading it with only CP862 loaded see Hebrew correctly but Arabic as garbage, and vice versa. Inline formatting toggles: `FMT_BOLD = 0x02`, `FMT_UNDERLINE = 0x03`, `FMT_BOLDUNDER = 0x04` (OpenQT's own toggles); `FMT_START = 0xAF` / `FMT_END = 0xAE` (QText 5.5 compatibility markers, recognized on read). Lines are CR/LF terminated.

**`oqt2word.c`** — OpenQT → UTF-8. Walks each byte, maps Hebrew bytes to U+05D0–U+05EA, Arabic CP864 bytes to the **basic Arabic block** U+0621–U+064A (so LibreOffice/Word can do their own positional shaping), Arabic-Indic digits to U+0660–U+0669, lam-alef-madda ligature bytes (0xF9/0xFA) to two-codepoint sequence U+0644 + U+0622, format toggles dropped. Writes a UTF-8 BOM. Trilingual files just work — every byte is classified by range.

**`txt2oqt.c`** — UTF-8 (default) or legacy DOS (auto-detected via missing BOM, or `/D` flag) → OpenQT. UTF-8 mode reverses the `oqt2word` mapping plus translates `**bold**` and `__underline__` markdown markers into FMT_BOLD/FMT_UNDERLINE toggles. It also folds Arabic Presentation Forms-B (U+FE70–U+FEFF) back to base letters before mapping to CP864, so files exported by older `oqt2word` builds still round-trip. Legacy mode handles both Hebrew CP862 and Arabic CP864 visual-to-logical reversal (extended in v3.2; was Hebrew-only before).

**`qt2oqt.c` / `oqt2qt.c`** — QText 5.5 ↔ OpenQT. Hebrew-only legacy. QText 5.5 stores in **visual** order; OpenQT in **logical** order. The pairs reverse Hebrew lines accordingly and translate `0xAF`/`0xAE` ↔ OpenQT's toggle bytes. Both also rewrap to QText's 71-column line.

### Encryption
Files saved with a password are prefixed with the magic string `OQT-ENC1` (8 bytes; see `ENCRYPT_MAGIC`). Detection is by header sniffing (`is_file_encrypted`); load/save go through `*_encrypted` variants. The cipher is XOR with a djb2-hashed-password-seeded LCG keystream (`generate_key` + `crypt_byte` + `encrypt_buffer`); same routine encrypts and decrypts (XOR is its own inverse). It operates byte-by-byte and is script-agnostic — trilingual files encrypt cleanly. The external converters do not handle encrypted files — open in OpenQT first, save unencrypted, then convert.

### In-editor Docs viewer

`show_docs()` is a paged read-only viewer triggered by Help → Docs… (Alt+H). It loads **`OPENQT.HLP`** — a plain-text file in OpenQT's mixed CP862+CP864 byte encoding (Hebrew at 0x80–0x9A, Arabic at 0xA0–0xFE) — and renders each line through the same `bidi_reorder()` (and therefore Arabic shaping) pipeline as the editor proper. The function temporarily sets `doc.rtl_mode = 1` for proper mixed-script rendering and restores it on exit. Static buffers `doc_lines[200][256]` and `line_lens[200]` cap the file at 200 lines × 256 cols; the viewer searches for the .HLP in cwd → `..\OPENQT.HLP` → `\OPENQT\OPENQT.HLP` → `C:\OPENQT\OPENQT.HLP`. Editing the docs is done by editing the `CONTENT` triple-quoted UTF-8 string in `make_hlp.py` and rerunning the script — no openqt.exe rebuild needed.

## Working in this repo

- `.bak` / `.obj` / `.map` / `.sym` / `.lk1` / `.mk1` files are compiler/IDE artifacts — leave them alone unless asked.
- When changing keybindings, screen layout, or document limits, update the `#define`s **at the top of `openqt.c`**. Editing `config.h` alone has no effect because it is not included.
- When adding a new top-level function in `openqt.c`, also add a forward declaration in the block near line 340, matching the existing style.
- **Watcom register-pack gotcha**: Open Watcom's `union REGS` / `WORDREGS` struct **does not expose the BP register**. BIOS calls that need BP (notably `INT 10h, AX=1110h` for VGA character-generator upload, where ES:BP is the font pointer) must use `union REGPACK` and the `intr()` function instead of `int86x()`. See `arabvga.c upload_to_vga_range()`.
- **Pre-existing code may not compile**: parts of the code that look fine on inspection may never have been built successfully on Open Watcom. Before assuming a "rewrite" is wrong, build it once cleanly. The original `arabvga.c upload_to_vga()` had `r.x.bp = ...` which fails to compile on modern Open Watcom — the existing 4 KB `ARABVGA.COM` predates the embedded-font version of the source entirely.
- **Display-only changes prefer `bidi_reorder`**: any glyph substitution that should not affect file storage (positional shaping, ligature substitution, etc.) belongs inside `bidi_reorder()` operating on the local `clean[]` buffer, not on `doc.lines[]`. Keep it length-preserving so cursor maps and `clean_attrs[]` stay aligned.
- **Stale binaries shadow new ones**: when adding a flag or fixing a bug in any tool, also delete the old binary (`.COM` and `.EXE` both, since DOS resolves `.COM` first). Verify the new one is loaded by running `<tool> /?` and checking that the help text matches the current source.
