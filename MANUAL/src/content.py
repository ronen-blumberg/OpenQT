# -*- coding: utf-8 -*-
"""Bilingual manual content for OpenQT. emit(doc, L) where L in {'en','he'}."""


def emit(doc, L):
    def t(en, he):
        return en if L == "en" else he
    P = doc.para
    B = doc.bullet

    # ============================================================ INTRO
    doc.h1(t("1. Introduction", "1. מבוא"))
    P(t("OpenQT is a full-screen word processor for DOS that writes Hebrew, "
        "Arabic, Russian and English. It is a modern clone of the classic "
        "QText 5.5, rebuilt in C with full bidirectional (right-to-left) "
        "support, inline formatting, undo/redo, password encryption, on-screen "
        "Arabic cursive shaping, and -- uniquely -- true trilingual editing in "
        "which Hebrew, Arabic and English all coexist on the same line of the "
        "same document, simultaneously visible on the VGA screen.",
        "OpenQT הוא מעבד תמלילים מסך-מלא ל-DOS הכותב עברית, ערבית, רוסית "
        "ואנגלית. זהו שכפול מודרני של QText 5.5 הקלאסי, שנכתב מחדש בשפת C עם "
        "תמיכה דו-כיוונית מלאה (ימין-לשמאל), עיצוב טקסט מוטמע, ביטול/שחזור, "
        "הצפנה בסיסמה, עיצוב ערבי מחובר על המסך, ובאופן ייחודי -- עריכה "
        "תלת-לשונית אמיתית שבה עברית, ערבית ואנגלית מתקיימות יחד באותה השורה "
        "ובאותו המסמך, וגלויות בו-זמנית על מסך ה-VGA."))
    P(t("The editor draws directly to VGA text-mode memory and uses BIOS "
        "interrupts for input, so it has no dependencies beyond plain DOS. It "
        "runs in real-mode 16-bit DOS or, preferably, 32-bit via the bundled "
        "DOS4GW extender. Inside DOSBox or DOSBox-X it runs on any modern "
        "Linux, Windows or macOS host.",
        "העורך כותב ישירות לזיכרון מסך הטקסט של VGA ומשתמש בפסיקות BIOS לקלט, "
        "ולכן אין לו תלויות מעבר ל-DOS עצמו. הוא פועל ב-DOS ריאלי 16-ביט או, "
        "באופן מועדף, ב-32-ביט דרך מרחיב ה-DOS4GW המצורף. בתוך DOSBox או "
        "DOSBox-X הוא פועל על כל מארח מודרני של Linux, Windows או macOS."))
    doc.note(t("Author: Ronen Blumberg.  License: Public Domain.  "
               "Version: OpenQT 3.9.2.",
               "מחבר: רונן בלומברג.  רישיון: נחלת הכלל.  גרסה: OpenQT 3.9.2."),
             "note")

    doc.h2(t("Key features", "תכונות עיקריות"))
    for en, he in [
        ("Full BiDi: Hebrew and Arabic right-to-left, English and digits "
         "left-to-right, freely interleaved.",
         "דו-כיווניות מלאה: עברית וערבית מימין לשמאל, אנגלית וספרות משמאל "
         "לימין, בשילוב חופשי."),
        ("Inline bold and underline formatting.",
         "עיצוב מוטמע של מודגש וקו-תחתי."),
        ("Up to 30,000 lines per document; undo/redo 250 levels deep.",
         "עד 30,000 שורות במסמך; ביטול/שחזור עד 250 רמות."),
        ("Search and search-and-replace across all scripts.",
         "חיפוש והחלפה בכל מערכות הכתב."),
        ("Block (range) operations: copy, cut, paste, delete, reformat.",
         "פעולות גוש (טווח): העתקה, גזירה, הדבקה, מחיקה, עיצוב מחדש."),
        ("Word wrap and paragraph reflow to column 71.",
         "גלישת מילים ועיצוב פסקאות מחדש לעמודה 71."),
        ("Password protection with the OQT-ENC1 encrypted format.",
         "הגנת סיסמה בפורמט המוצפן OQT-ENC1."),
        ("On-screen CP864 Arabic positional shaping.",
         "עיצוב ערבי מיקומי CP864 על המסך."),
        ("Mouse support (clickable menus, click-to-caret, drag-select, wheel).",
         "תמיכת עכבר (תפריטים לחיצים, מיקום סמן בלחיצה, בחירה בגרירה, גלגלת)."),
        ("Host-assisted tools: spell check, text-to-speech, dictation, "
         "machine translation and Unicode clipboard paste.",
         "כלים בעזרת המארח: בדיקת איות, הקראה, הכתבה, תרגום מכונה והדבקה "
         "מלוח-הגזירים ב-Unicode."),
    ]:
        B(t(en, he))

    # ============================================================ QUICK START
    doc.h1(t("2. Quick start", "2. התחלה מהירה"))
    P(t("The compiled editor and all tools ship ready to run. Mount the OpenQT "
        "folder as C:\\OPENQT inside DOSBox (or DOSBox-X):",
        "העורך המהודר וכל הכלים מצורפים מוכנים להרצה. עגנו את תיקיית OpenQT "
        "כ-C:\\OPENQT בתוך DOSBox (או DOSBox-X):"))
    doc.code("mount c /path/to/openqt\nc:\ncd \\OPENQT")
    P(t("Then launch the editor in the language combination you want:",
        "לאחר מכן הריצו את העורך בצירוף השפות הרצוי:"))
    doc.code("OQT 3 mydoc.txt    " + t("# Hebrew + Arabic + English (trilingual)", "# עברית + ערבית + אנגלית") + "\n"
             "OQTH mydoc.txt     " + t("# Hebrew + English", "# עברית + אנגלית") + "\n"
             "OQTA mydoc.txt     " + t("# Arabic + English", "# ערבית + אנגלית") + "\n"
             "OQTR mydoc.txt     " + t("# Russian + English (CP866)", "# רוסית + אנגלית"))
    P(t("The first few keys to know:",
        "המקשים הראשונים שכדאי להכיר:"))
    doc.table(
        [t("Key", "מקש"), t("Action", "פעולה")],
        [["F4", t("Cycle input language: English -> Hebrew -> Arabic -> Russian",
                  "מעבר בין שפות קלט: אנגלית ← עברית ← ערבית ← רוסית")],
         ["F5", t("Toggle right-to-left layout", "החלפת פריסת ימין-לשמאל")],
         ["F2", t("Save", "שמירה")],
         ["F3", t("Open another file", "פתיחת קובץ אחר")],
         ["F1", t("Quick help screen", "מסך עזרה מהיר")],
         ["Alt+H", t("Help menu -> Docs... opens the in-editor guide",
                     "תפריט עזרה ← Docs... פותח את המדריך בתוך העורך")],
         ["Alt+X", t("Exit", "יציאה")]],
        widths=[1, 3.2])

    # ============================================================ LAUNCHERS
    doc.h1(t("3. Launchers", "3. קובצי הפעלה"))
    P(t("Each launcher is a small BAT file that prepares the VGA "
        "character-generator font for its language set, then starts the "
        "editor with the right flags. Always launch through these rather than "
        "running OPENQT.EXE directly, so the screen font matches your text.",
        "כל קובץ הפעלה הוא קובץ BAT קטן שמכין את גופן מחולל-התווים של VGA "
        "עבור קבוצת השפות שלו, ואז מפעיל את העורך עם הדגלים הנכונים. הפעילו "
        "תמיד דרכם ולא ישירות דרך OPENQT.EXE, כדי שגופן המסך יתאים לטקסט."))
    doc.table(
        [t("Launcher", "קובץ הפעלה"), t("What it sets up", "מה הוא מכין")],
        [["OQT 3 file", t("Trilingual: HEBVGA, then ARABVGA /P overlay, then "
                          "OPENQT /A. Hebrew + Arabic + English on one screen.",
                          "תלת-לשוני: HEBVGA, ואז שכבת ARABVGA /P, ואז "
                          "OPENQT /A. עברית + ערבית + אנגלית במסך אחד.")],
         ["OQTH file", t("Hebrew + English. Runs HEBVGA, then OPENQT.",
                         "עברית + אנגלית. מריץ HEBVGA, ואז OPENQT.")],
         ["OQTA file", t("Arabic + English. Runs ARABVGA, then OPENQT /A.",
                         "ערבית + אנגלית. מריץ ARABVGA, ואז OPENQT /A.")],
         ["OQTR file", t("Russian + English (CP866). Runs RUSVGA, then "
                         "OPENQT /R.",
                         "רוסית + אנגלית (CP866). מריץ RUSVGA, ואז OPENQT /R.")],
         ["OQT file", t("Default -- same as OQTH (Hebrew + English).",
                        "ברירת מחדל -- כמו OQTH (עברית + אנגלית).")]],
        widths=[1.1, 3])
    doc.note(t("The /A flag tells OpenQT that Arabic glyphs occupy the "
               "0xC0-0xDF byte range where CP437 keeps its box-drawing "
               "characters, so it draws dialog frames with plain ASCII "
               "(+ - |) instead. Russian (/R) keeps CP437 box-drawing intact "
               "and needs no such fallback.",
               "הדגל A/ מודיע ל-OpenQT שאותיות ערביות תופסות את טווח הבייטים "
               "0xC0-0xDF שבו CP437 מחזיק את תווי המסגרת, ולכן הוא מצייר מסגרות "
               "תיבות-דו-שיח בתווי ASCII רגילים (+ - |). רוסית (R/) משאירה את "
               "תווי המסגרת של CP437 ואינה זקוקה לנפילה-לאחור כזו."), "note")

    # ============================================================ KEYBOARD REF
    doc.h1(t("4. Keyboard reference", "4. מדריך מקלדת"))

    doc.h2(t("Files and editing", "קבצים ועריכה"))
    doc.table([t("Key", "מקש"), t("Action", "פעולה")],
              [["F1", t("Help screen", "מסך עזרה")],
               ["F2", t("Save current file", "שמירת הקובץ הנוכחי")],
               ["F3", t("Open another file (file dialog)", "פתיחת קובץ אחר")],
               ["Ctrl+S", t("Save without a dialog", "שמירה ללא דו-שיח")],
               ["Ctrl+N", t("New document", "מסמך חדש")],
               ["Ctrl+Z", t("Undo", "ביטול")],
               ["Ctrl+Y", t("Redo", "שחזור")],
               ["Insert", t("Toggle insert / overwrite", "החלפת הוספה/דריסה")],
               ["Delete", t("Delete character under cursor", "מחיקת התו שמתחת לסמן")],
               ["Backspace", t("Delete character to the left", "מחיקת התו משמאל")]],
              widths=[1, 3])

    doc.h2(t("Cursor movement", "תנועת הסמן"))
    doc.table([t("Key", "מקש"), t("Action", "פעולה")],
              [[t("Arrow keys", "מקשי חצים"), t("Move one character / one line",
                                                "תנועה תו אחד / שורה אחת")],
               ["Ctrl+Left / Ctrl+Right", t("Move five characters at a time",
                                            "תנועה של חמישה תווים")],
               ["Home / End", t("Start / end of line", "תחילת / סוף שורה")],
               ["PgUp / PgDn", t("Page up / down", "עמוד מעלה / מטה")],
               ["Ctrl+Home / Ctrl+End", t("Start / end of document",
                                          "תחילת / סוף המסמך")],
               ["Ctrl+G", t("Go to line number", "מעבר למספר שורה")]],
              widths=[1.5, 2.5])

    doc.h2(t("Language and direction", "שפה וכיוון"))
    doc.table([t("Key", "מקש"), t("Action", "פעולה")],
              [["F4", t("Cycle input language: English -> Hebrew -> Arabic -> "
                        "Russian (auto-flips RTL/LTR to match)",
                        "מעבר בין שפות קלט: אנגלית ← עברית ← ערבית ← רוסית "
                        "(מהפך אוטומטית בין ימין-לשמאל לשמאל-לימין)")],
               ["F5", t("Toggle right-to-left paragraph direction",
                        "החלפת כיוון פסקה ימין-לשמאל")],
               ["F10", t("Toggle 'embedded LTR' -- type English/digits inline "
                         "inside an RTL paragraph without changing language",
                         "החלפת 'שמאל-לימין מוטמע' -- הקלדת אנגלית/ספרות בתוך "
                         "פסקה ימנית בלי לשנות שפה")]],
              widths=[1, 3])

    doc.h2(t("Search, formatting, blocks", "חיפוש, עיצוב, גושים"))
    doc.table([t("Key", "מקש"), t("Action", "פעולה")],
              [["F7", t("Find", "חיפוש")],
               ["F8", t("Replace", "החלפה")],
               ["Ctrl+B", t("Toggle bold for the next typed text",
                            "החלפת מודגש לטקסט הבא")],
               ["Ctrl+U", t("Toggle underline", "החלפת קו-תחתי")],
               ["Alt+U", t("Toggle bold + underline together",
                           "החלפת מודגש + קו-תחתי יחד")],
               ["F9", t("Paste block at cursor", "הדבקת גוש במיקום הסמן")],
               ["Alt+V", t("Paste host clipboard (via helper)",
                           "הדבקת לוח-הגזירים של המארח (דרך העוזר)")],
               ["Alt+C", t("Copy selection/document to host clipboard",
                           "העתקת הבחירה/המסמך ללוח-הגזירים של המארח")],
               ["Alt+B", t("Open Block menu (start/end/copy/cut/paste/"
                           "delete/reformat)",
                           "פתיחת תפריט גוש (התחלה/סיום/העתקה/גזירה/"
                           "הדבקה/מחיקה/עיצוב-מחדש)")]],
              widths=[1.1, 3])

    doc.h2(t("Menus", "תפריטים"))
    doc.table([t("Key", "מקש"), t("Menu", "תפריט")],
              [["Alt+F", t("File", "קובץ")],
               ["Alt+E", t("Edit", "עריכה")],
               ["Alt+S", t("Search", "חיפוש")],
               ["Alt+B", t("Block", "גוש")],
               ["Alt+O", t("Options", "אפשרויות")],
               ["Alt+T", t("Tools (host-assisted features)", "כלים (תכונות המארח)")],
               ["Alt+H", t("Help (Help / Docs... / About...)",
                           "עזרה (עזרה / Docs... / אודות...)")],
               ["Alt+X", t("Exit", "יציאה")]],
              widths=[1, 3])

    # ============================================================ TYPING LAYOUTS
    doc.h1(t("5. Typing layouts", "5. פריסות הקלדה"))
    P(t("OpenQT maps incoming ASCII keystrokes to letters itself, so set your "
        "DOSBox keyboard to a plain US layout. F4 chooses which script the "
        "next keystrokes produce.",
        "OpenQT ממפה בעצמו את הקשות ה-ASCII הנכנסות לאותיות, לכן הגדירו את "
        "מקלדת DOSBox לפריסת US רגילה. מקש F4 בוחר באיזו כתב יפיקו ההקשות הבאות."))

    doc.h2(t("Hebrew (standard Israeli layout)", "עברית (פריסה ישראלית תקנית)"))
    doc.table([t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות")],
              [["t", "א", "c", "ב", "d", "ג"],
               ["s", "ד", "v", "ה", "u", "ו"],
               ["z", "ז", "j", "ח", "y", "ט"],
               ["h", "י", "l", "כ ך", "f", "ל"],
               ["k", "מ ם", "n", "נ ן", "b", "ס"],
               ["g", "ע", ";", "פ ף", "m", "צ ץ"],
               ["e", "ק", "r", "ר", "a", "ש"],
               [",", "ת", "", "", "", ""]],
              widths=[1, 1.4, 1, 1.4, 1, 1.4], size=9)
    P(t("Final-letter (sofit) substitution is automatic at the end of a word.",
        "החלפת אות סופית מתבצעת אוטומטית בסוף מילה."), size=9)

    doc.h2(t("Arabic (Microsoft Arabic 101 layout)", "ערבית (פריסת Microsoft Arabic 101)"))
    doc.table([t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות")],
              [["a", "ش", "b", "لآ", "c", "ؤ"],
               ["d", "ي", "e", "ث", "f", "ب"],
               ["g", "ل", "h", "ا", "i", "ه"],
               ["j", "ت", "k", "ن", "l", "م"],
               ["m", "ة", "n", "ى", "o", "خ"],
               ["p", "ح", "q", "ض", "r", "ق"],
               ["s", "س", "t", "ف", "u", "ع"],
               ["v", "ر", "w", "ص", "x", "ء"],
               ["y", "غ", "z", "ئ", "[", "ج"],
               ["]", "د", "`", "ذ", "", ""]],
              widths=[1, 1.4, 1, 1.4, 1, 1.4], size=9)
    P(t("Digits 0-9 give Arabic-Indic numerals. Punctuation keys map to extra "
        "letters: ' -> Tah, , -> Waw, ; -> Kaf, . -> Zain, / -> Zah.",
        "הספרות 0-9 מפיקות ספרות ערביות-הודיות. מקשי הפיסוק ממופים לאותיות "
        "נוספות: ' ← טא, , ← ואו, ; ← כּאף, . ← זאי, / ← טַ׳א."), size=9)

    doc.h2(t("Russian (JCUKEN layout)", "רוסית (פריסת JCUKEN)"))
    doc.table([t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות"),
               t("Key", "מקש"), t("Letter", "אות")],
              [["q", "й", "w", "ц", "e", "у"],
               ["r", "к", "t", "е", "y", "н"],
               ["u", "г", "i", "ш", "o", "щ"],
               ["p", "з", "[", "х", "]", "ъ"],
               ["a", "ф", "s", "ы", "d", "в"],
               ["f", "а", "g", "п", "h", "р"],
               ["j", "о", "k", "л", "l", "д"],
               [";", "ж", "'", "э", "z", "я"],
               ["x", "ч", "c", "с", "v", "м"],
               ["b", "и", "n", "т", "m", "ь"],
               [",", "б", ".", "ю", "`", "ё"]],
              widths=[1, 1.4, 1, 1.4, 1, 1.4], size=9)
    P(t("Russian has letter case (Shift gives uppercase). Digits 0-9 pass "
        "through; / gives a period and ? a comma (JCUKEN swap).",
        "ברוסית יש אותיות גדולות וקטנות (Shift נותן אות גדולה). הספרות 0-9 "
        "עוברות כמות שהן; / נותן נקודה ו-? פסיק (החלפת JCUKEN)."), size=9)

    # ============================================================ TRILINGUAL
    doc.h1(t("6. Trilingual mode", "6. מצב תלת-לשוני"))
    P(t("OQT 3 lets Hebrew, Arabic and English share the same line of the same "
        "document, all visible at once. The trick is byte ranges: the CP862 "
        "Hebrew font puts letters at 0x80-0x9A while the CP864 Arabic font puts "
        "letters at 0xA0-0xFE. These ranges do not overlap, so both fonts can "
        "live in the VGA character generator at the same time.",
        "מצב OQT 3 מאפשר לעברית, ערבית ואנגלית לחלוק את אותה השורה ואותו "
        "המסמך, וכולן גלויות בו-זמנית. הטריק הוא טווחי הבייטים: גופן CP862 "
        "העברי ממקם אותיות ב-0x80-0x9A בעוד גופן CP864 הערבי ממקם אותיות "
        "ב-0xA0-0xFE. הטווחים אינם חופפים, ולכן שני הגופנים יכולים לחיות יחד "
        "במחולל-התווים של VGA."))
    P(t("The launcher loads a full CP862 base font with HEBVGA, then overlays "
        "only the 95 Arabic slots with ARABVGA /P. The result is a single "
        "screen font carrying all three scripts. Press F4 to choose which "
        "range your keystrokes land in; the BiDi engine arranges direction "
        "automatically.",
        "קובץ ההפעלה טוען גופן בסיס CP862 מלא עם HEBVGA, ואז מכסה רק את 95 "
        "המשבצות הערביות עם ARABVGA /P. התוצאה היא גופן מסך יחיד הנושא את שלוש "
        "מערכות הכתב. הקישו F4 כדי לבחור לאיזה טווח ייכנסו ההקשות; מנוע "
        "הדו-כיווניות מסדר את הכיוון אוטומטית."))
    doc.code("F4 -> English   type:  See the word\n"
             "F4 -> Hebrew    type:  שלום\n"
             "F4 -> Arabic    type:  مرحبا\n"
             "F4 -> English   type:  in three scripts.")
    doc.note(t("A saved trilingual file is a mixed CP862+CP864 byte stream -- "
               "not a standard codepage. To share it outside OpenQT, export to "
               "UTF-8 with oqt2word (see chapter 13).",
               "קובץ תלת-לשוני שמור הוא זרם בייטים מעורב CP862+CP864 -- לא "
               "קודפייג' תקני. כדי לשתף אותו מחוץ ל-OpenQT, ייצאו ל-UTF-8 עם "
               "oqt2word (ראו פרק 13)."), "note")

    # ============================================================ RUSSIAN
    doc.h1(t("7. Russian mode", "7. מצב רוסית"))
    P(t("Russian (CP866) runs as its own session -- English + Russian only. "
        "It cannot mix with Hebrew or Arabic because CP866 Cyrillic overlaps "
        "both the CP862 Hebrew range (0x80-0x9F) and the CP864 Arabic range "
        "(0xA0-0xAF, 0xE0-0xEF). The same byte slot would have to be a Hebrew, "
        "an Arabic and a Cyrillic letter at once, which 8-bit storage cannot "
        "express.",
        "רוסית (CP866) פועלת כמושב נפרד -- אנגלית + רוסית בלבד. היא אינה יכולה "
        "להתערבב עם עברית או ערבית מפני שהקירילית של CP866 חופפת גם לטווח "
        "העברי CP862 (0x80-0x9F) וגם לטווח הערבי CP864 (0xA0-0xAF, 0xE0-0xEF). "
        "אותה משבצת בייט הייתה צריכה להיות אות עברית, ערבית וקירילית בו-זמנית, "
        "וזאת אחסון 8-ביט אינו יכול לבטא."))
    P(t("Launch with OQTR. F4 still cycles through all four languages, and the "
        "direction auto-flips: RTL for Hebrew/Arabic, LTR for English/Russian. "
        "Everything else -- bold, underline, undo, search, blocks, encryption "
        "-- behaves exactly as in English mode. Because CP866 keeps CP437 "
        "box-drawing intact, dialogs render normally with no /A fallback.",
        "הפעילו עם OQTR. מקש F4 עדיין עובר בין כל ארבע השפות, והכיוון מתהפך "
        "אוטומטית: ימין-לשמאל לעברית/ערבית, שמאל-לימין לאנגלית/רוסית. כל השאר "
        "-- מודגש, קו-תחתי, ביטול, חיפוש, גושים, הצפנה -- מתנהג בדיוק כמו במצב "
        "אנגלית. כיוון ש-CP866 שומר על תווי המסגרת של CP437, תיבות הדו-שיח "
        "מוצגות כרגיל ללא נפילת A/."))
    doc.note(t("A document written in Russian mode is a standard CP866 file. "
               "If reopened in Hebrew/Arabic mode its letters render as the "
               "wrong script -- the 8-bit storage carries no language tag. "
               "Use the converter /R flag to move Russian text to/from UTF-8.",
               "מסמך שנכתב במצב רוסית הוא קובץ CP866 תקני. אם ייפתח מחדש במצב "
               "עברית/ערבית אותיותיו יוצגו ככתב שגוי -- האחסון של 8-ביט אינו "
               "נושא תג שפה. השתמשו בדגל R/ של הממירים כדי להעביר טקסט רוסי "
               "אל/מן UTF-8."), "warn")

    # ============================================================ BIDI
    doc.h1(t("8. BiDi and Arabic shaping", "8. דו-כיווניות ועיצוב ערבי"))
    P(t("The file always stores text in logical order -- the order you typed "
        "the letters. At display time OpenQT reorders the bytes to their "
        "visual position: it strips inline format codes, reverses the line "
        "when the paragraph is RTL, then re-reverses each English/digit run so "
        "numbers and Latin words read left-to-right within the larger RTL "
        "flow. The cursor maps keep the caret correct even when visual order "
        "differs from typing order.",
        "הקובץ תמיד מאחסן טקסט בסדר לוגי -- הסדר שבו הקלדתם את האותיות. בעת "
        "התצוגה OpenQT מסדר מחדש את הבייטים למיקומם החזותי: הוא מסיר קודי "
        "עיצוב מוטמעים, הופך את השורה כשהפסקה ימנית, ואז הופך מחדש כל רצף של "
        "אנגלית/ספרות כך שמספרים ומילים לטיניות נקראים משמאל לימין בתוך הזרימה "
        "הימנית הכוללת. מפות הסמן שומרות על מיקום נכון גם כשהסדר החזותי שונה "
        "מסדר ההקלדה."))
    doc.h2(t("Arabic positional shaping", "עיצוב ערבי מיקומי"))
    P(t("OpenQT shapes Arabic cursively on the DOS screen: each letter is "
        "drawn in its correct positional form (isolated, initial, medial, "
        "final) based on its neighbours, so the text joins like real Arabic. "
        "This is display-only -- the file always stores the abstract isolated "
        "form, so saving, loading and the converters are unaffected.",
        "OpenQT מעצב ערבית בצורה מחוברת על מסך ה-DOS: כל אות מצוירת בצורתה "
        "המיקומית הנכונה (מבודדת, תחילית, אמצעית, סופית) לפי שכנותיה, כך שהטקסט "
        "מתחבר כמו ערבית אמיתית. זוהי תצוגה בלבד -- הקובץ תמיד מאחסן את הצורה "
        "המבודדת המופשטת, ולכן השמירה, הטעינה והממירים אינם מושפעים."))
    doc.note(t("A few CP864 letters (Tah, Zah, Yeh-Hamza, and plain Lam-Alef "
               "without madda) lack separate positional glyphs and appear in "
               "their isolated shape inside a join -- a small on-screen "
               "imperfection. Exported UTF-8 is always correct because the "
               "receiving program does its own shaping.",
               "מספר אותיות CP864 (טַא, טַ׳א, יא-המזה, ולאם-אליף פשוט ללא "
               "מדה) חסרות צורות מיקומיות נפרדות ומופיעות בצורתן המבודדת בתוך "
               "חיבור -- פגם קטן על המסך. ה-UTF-8 המיוצא תמיד תקין כי התוכנית "
               "המקבלת מבצעת עיצוב משלה."), "note")

    # ============================================================ FORMATTING
    doc.h1(t("9. Formatting, blocks and reflow", "9. עיצוב, גושים ועיצוב מחדש"))
    doc.h2(t("Bold and underline", "מודגש וקו-תחתי"))
    P(t("Press Ctrl+B to toggle bold and Ctrl+U to toggle underline for the "
        "text you type next; Alt+U toggles both together. Formatting is stored "
        "as in-band toggle bytes inside the line and resets at the start of "
        "each line.",
        "הקישו Ctrl+B כדי להחליף מודגש ו-Ctrl+U כדי להחליף קו-תחתי עבור הטקסט "
        "שתקלידו בהמשך; Alt+U מחליף את שניהם יחד. העיצוב מאוחסן כבייטי-מתג "
        "בתוך השורה ומתאפס בתחילת כל שורה."))
    doc.h2(t("Block operations", "פעולות גוש"))
    P(t("Open the Block menu with Alt+B. Mark a block (start, move, end), then "
        "copy, cut, paste (also F9), or delete it. Blocks use the same "
        "coordinates as mouse drag-selection, so the two are interchangeable.",
        "פתחו את תפריט הגוש עם Alt+B. סמנו גוש (התחלה, תנועה, סיום), ואז "
        "העתיקו, גזרו, הדביקו (גם F9), או מחקו אותו. הגושים משתמשים באותן "
        "קואורדינטות כמו בחירת-גרירה בעכבר, כך שהשניים מתחלפים זה בזה."))
    doc.h2(t("Reformat (reflow)", "עיצוב מחדש (זרימה מחדש)"))
    P(t("Block -> Reformat re-wraps text to the column-71 margin: short lines "
        "are joined and long lines are re-split at word boundaries, while blank "
        "lines are preserved as paragraph breaks. With nothing marked it "
        "reflows the whole document; with a block marked it reflows only those "
        "lines. It works on logical-order bytes (BiDi-safe) and keeps bold/"
        "underline travelling with their words.",
        "Block -> Reformat עוטף מחדש טקסט לשוליים בעמודה 71: שורות קצרות "
        "מחוברות ושורות ארוכות מפוצלות מחדש בגבולות מילים, בעוד שורות ריקות "
        "נשמרות כמעברי פסקה. ללא סימון הוא מעצב מחדש את כל המסמך; עם גוש מסומן "
        "הוא מעצב מחדש רק את אותן שורות. הוא פועל על בייטים בסדר לוגי (בטוח "
        "לדו-כיווניות) ושומר על המודגש/קו-תחתי נעים עם מילותיהם."))
    doc.note(t("Reformat rebuilds the buffer in one pass and cannot be undone. "
               "It asks you to confirm first and clears the undo history -- "
               "save before running it if you want a fallback.",
               "Reformat בונה מחדש את החוצץ במעבר אחד ואינו ניתן לביטול. הוא "
               "מבקש אישור תחילה ומנקה את היסטוריית הביטול -- שמרו לפני ההרצה "
               "אם תרצו אפשרות חזרה."), "warn")

    # ============================================================ MOUSE
    doc.h1(t("10. Mouse", "10. עכבר"))
    P(t("If a DOS mouse driver is present (DOSBox and DOSBox-X provide one), "
        "OpenQT becomes clickable. With no driver the mouse is simply inert "
        "and every keyboard command still works.",
        "אם קיים מנהל עכבר ל-DOS (DOSBox ו-DOSBox-X מספקים כזה), OpenQT הופך "
        "ללחיץ. ללא מנהל העכבר פשוט אינו פעיל וכל פקודות המקלדת עדיין פועלות."))
    doc.table([t("Action", "פעולה"), t("Result", "תוצאה")],
              [[t("Click a top-bar title", "לחיצה על כותרת בסרגל העליון"),
                t("Open that menu", "פתיחת אותו תפריט")],
               [t("Click a menu item", "לחיצה על פריט בתפריט"),
                t("Run it", "הפעלתו")],
               [t("Click in the text", "לחיצה בטקסט"),
                t("Move the cursor there (LTR and RTL)",
                  "העברת הסמן לשם (שמאל-לימין וימין-לשמאל)")],
               [t("Press and drag", "לחיצה וגרירה"),
                t("Select a block", "בחירת גוש")],
               [t("Drag past top/bottom", "גרירה מעבר לקצה עליון/תחתון"),
                t("Auto-scroll while selecting", "גלילה אוטומטית תוך בחירה")],
               [t("Mouse wheel", "גלגלת העכבר"),
                t("Scroll (three lines per notch)", "גלילה (שלוש שורות לתנועה)")]],
              widths=[1.4, 2.6])

    # ============================================================ DOCS VIEWER
    doc.h1(t("11. In-editor Docs viewer", "11. מציג ה-Docs שבעורך"))
    P(t("Alt+H -> Docs... opens a paged, read-only guide over the editor "
        "without disturbing your document (Esc returns exactly where you "
        "were). The viewer loads one of four .HLP files chosen by the current "
        "input language, so the loaded VGA font always matches the file's "
        "content:",
        "Alt+H -> Docs... פותח מדריך מעומד לקריאה-בלבד מעל העורך מבלי להפריע "
        "למסמך (Esc מחזיר בדיוק למקום שבו הייתם). המציג טוען אחד מארבעה קובצי "
        ".HLP הנבחר לפי שפת הקלט הנוכחית, כך שגופן ה-VGA הטעון תמיד תואם לתוכן "
        "הקובץ:"))
    doc.table([t("Active language", "שפה פעילה"), t("File loaded", "קובץ נטען")],
              [[t("Hebrew", "עברית"), "OPENQTH.HLP " + t("(Hebrew + English)", "(עברית + אנגלית)")],
               [t("Arabic", "ערבית"), "OPENQTA.HLP " + t("(Arabic + English)", "(ערבית + אנגלית)")],
               [t("Russian", "רוסית"), "OPENQTR.HLP " + t("(Russian + English)", "(רוסית + אנגלית)")],
               [t("English / default", "אנגלית / ברירת מחדל"),
                "OPENQT.HLP " + t("(trilingual)", "(תלת-לשוני)")]],
              widths=[1.4, 2.6])
    P(t("Each line is rendered through the same BiDi and Arabic-shaping "
        "pipeline as the editor. Page with Up/Down, PgUp/PgDn, Home/End; close "
        "with Esc or Q.",
        "כל שורה מוצגת דרך אותו צינור דו-כיווניות ועיצוב-ערבי כמו העורך. דפדפו "
        "עם מעלה/מטה, PgUp/PgDn, Home/End; סגרו עם Esc או Q."))

    # ============================================================ HOST TOOLS
    doc.h1(t("12. Host-assisted tools", "12. כלים בעזרת המארח"))
    P(t("The Tools menu (Alt+T) hands work to a small helper daemon running on "
        "the host (Linux, Windows or macOS), because DOS has no microphone, no "
        "speech engine, no modern network, and no UTF-8 clipboard. Start the "
        "helper on the host first (host_helper/run_helper.sh); if it is not "
        "running these items simply report 'Helper not responding' and the rest "
        "of the editor is unaffected.",
        "תפריט הכלים (Alt+T) מעביר עבודה ל-daemon עוזר קטן הפועל על המארח "
        "(Linux, Windows או macOS), מפני של-DOS אין מיקרופון, אין מנוע דיבור, "
        "אין רשת מודרנית, ואין לוח-גזירים ב-UTF-8. הפעילו תחילה את העוזר על "
        "המארח (host_helper/run_helper.sh); אם אינו פועל, פריטים אלה פשוט "
        "ידווחו 'Helper not responding' ושאר העורך אינו מושפע."))
    doc.table([t("Tools item", "פריט בכלים"), t("What it does", "מה הוא עושה"),
               t("Engine", "מנוע")],
              [[t("Spell Check", "בדיקת איות"),
                t("Flags unknown words with numbered suggestions; follows the "
                  "F4 language (English/Hebrew)",
                  "מסמן מילים לא מוכרות עם הצעות ממוספרות; עוקב אחר שפת F4 "
                  "(אנגלית/עברית)"),
                t("aspell/hspell", "aspell/hspell")],
               [t("Read Aloud", "הקראה"),
                t("Speaks the selection or document; voice follows F4 language",
                  "מקריא את הבחירה או המסמך; הקול עוקב אחר שפת F4"), "espeak-ng"],
               [t("Translate to Hebrew", "תרגום לעברית"),
                t("Translates English and inserts Hebrew (needs internet)",
                  "מתרגם אנגלית ומכניס עברית (דורש אינטרנט)"),
                t("online MT", "MT מקוון")],
               [t("Dictate", "הכתבה"),
                t("Records the mic and inserts recognised text (Eng/Heb)",
                  "מקליט מהמיקרופון ומכניס טקסט מזוהה (אנג'/עב')"), "whisper"],
               [t("Paste Host", "הדבקה מהמארח"),
                t("Inserts host clipboard as clean text for the F4 language",
                  "מכניס את לוח-הגזירים של המארח כטקסט נקי לשפת F4"), "xclip"],
               [t("Copy to Host", "העתקה למארח"),
                t("Sends the selection or document to the host clipboard as "
                  "UTF-8 (Alt+C)",
                  "שולח את הבחירה או המסמך ללוח-הגזירים של המארח כ-UTF-8 "
                  "(Alt+C)"), "xclip"]],
              widths=[1.2, 2.6, 1])
    doc.note(t("Paste Host (also Alt+V) strips niqqud and harakat and folds "
               "smart quotes and dashes to ASCII -- use it instead of "
               "DOSBox-X's Ctrl+F6, which mangles RTL text by re-mapping it "
               "through the DOS keyboard layout.",
               "הדבקה מהמארח (גם Alt+V) מסירה ניקוד וחרכאת ומקפלת מרכאות "
               "וקווים חכמים ל-ASCII -- השתמשו בה במקום Ctrl+F6 של DOSBox-X, "
               "שמשבש טקסט ימני על ידי מיפויו מחדש דרך פריסת מקלדת ה-DOS."), "tip")

    # ============================================================ FILE FORMAT
    doc.h1(t("13. File format and encryption", "13. פורמט קובץ והצפנה"))
    P(t("OpenQT files are plain DOS text files (CR/LF line endings) in a mixed "
        "byte encoding. Each byte range maps to one script:",
        "קובצי OpenQT הם קובצי טקסט רגילים של DOS (סופי שורה CR/LF) בקידוד "
        "בייטים מעורב. כל טווח בייטים ממפה למערכת כתב אחת:"))
    doc.table([t("Byte range", "טווח בייטים"), t("Meaning", "משמעות")],
              [["0x00-0x7F", t("ASCII (English, digits, punctuation)",
                               "ASCII (אנגלית, ספרות, פיסוק)")],
               ["0x80-0x9A", t("Hebrew letters (CP862)", "אותיות עבריות (CP862)")],
               ["0xA0-0xFE", t("Arabic letters (CP864 isolated form)",
                               "אותיות ערביות (צורת CP864 מבודדת)")],
               ["0x02 / 0x03 / 0x04", t("Bold / underline / both toggles",
                                        "מתגי מודגש / קו-תחתי / שניהם")],
               ["0xAF / 0xAE", t("QText 5.5 markers (recognised on read)",
                                 "סמני QText 5.5 (מזוהים בקריאה)")]],
              widths=[1.3, 2.7])
    P(t("Russian documents are instead standard CP866 byte streams; their "
        "ranges overlap Hebrew and Arabic, which is why CP866 cannot share a "
        "file with them.",
        "מסמכים רוסיים הם במקום זאת זרמי בייטים תקניים של CP866; הטווחים שלהם "
        "חופפים לעברית ולערבית, ולכן CP866 אינו יכול לחלוק עמם קובץ."))
    doc.h2(t("Encryption", "הצפנה"))
    P(t("Saving with a password prefixes the file with the magic header "
        "OQT-ENC1 and encrypts the rest byte-by-byte; OpenQT detects the "
        "header on load and prompts for the password. The cipher is a "
        "deterrent against casual reading, not serious cryptography -- do not "
        "rely on it for truly sensitive material. The external converters do "
        "not read encrypted files; open in OpenQT, save unencrypted, then "
        "convert.",
        "שמירה עם סיסמה מקדימה לקובץ את הכותרת המזהה OQT-ENC1 ומצפינה את "
        "השאר בייט-אחר-בייט; OpenQT מזהה את הכותרת בטעינה ומבקש את הסיסמה. "
        "הצופן הוא הרתעה מפני קריאה מזדמנת, לא קריפטוגרפיה רצינית -- אל "
        "תסתמכו עליו לחומר רגיש באמת. הממירים החיצוניים אינם קוראים קבצים "
        "מוצפנים; פתחו ב-OpenQT, שמרו ללא הצפנה, ואז המירו."), )

    # ============================================================ CONVERTERS
    doc.h1(t("14. Converters", "14. ממירים"))
    P(t("Five command-line tools move documents in and out of OpenQT's format. "
        "All are 32-bit DOS4GW builds run from the DOS prompt.",
        "חמישה כלי שורת-פקודה מעבירים מסמכים אל פורמט OpenQT וממנו. כולם בנויות "
        "ב-32-ביט DOS4GW ומורצות משורת הפקודה של DOS."))
    doc.table([t("Tool", "כלי"), t("Direction", "כיוון")],
              [["oqt2word", t("OpenQT -> UTF-8 (for Word / LibreOffice)",
                              "OpenQT ← UTF-8 (ל-Word / LibreOffice)")],
               ["txt2oqt", t("UTF-8 or legacy DOS -> OpenQT",
                             "UTF-8 או DOS ישן ← OpenQT")],
               ["txt2rtf", t("OpenQT (Heb+Eng) -> RTF, keeps bold/underline",
                             "OpenQT (עב+אנג) ← RTF, שומר מודגש/קו-תחתי")],
               ["oqt2qt", t("OpenQT -> QText 5.5", "OpenQT ← QText 5.5")],
               ["qt2oqt", t("QText 5.5 -> OpenQT (visual -> logical)",
                            "QText 5.5 ← OpenQT (חזותי ← לוגי)")]],
              widths=[1.1, 3])
    doc.h2("oqt2word")
    doc.code("oqt2word [/R] <input.openqt> <output.utf8>")
    P(t("Maps Hebrew to U+05D0-U+05EA, Arabic to the basic block U+0621-U+064A "
        "(so the receiving program shapes the letters itself), Arabic-Indic "
        "digits to U+0660-U+0669, and writes a UTF-8 BOM. Trilingual files "
        "just work -- every byte is classified by range. Add /R for documents "
        "written in Russian mode, which routes every high byte through the "
        "CP866 table.",
        "ממפה עברית ל-U+05D0-U+05EA, ערבית לבלוק הבסיסי U+0621-U+064A (כך "
        "שהתוכנית המקבלת מעצבת את האותיות בעצמה), ספרות ערביות-הודיות "
        "ל-U+0660-U+0669, וכותב BOM של UTF-8. קבצים תלת-לשוניים פשוט עובדים -- "
        "כל בייט מסווג לפי טווח. הוסיפו R/ למסמכים שנכתבו במצב רוסית, שמנתב כל "
        "בייט גבוה דרך טבלת CP866."))
    doc.h2("txt2oqt")
    doc.code("txt2oqt <input.txt> <output.openqt> [/V] [/D] [/U] [/R]")
    P(t("The inverse of oqt2word. By default it auto-detects UTF-8 via the "
        "BOM, reverses the mapping, and converts **bold** / __underline__ "
        "markdown markers into format toggles. Flags: /D forces legacy DOS "
        "mode, /U forces UTF-8, /V keeps visual order (legacy only), /R targets "
        "Russian CP866.",
        "ההפך מ-oqt2word. כברירת מחדל הוא מזהה אוטומטית UTF-8 דרך ה-BOM, הופך "
        "את המיפוי, וממיר סמני markdown מסוג **bold** / __underline__ למתגי "
        "עיצוב. דגלים: D/ כופה מצב DOS ישן, U/ כופה UTF-8, V/ שומר סדר חזותי "
        "(ישן בלבד), R/ מכוון לרוסית CP866."))
    doc.h2("txt2rtf")
    doc.code("txt2rtf <input.openqt> <output.rtf>")
    P(t("Exports a Hebrew + English document to RTF, preserving bold, "
        "underline and bold+underline so the styled text opens correctly in "
        "Word, LibreOffice and WordPad. Hebrew is emitted as Unicode with "
        "right-to-left runs (the renderer does its own bidi). Arabic and "
        "Russian are out of scope -- use oqt2word for those. Encrypted files "
        "are rejected with guidance.",
        "מייצא מסמך עברית + אנגלית ל-RTF, תוך שמירת מודגש, קו-תחתי ושניהם כך "
        "שהטקסט המעוצב נפתח נכון ב-Word, LibreOffice ו-WordPad. עברית נפלטת "
        "כ-Unicode עם רצפים ימניים (המעבד מבצע דו-כיווניות משלו). ערבית ורוסית "
        "מחוץ לתחום -- השתמשו ב-oqt2word עבורן. קבצים מוצפנים נדחים עם הנחיה."))

    # ============================================================ ROUND TRIP
    doc.h1(t("15. Round-trip workflow", "15. תהליך עבודה הלוך-ושוב"))
    P(t("The intended workflow for editing OpenQT documents in modern tools "
        "and bringing them back:",
        "תהליך העבודה המיועד לעריכת מסמכי OpenQT בכלים מודרניים והחזרתם:"))
    for i, (en, he) in enumerate([
        ("Type and save your document in OpenQT (F2).",
         "הקלידו ושמרו את המסמך ב-OpenQT (F2)."),
        ("Run oqt2word doc.txt doc-utf8.txt.",
         "הריצו oqt2word doc.txt doc-utf8.txt."),
        ("Open doc-utf8.txt in LibreOffice or Word -- Arabic shapes, Hebrew "
         "reads RTL, English stays LTR, all handled by the modern renderer.",
         "פתחו את doc-utf8.txt ב-LibreOffice או Word -- ערבית מתעצבת, עברית "
         "נקראת ימין-לשמאל, אנגלית נשארת שמאל-לימין, הכל מטופל על ידי המעבד "
         "המודרני."),
        ("Edit, then save as plain text, UTF-8 encoded.",
         "ערכו, ואז שמרו כטקסט רגיל בקידוד UTF-8."),
        ("Run txt2oqt doc-utf8.txt doc.txt to return to OpenQT format.",
         "הריצו txt2oqt doc-utf8.txt doc.txt כדי לחזור לפורמט OpenQT."),
        ("Reopen in OpenQT -- same content, re-shaped on screen.",
         "פתחו מחדש ב-OpenQT -- אותו תוכן, מעוצב מחדש על המסך."),
    ]):
        B(str(i + 1) + ". " + t(en, he))
    doc.note(t("To carry bold/underline through LibreOffice's plain-text "
               "export, mark them in the text with **word** and __word__ "
               "markdown -- txt2oqt converts those markers back into OpenQT "
               "format toggles.",
               "כדי לשמר מודגש/קו-תחתי דרך ייצוא הטקסט-הרגיל של LibreOffice, "
               "סמנו אותם בטקסט עם **word** ו-__word__ ב-markdown -- txt2oqt "
               "ממיר סמנים אלה בחזרה למתגי עיצוב של OpenQT."), "tip")

    # ============================================================ TROUBLESHOOT
    doc.h1(t("16. Troubleshooting", "16. פתרון תקלות"))
    pairs = [
        ("'OPENQT.HLP not found' when opening Docs",
         "DOSBox's drive cache is stale, or the working directory is wrong. "
         "Run 'rescan' in DOSBox after adding the file from the host, and make "
         "sure OPENQT.HLP is in the current dir, parent, \\OPENQT\\ or "
         "C:\\OPENQT\\. Regenerate it with python3 make_hlp.py.",
         "'OPENQT.HLP not found' בפתיחת Docs",
         "מטמון הכונן של DOSBox מיושן, או שתיקיית העבודה שגויה. הריצו 'rescan' "
         "ב-DOSBox לאחר הוספת הקובץ מהמארח, וודאו ש-OPENQT.HLP נמצא בתיקייה "
         "הנוכחית, באב, ב-\\OPENQT\\ או ב-C:\\OPENQT\\. צרו אותו מחדש עם "
         "python3 make_hlp.py."),
        ("OQT 3 shows Hebrew right but Arabic as gibberish (or vice-versa)",
         "Almost always a stale ARABVGA.COM shadowing the new ARABVGA.EXE. DOS "
         "resolves .COM before .EXE. Delete ARABVGA.COM, rebuild from "
         "arabvga.c, and verify ARABVGA /? shows the /P partial-upload help.",
         "OQT 3 מציג עברית נכון אך ערבית כג'יבריש (או להפך)",
         "כמעט תמיד ARABVGA.COM ישן שמסתיר את ARABVGA.EXE החדש. DOS פותר "
         ".COM לפני .EXE. מחקו את ARABVGA.COM, בנו מחדש מ-arabvga.c, וודאו "
         "ש-ARABVGA /? מציג את עזרת ה-P/ של העלאה חלקית."),
        ("The editor opens in Hebrew when I run OQTA",
         "Likely two OPENQT.EXE files exist and DOS picks the older one. "
         "Delete the stale copy and rebuild; the current binary is about 75 KB.",
         "העורך נפתח בעברית כשאני מריץ OQTA",
         "כנראה קיימים שני קובצי OPENQT.EXE ו-DOS בוחר בישן. מחקו את העותק "
         "הישן ובנו מחדש; הקובץ הנוכחי הוא כ-75 KB."),
        ("Menus look corrupted under OQTA",
         "CP864 reuses the 0xC0-0xDF box-drawing range. The launcher passes "
         "/A so dialogs draw with ASCII. If frames show Arabic letters, the "
         "/A flag is not reaching the binary -- launch via OQTA, not OPENQT "
         "directly.",
         "תפריטים נראים משובשים תחת OQTA",
         "CP864 משתמש מחדש בטווח תווי-המסגרת 0xC0-0xDF. קובץ ההפעלה מעביר A/ "
         "כך שתיבות הדו-שיח מציירות ב-ASCII. אם מסגרות מציגות אותיות ערביות, "
         "הדגל A/ אינו מגיע לקובץ -- הפעילו דרך OQTA, לא ישירות דרך OPENQT."),
        ("txt2oqt reads my UTF-8 file as legacy DOS",
         "The file probably lacks a BOM. Save it from LibreOffice with 'Use "
         "Unicode (UTF-8)' and 'Include BOM' checked, or pass /U to force "
         "UTF-8 mode.",
         "txt2oqt קורא את קובץ ה-UTF-8 שלי כ-DOS ישן",
         "כנראה חסר לקובץ BOM. שמרו אותו מ-LibreOffice עם 'Use Unicode "
         "(UTF-8)' ו-'Include BOM' מסומנים, או העבירו U/ כדי לכפות מצב UTF-8."),
        ("My keys do nothing in DOSBox",
         "Set the DOSBox keyboard to a US layout. OpenQT does its own ASCII -> "
         "Hebrew/Arabic mapping, so it expects unmodified ASCII keystrokes.",
         "המקשים שלי לא עושים כלום ב-DOSBox",
         "הגדירו את מקלדת DOSBox לפריסת US. OpenQT מבצע מיפוי ASCII ← "
         "עברית/ערבית משלו, ולכן הוא מצפה להקשות ASCII בלתי-משתנות."),
    ]
    for en_q, en_a, he_q, he_a in pairs:
        doc.h3(t(en_q, he_q))
        P(t(en_a, he_a), size=9)

    # ============================================================ VERSION
    doc.h1(t("17. Version history", "17. היסטוריית גרסאות"))
    vh = [
        ("3.9.2", "Open/Save dialog: wide dir/w file list (F2), filter "
                  "cycle (F3), and keyboard-navigable Open/Cancel buttons "
                  "(Tab / arrows).",
                  "תיבת פתיחה/שמירה: רשימת קבצים רחבה בסגנון dir/w (F2), "
                  "מעבר בין מסננים (F3), וכפתורי פתיחה/ביטול הניתנים לניווט "
                  "במקלדת (Tab / חיצים)."),
        ("3.9", "Hebrew spell check (hspell / aspell-he, follows F4); "
                "Copy to Host (Alt+C); Open warns on unsaved changes; "
                "Select All works with Copy.",
                "בדיקת איות עברית (hspell / aspell-he, עוקבת אחר F4); "
                "העתקה למארח (Alt+C); פתיחה מזהירה על שינויים שלא נשמרו; "
                "בחירת הכול עובדת עם העתקה."),
        ("3.8", "Paragraph reflow (Block -> Reformat); self-drawn mouse "
                "pointer visible in every DOSBox-X mouse mode.",
                "עיצוב פסקאות מחדש (Block -> Reformat); סמן עכבר מצויר-עצמית "
                "הנראה בכל מצבי העכבר של DOSBox-X."),
        ("3.7", "Mouse support -- clickable menus, click-to-caret, "
                "drag-select, wheel scrolling.",
                "תמיכת עכבר -- תפריטים לחיצים, מיקום סמן בלחיצה, בחירה "
                "בגרירה, גלילה בגלגלת."),
        ("3.6", "New converter txt2rtf -- Hebrew + English to RTF with "
                "bold/underline preserved.",
                "ממיר חדש txt2rtf -- עברית + אנגלית ל-RTF עם שמירת "
                "מודגש/קו-תחתי."),
        ("3.5", "Smart Paste from the host clipboard (Paste Host / Alt+V).",
                "הדבקה חכמה מלוח-הגזירים של המארח (Paste Host / Alt+V)."),
        ("3.4", "Host-assisted Tools: spell check, read aloud, translate, "
                "dictate.",
                "כלי המארח: בדיקת איות, הקראה, תרגום, הכתבה."),
        ("3.3", "Russian (CP866) input mode and converter /R flag.",
                "מצב קלט רוסית (CP866) ודגל R/ בממירים."),
        ("3.2", "Trilingual editing on one screen; in-editor Docs viewer; "
                "full 4-form Arabic shaping.",
                "עריכה תלת-לשונית במסך אחד; מציג Docs בעורך; עיצוב ערבי מלא "
                "ב-4 צורות."),
        ("3.1", "First on-screen Arabic shaping pass.",
                "מעבר עיצוב ערבי ראשון על המסך."),
        ("3.0", "Arabic input, password encryption, 30,000-line limit.",
                "קלט ערבית, הצפנת סיסמה, מגבלת 30,000 שורות."),
    ]
    doc.table([t("Version", "גרסה"), t("Highlights", "עיקרים")],
              [[v, t(en, he)] for v, en, he in vh],
              widths=[0.8, 4])
    P(t("Earlier versions (1.0-2.x) were Hebrew-only.",
        "גרסאות מוקדמות (1.0-2.x) היו עבריות בלבד."), size=9)
