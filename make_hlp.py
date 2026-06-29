#!/usr/bin/env python3
"""Generate OpenQT in-editor docs as a set of per-language .HLP files.

Why multiple files: a single .HLP can only contain text in scripts whose
fonts are loaded simultaneously. Hebrew (CP862, 0x80-0x9A) and Arabic
(CP864, 0xA0-0xFE) coexist under OQT 3 because their byte ranges are
disjoint. Russian (CP866) overlaps both. So a unified quad-script file
would render gibberish under any single-font session - half the page
shows correct glyphs, the rest shows whatever bytes happen to land at
those code points in the loaded font.

Outputs:
  OPENQT.HLP   - trilingual: English + Hebrew + Arabic. Targets OQT 3.
                 Under OQTH/OQTA the section in the unloaded script
                 still renders gibberish; the user can scroll past.
  OPENQTH.HLP  - English + Hebrew only. Targets OQTH (no Arabic font).
  OPENQTA.HLP  - English + Arabic only. Targets OQTA.
  OPENQTR.HLP  - English + Russian only. Targets OQTR.

Encoding rules:
  Hebrew  U+05D0..U+05EA       -> CP862 0x80..0x9A.
  Arabic  basic block          -> CP864 abstract isolated-form bytes.
  Russian U+0410..U+044F + Yo  -> CP866 byte values.
  ASCII                        -> passthrough.
"""

ARABIC_MAP = {
    0x0621: 0xC1, 0x0622: 0xC2, 0x0623: 0xC3, 0x0624: 0xC4, 0x0625: 0xC3,
    0x0626: 0xC6, 0x0627: 0xC7, 0x0628: 0xA9, 0x0629: 0xC9, 0x062A: 0xAA,
    0x062B: 0xAB, 0x062C: 0xAD, 0x062D: 0xAE, 0x062E: 0xAF, 0x062F: 0xCF,
    0x0630: 0xD0, 0x0631: 0xD1, 0x0632: 0xD2, 0x0633: 0xBC, 0x0634: 0xBD,
    0x0635: 0xBE, 0x0636: 0xEB, 0x0637: 0xD7, 0x0638: 0xD8, 0x0639: 0xDF,
    0x063A: 0xEE, 0x0640: 0xE0,
    0x0641: 0xBA, 0x0642: 0xF8, 0x0643: 0xFC, 0x0644: 0xFB, 0x0645: 0xEF,
    0x0646: 0xF2, 0x0647: 0xF3, 0x0648: 0xE8, 0x0649: 0xE9, 0x064A: 0xFD,
    0x060C: 0xAC, 0x061B: 0xBB, 0x061F: 0xBF,
    0x0660: 0xB0, 0x0661: 0xB1, 0x0662: 0xB2, 0x0663: 0xB3, 0x0664: 0xB4,
    0x0665: 0xB5, 0x0666: 0xB6, 0x0667: 0xB7, 0x0668: 0xB8, 0x0669: 0xB9,
}


def cyrillic_byte(cp: int) -> int:
    """Map Cyrillic / CP866-extras codepoint to its CP866 byte. Returns 0 if not in CP866."""
    if 0x0410 <= cp <= 0x042F: return 0x80 + (cp - 0x0410)   # А..Я
    if 0x0430 <= cp <= 0x043F: return 0xA0 + (cp - 0x0430)   # а..п
    if 0x0440 <= cp <= 0x044F: return 0xE0 + (cp - 0x0440)   # р..я
    if cp == 0x0401: return 0xF0                              # Ё
    if cp == 0x0451: return 0xF1                              # ё
    if cp == 0x2116: return 0xFC                              # №
    return 0


def encode(text: str) -> bytes:
    """UTF-8 -> OpenQT mixed CP862/CP864/CP866 bytes (single line should
    contain at most one non-Latin script + ASCII)."""
    out = bytearray()
    for ch in text:
        cp = ord(ch)
        if cp == 0x000A or cp == 0x000D:
            out.append(cp)
        elif cp < 0x80:
            out.append(cp)
        elif 0x05D0 <= cp <= 0x05EA:
            out.append(0x80 + (cp - 0x05D0))
        elif cp in ARABIC_MAP:
            out.append(ARABIC_MAP[cp])
        elif cp in (0xFEF5, 0xFEF6, 0xFEF7, 0xFEF8, 0xFEF9, 0xFEFA, 0xFEFB, 0xFEFC):
            out.append(0xF9)
        elif (b := cyrillic_byte(cp)):
            out.append(b)
        else:
            out.append(0x3F)
    return bytes(out)


def write_hlp(path: str, text: str) -> None:
    blob = encode(text).replace(b'\n', b'\r\n')
    with open(path, 'wb') as f:
        f.write(blob)
    print(f"Wrote {path} ({len(blob)} bytes)")


# ---------------------------------------------------------------------------
# Section pieces. Mix-and-match into the four output files below.
# ---------------------------------------------------------------------------

HEADER = """\
============================================================
            OPENQT v3.9.1 - USER GUIDE
============================================================

ENGLISH GUIDE
-------------

OpenQT is a quadlingual word processor for DOS that
supports English, Hebrew, Arabic, and Russian. The first
three can mix on one screen via OQT 3; Russian (CP866)
runs as its own session because its byte ranges overlap
Hebrew + Arabic. Press F4 to cycle the input language.

KEY COMMANDS:
  F1   Help            F2  Save           F3  Open
  F4   Switch lang     F5  RTL toggle     F7  Find
  F8   Replace         F9  Paste block    F10 Embed LTR
  Ctrl+B Bold          Ctrl+U Underline
  Ctrl+Z Undo          Ctrl+Y Redo
  Alt+V  Paste host     Alt+C  Copy to host
  Alt+X  Exit

LAUNCHERS:
  OQT 3 file   Trilingual (Hebrew + Arabic + English)
  OQTH  file   Hebrew + English only
  OQTA  file   Arabic + English only
  OQTR  file   Russian + English only (CP866, new in 3.3)

ARABIC SHAPING (v3.2):
  Cursive joining is automatic. Letters connect to their
  neighbours when CP864 provides positional form bytes.
  All four forms (isolated/initial/medial/final) used
  where available.

RUSSIAN MODE (v3.3):
  OQTR loads RUSVGA (CP866 font) and starts the editor in
  Russian input mode. F4 still cycles all four languages,
  but Russian text only renders right under RUSVGA - byte
  ranges overlap CP862 Hebrew + CP864 Arabic, so a Russian
  document cannot also contain Hebrew or Arabic letters.
  Use OQT 3 for the Heb+Ara+Eng combo, OQTR for Russian.

SMART PASTE (v3.5):
  Tools -> Paste Host, or Alt+V. Inserts the host
  clipboard for the current F4 language. Strips niqqud
  and Arabic harakat, folds smart quotes/dashes to ASCII.
  Use this instead of the emulator clipboard paste for
  Hebrew/Arabic/Russian. Needs the host helper running.

MOUSE (v3.7):
  Needs a DOS mouse driver (DOSBox/DOSBox-X provide one).
  Click a top menu title to open it, click an item to run
  it. Click in the text to move the cursor (works in RTL).
  Press and drag to select a block, then Copy/Cut from the
  Edit or Block menu. Roll the wheel to scroll. Without a
  driver the mouse is inert and the keyboard still works.

REFORMAT (v3.8):
  Block -> Reformat re-wraps text to the column-71 margin:
  short lines are joined, long lines re-split at word
  boundaries, blank lines kept as paragraph breaks. Acts on
  a marked block if one is set, else the whole document.
  Handy for converted QText files or pasted text whose
  lines run off-screen. Cannot be undone (asks to confirm).

SPELL CHECK (v3.9):
  Tools -> Spell Check follows the F4 language. English uses
  aspell; Hebrew uses hspell (or aspell's Hebrew dictionary).
  Misspelled words are flagged with suggestions; embedded
  English in a Hebrew document is left alone. Arabic/Russian
  have no bundled engine. Needs the host helper running.

COPY TO HOST (v3.9):
  Tools -> Copy to Host, or Alt+C. Sends the marked block
  (or the whole document) to the host clipboard as UTF-8, so
  Hebrew/Arabic/Russian text pastes into any host app. The
  outbound mirror of Smart Paste. Needs the host helper.

ENCRYPTION:
  File menu -> Save Encrypted -> enter password.
  Magic header OQT-ENC1 marks the file.
  Same password unlocks; works for all four scripts.

CONVERTERS:
  oqt2word file.txt out.txt   OpenQT  -> UTF-8
  oqt2word /R rus.txt out.txt CP866   -> UTF-8 (Russian)
  txt2oqt  utf8.txt  out.txt  UTF-8   -> OpenQT
  txt2oqt /R utf8.txt out.txt UTF-8   -> CP866 (Russian)
  oqt2qt   file.txt  out.qt   OpenQT  -> QText 5.5
  qt2oqt   file.qt   out.txt  QText   -> OpenQT

WHICH .HLP YOU ARE READING:
  OPENQT.HLP    Heb + Ara + Eng (the OQT 3 trilingual guide)
  OPENQTH.HLP   Heb + Eng (loaded automatically under OQTH)
  OPENQTA.HLP   Ara + Eng (loaded automatically under OQTA)
  OPENQTR.HLP   Rus + Eng (loaded automatically under OQTR)
"""

HEBREW_SECTION = """\

מדריך בעברית
------------

אופנקיו הוא מעבד תמלילים תלת לשוני
לדוס התומך באנגלית עברית וערבית
בו זמנית באותו מסך.
הקש F4 כדי לעבור בין שפות.

מקשים שימושיים:
  F2 שמירה            F3 פתיחה
  F4 החלפת שפה        F5 כיוון
  F7 חיפוש            F8 החלפה
  F9 הדבק             F10 אנגלית משובצת
  Ctrl+B מודגש        Ctrl+U קו תחתון
  Alt+X יציאה

תמיכה בכיוון טקסט, סופיות אוטומטיות,
חיפוש החלפה, ססמאות, בלוקים.

סידור טקסט מחדש לרוחב העמודה:
תפריט בלוק ואז Reformat.

לחזור למסמך הראשי הקש ESC.
"""

ARABIC_SECTION = """\

دليل بالعربية
-------------

اوبن كيو تي محرر نصوص ثلاثي اللغة
لنظام دوس يدعم الانجليزية والعبرية
والعربية على شاشة واحدة معا.
اضغط F4 لتبديل لغة الادخال.

مفاتيح مهمة:
  F2 حفظ              F3 فتح
  F4 تبديل اللغة      F5 اتجاه
  F7 بحث              F8 استبدال
  F9 لصق              F10 انجليزية مدمجة
  Ctrl+B عريض         Ctrl+U تسطير
  Alt+X خروج

تشكيل عربي كامل واتصال الحروف
دعم الارقام العربية والعلامات.

اعادة تنسيق الفقرات الى عرض العمود:
قائمة بلوك ثم Reformat.

اضغط ESC للعودة للمستند.
"""

RUSSIAN_SECTION = """\

РУКОВОДСТВО НА РУССКОМ
----------------------

ОпенКьюТи это текстовый редактор для ДОС
поддерживающий английский, иврит, арабский и
русский. Русский язык работает в отдельной сессии
запускаемой через ОКЬЮТР, потому что коды символов
КП866 совпадают с диапазонами иврита и арабского.

КЛАВИШИ:
  F2 Сохранить        F3 Открыть
  F4 Сменить язык     F5 Направление текста
  F7 Поиск            F8 Замена
  F9 Вставить блок    F10 Английский в РТЛ
  Ctrl+B Жирный       Ctrl+U Подчеркнутый
  Alt+X Выход

ЗАПУСК:
  ОКЬЮТР файл       Русский плюс английский (КП866)
  Команда openqt /R  стартовать сразу в русской сессии

КОНВЕРТЕРЫ:
  oqt2word /R вход.txt utf8.txt
  txt2oqt  /R utf8.txt вход.txt
Флаг /R обязателен для русских документов так как
байтовые диапазоны КП866 пересекаются с КП862 и КП864.

ПЕРЕФОРМАТИРОВАНИЕ:
  Меню Block, пункт Reformat - перенос строк по
  ширине столбца 71. Действует на блок или весь текст.

Нажмите ESC для возврата в документ.
"""

FOOTER = """\

============================================================
End of guide.   ESC closes.   See README.md for full details.
============================================================
"""


if __name__ == "__main__":
    import os
    base = os.path.dirname(os.path.abspath(__file__)) + '/'
    write_hlp(base + 'OPENQT.HLP',
              HEADER + HEBREW_SECTION + ARABIC_SECTION + FOOTER)
    write_hlp(base + 'OPENQTH.HLP',
              HEADER + HEBREW_SECTION + FOOTER)
    write_hlp(base + 'OPENQTA.HLP',
              HEADER + ARABIC_SECTION + FOOTER)
    write_hlp(base + 'OPENQTR.HLP',
              HEADER + RUSSIAN_SECTION + FOOTER)
