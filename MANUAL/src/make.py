# -*- coding: utf-8 -*-
"""Build the OpenQT user manual as two PDFs (English LTR + Hebrew RTL).

Output is written to the parent MANUAL/ directory:
    OpenQT-Manual-EN.pdf
    OpenQT-Manual-HE.pdf

Each language is rendered twice: a first ("dry") pass collects the page number
of every chapter heading, and a second pass renders the real document with a
table of contents that carries those numbers. Both passes have an identical
title page + 1-page TOC in front of the body, so pagination -- and therefore
the recorded page numbers -- is the same in both passes.

Requirements (see README.md in this folder):
    reportlab, python-bidi, arabic-reshaper, and the DejaVu TrueType fonts
    (Debian/Ubuntu: the `fonts-dejavu-core` package).
"""
import os
import tempfile

from engine import Doc
from content import emit

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.dirname(HERE)              # the MANUAL/ folder
TMP = tempfile.gettempdir()

META = {
    "en": dict(
        file="OpenQT-Manual-EN.pdf",
        title="OpenQT User Manual",
        subtitle="A Hebrew / Arabic / Russian / English word processor for DOS",
        footer="OpenQT 3.8.0 - User Manual",
        tagline="Multilingual DOS Word Processor",
        version="Version 3.8.0   -   Author: Ronen Blumberg   -   Public Domain",
        bullets=[
            "Hebrew - Arabic - Russian - English",
            "True trilingual editing with full bidirectional (RTL) support",
            "Inline formatting, undo/redo, encryption, UTF-8 round-trip",
        ],
        toc="Contents",
    ),
    "he": dict(
        file="OpenQT-Manual-HE.pdf",
        title="מדריך למשתמש OpenQT",
        subtitle="מעבד תמלילים ל-DOS לעברית / ערבית / רוסית / אנגלית",
        footer="OpenQT 3.8.0 - מדריך למשתמש",
        tagline="מעבד תמלילים רב-לשוני ל-DOS",
        version="גרסה 3.8.0   -   מחבר: רונן בלומברג   -   נחלת הכלל",
        bullets=[
            "עברית - ערבית - רוסית - אנגלית",
            "עריכה תלת-לשונית אמיתית עם תמיכה דו-כיוונית מלאה",
            "עיצוב מוטמע, ביטול/שחזור, הצפנה, הלוך-ושוב ב-UTF-8",
        ],
        toc="תוכן העניינים",
    ),
}


def build(lang, pagemap, entries, path):
    m = META[lang]
    rtl = (lang == "he")
    d = Doc(path, rtl, m["title"], m["subtitle"], m["footer"])
    d.title_page(m["version"], m["tagline"], m["bullets"])
    d.toc_page(entries, m["toc"], pagemap)
    d.page_header()
    emit(d, lang)
    d.finish()
    return d


def main():
    for lang in ("en", "he"):
        m = META[lang]
        # pass 1: collect chapter-heading page numbers
        p1 = build(lang, {}, [], os.path.join(TMP, "_oqtman_p1_" + lang + ".pdf"))
        pagemap = dict(p1.h1pages)
        entries = list(pagemap.keys())
        # pass 2: real render with TOC page numbers
        final = os.path.join(OUT, m["file"])
        build(lang, pagemap, entries, final)
        print("wrote", final, "(%d chapters)" % len(entries))
    print("done")


if __name__ == "__main__":
    main()
