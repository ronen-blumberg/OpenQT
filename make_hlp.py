#!/usr/bin/env python3
"""Generate OPENQT.HLP — trilingual docs in OpenQT mixed CP862+CP864 byte format.

Hebrew U+05D0..U+05EA  -> CP862 0x80..0x9A.
Arabic basic block     -> CP864 abstract isolated-form bytes.
ASCII                  -> passthrough.
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


def encode(text: str) -> bytes:
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
            out.append(0xF9)  # any lam-alef ligature -> CP864 0xF9
        else:
            out.append(0x3F)  # '?'
    return bytes(out)


CONTENT = """\
============================================================
            OPENQT v3.2 - USER GUIDE
============================================================

ENGLISH GUIDE
-------------

OpenQT is a trilingual word processor for DOS that
supports English, Hebrew and Arabic on one screen
simultaneously. Press F4 to cycle between languages.

KEY COMMANDS:
  F1   Help            F2  Save           F3  Open
  F4   Switch lang     F5  RTL toggle     F7  Find
  F8   Replace         F9  Paste block    F10 Embed LTR
  Ctrl+B Bold          Ctrl+U Underline
  Ctrl+Z Undo          Ctrl+Y Redo
  Alt+X  Exit

LAUNCHERS:
  OQT 3 file   Trilingual (Hebrew + Arabic + English)
  OQTH  file   Hebrew + English only
  OQTA  file   Arabic + English only

ARABIC SHAPING (v3.2):
  Cursive joining is automatic. Letters connect to their
  neighbours when CP864 provides positional form bytes.
  All four forms (isolated/initial/medial/final) used
  where available.

ENCRYPTION:
  File menu -> Save Encrypted -> enter password.
  Magic header OQT-ENC1 marks the file.
  Same password unlocks; works for all three scripts.

CONVERTERS:
  oqt2word file.txt out.txt   OpenQT  -> UTF-8
  txt2oqt  utf8.txt  out.txt  UTF-8   -> OpenQT
  oqt2qt   file.txt  out.qt   OpenQT  -> QText 5.5
  qt2oqt   file.qt   out.txt  QText   -> OpenQT


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

לחזור למסמך הראשי הקש ESC.


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

اضغط ESC للعودة للمستند.


============================================================
End of guide.   ESC closes.   See README.md for full details.
============================================================
"""

if __name__ == "__main__":
    blob = encode(CONTENT)
    blob = blob.replace(b'\n', b'\r\n')
    with open('/home/ronen/dos/eini2/OPENQT/OPENQT.HLP', 'wb') as f:
        f.write(blob)
    print(f"Wrote OPENQT.HLP ({len(blob)} bytes)")
