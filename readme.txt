                         ╔══════════════════════════════════╗
                         ║          O P E N Q T             ║
                         ║    Hebrew Word Processor         ║
                         ║     QText 5.5 Compatible         ║
                         ╚══════════════════════════════════╝

                              Version 3.9.1 - Public Domain

         NOTE: This file is a legacy summary. The complete, current
         documentation is in README.md (Hebrew/English/Arabic/Russian,
         all tools and converters). See it for anything not covered here.


OVERVIEW
========

OpenQT is a Hebrew word processor for DOS, designed to be compatible with
QText 5.5. It provides full Hebrew language support with right-to-left
text editing, automatic final letter conversion, and a familiar menu-based
interface.


REQUIREMENTS
============

- IBM PC/AT or compatible
- DOS 3.3 or later (MS-DOS, PC-DOS, DR-DOS, FreeDOS)
- VGA display (for Hebrew font support)
- 640KB RAM minimum
- Watcom C/C++ compiler (for building from source)


FILES INCLUDED
==============

OPENQT.C     - Main source code
MAKEFILE     - Watcom make file
BUILD.BAT    - Quick build script
README.TXT   - This file
HEBVGA.COM   - Hebrew VGA font (Font Mania 2.2)


BUILDING FROM SOURCE
====================

1. Install Watcom C/C++ compiler
2. Set up environment: CALL WATCOM\OWSETENV.BAT
3. Run: BUILD.BAT

For 16-bit version (works on any DOS):
   BUILD 16

For 32-bit version (requires DOS4GW):
   BUILD 32


KEYBOARD SHORTCUTS
==================

┌───────────────────────────────────────────────────────────────────────┐
│ Function Keys                                                         │
├───────────────────────────────────────────────────────────────────────┤
│ F1      Help screen                                                   │
│ F2      Save file                                                     │
│ F3      Open file                                                     │
│ F4      Toggle Hebrew/English mode                                    │
│ F5      Toggle RTL/LTR mode                                           │
│ F6      Copy block                                                    │
│ F7      Find text                                                     │
│ F8      Replace text                                                  │
│ F9      Paste block                                                   │
│ F10     Open menu bar                                                 │
├───────────────────────────────────────────────────────────────────────┤
│ Navigation                                                            │
├───────────────────────────────────────────────────────────────────────┤
│ Arrow Keys    Move cursor                                             │
│ Home          Start of line                                           │
│ End           End of line                                             │
│ PgUp          Page up                                                 │
│ PgDn          Page down                                               │
│ Ctrl+Home     Start of document                                       │
│ Ctrl+End      End of document                                         │
│ Ctrl+Left     Word left                                               │
│ Ctrl+Right    Word right                                              │
├───────────────────────────────────────────────────────────────────────┤
│ Editing                                                               │
├───────────────────────────────────────────────────────────────────────┤
│ Insert        Toggle insert/overwrite mode                            │
│ Delete        Delete character at cursor                              │
│ Backspace     Delete character before cursor                          │
│ Enter         Insert new line                                         │
│ Tab           Insert tab (spaces to next tab stop)                    │
│ Esc           Cancel block selection / Close menu                     │
├───────────────────────────────────────────────────────────────────────┤
│ Menu Shortcuts                                                        │
├───────────────────────────────────────────────────────────────────────┤
│ Alt+F         File menu                                               │
│ Alt+E         Edit menu                                               │
│ Alt+S         Search menu                                             │
│ Alt+B         Block menu                                              │
│ Alt+O         Options menu                                            │
│ Alt+H         Help menu                                               │
│ Alt+X         Exit program                                            │
└───────────────────────────────────────────────────────────────────────┘


MENU SYSTEM
===========

FILE MENU
---------
New           - Create new document
Open          - Open existing file
Save          - Save current file
Save As       - Save with new name
Print         - Send to printer
Exit          - Exit OpenQT

EDIT MENU
---------
Undo          - Undo last action (not yet implemented)
Redo          - Redo undone action (not yet implemented)
Cut           - Cut selected block
Copy          - Copy selected block
Paste         - Paste from clipboard
Delete Line   - Delete current line
Select All    - Select entire document

SEARCH MENU
-----------
Find          - Search for text
Find Next     - Find next occurrence
Find Previous - Find previous occurrence
Replace       - Search and replace
Replace All   - Replace all occurrences
Go to Line    - Jump to specific line number

BLOCK MENU
----------
Start Block   - Begin block selection
End Block     - End block selection
Copy Block    - Copy selected block
Move Block    - Move selected block
Delete Block  - Delete selected block
Unmark Block  - Cancel block selection
Write to File - Save block to separate file

OPTIONS MENU
------------
Hebrew Mode   - Toggle Hebrew keyboard input
RTL Mode      - Toggle right-to-left display
Word Wrap     - Toggle automatic word wrap
Show Ruler    - Toggle ruler display
Insert Mode   - Toggle insert/overwrite
Set Colors    - Customize colors (not yet implemented)

HELP MENU
---------
Help Index    - Show keyboard help
Keyboard Help - Show keyboard shortcuts
About OpenQT  - About this program


HEBREW KEYBOARD LAYOUT
======================

When Hebrew mode is active (F4), the English keyboard maps to Hebrew:

    Q W E R T Y U I O P          /  '  ק ר א ט ו ן ם פ
    A S D F G H J K L ;   =>     ש ד ג כ ע י ח ל ך ף
    Z X C V B N M , . /          ז ס ב ה נ מ צ ת ץ .

Final letters (sofit) are automatically applied when appropriate:
- כ -> ך (Kaf -> Kaf Sofit)
- מ -> ם (Mem -> Mem Sofit)  
- נ -> ן (Nun -> Nun Sofit)
- פ -> ף (Pe -> Pe Sofit)
- צ -> ץ (Tsadi -> Tsadi Sofit)


HEBREW FONT SETUP
=================

OpenQT uses HEBVGA.COM (Font Mania 2.2 format) for Hebrew display.
Place the font file in one of these locations:

1. Same directory as OPENQT.EXE
2. C:\DOS\HEBVGA.COM

The font is automatically loaded at startup. If not found, Hebrew
characters may not display correctly.

Alternative: Run HEBVGA.COM manually before starting OpenQT:
   HEBVGA I
   OPENQT


FILE FORMAT
===========

OpenQT saves files as plain ASCII text with DOS line endings (CR+LF).
Files are compatible with:
- QText 5.x
- DOS Edit
- Windows Notepad
- Any text editor

Hebrew characters are stored using the IBM PC Hebrew code page (CP862).


STATUS BAR
==========

The status bar at the bottom shows:

┌────────────────────────────────────────────────────────────────────────┐
│ FILENAME.TXT [Modified]    Ln 1/100 Col 1   INS RTL        F1=Help    │
└────────────────────────────────────────────────────────────────────────┘

- Filename and modification status
- Current line / total lines
- Current column
- Insert/Overwrite mode (INS/OVR)
- Text direction (RTL/LTR)


TIPS AND TRICKS
===============

1. Quick Hebrew Toggle: Press F4 to switch between Hebrew and English
   typing modes without changing the display direction.

2. RTL Mode: Press F5 to change the text direction. Hebrew text reads
   right-to-left, English text reads left-to-right.

3. Block Selection: Press F6 twice (or use Block menu) to select text,
   then F6 to copy or F9 to paste.

4. Large Files: OpenQT supports up to 5000 lines. For larger files,
   consider splitting them.

5. Printing: Hebrew text may not print correctly on non-Hebrew printers.
   Consider using a Hebrew-capable printer driver.


KNOWN LIMITATIONS
=================

- Maximum 5000 lines per document
- Maximum 256 characters per line
- No spell checking
- No graphics or formatting (plain text only)
- Undo/Redo not yet implemented
- No macro support


QTEXT 5.5 COMPATIBILITY
=======================

OpenQT aims to be compatible with QText 5.5 files and keyboard shortcuts.
The following features are similar:

✓ Hebrew/English mode toggle
✓ RTL/LTR display toggle  
✓ Block operations
✓ Search and replace
✓ File operations
✓ Menu system
✓ Keyboard shortcuts


VERSION HISTORY
===============

(For the full, detailed history see README.md. Summary of headline changes:)

3.8.0 (2026)
- Reformat (reflow): Block -> Reformat re-wraps a marked block or the
  whole document to the column-71 margin (join short lines, split long
  ones, keep blank-line paragraph breaks). BiDi-safe; not undoable.
- Mouse pointer is now drawn by OpenQT itself (a bright highlight cell)
  so it is visible under DOSBox-X mouse-integration mode and fullscreen.

3.7.0 (2026)
- Mouse support (INT 33h): clickable menus, click-to-position the caret
  (LTR + RTL), press-drag block selection, and mouse-wheel scrolling.
  Inert when no mouse driver is present.

3.6.0 (2026)
- New converter txt2rtf: OpenQT (Hebrew + English) -> RTF, preserving
  bold / underline / bold+underline for Word / LibreOffice / WordPad.

3.0-3.5 (2025-2026)
- Arabic and Russian support (quadlingual editor), positional Arabic
  shaping, BiDi, encryption, UTF-8 / QText converters, in-editor Docs
  viewer, and host-assisted tools (spell check, read aloud, translate,
  dictate, smart paste).

1.0 (2024)
- Initial release
- Full Hebrew support
- RTL text editing
- Menu system
- Block operations
- Search and replace
- File operations
- Status bar
- Hebrew font loading


LICENSE
=======

OpenQT is released to the Public Domain.
Use, modify, and distribute freely.

Hebrew font (HEBVGA.COM) is copyright Rexxcom Systems 1992.


CONTACT
=======

For bug reports, suggestions, or contributions, please contact
the maintainer or submit to the project repository.


                              ════════════════════
                                 Happy Writing!
                              ════════════════════
