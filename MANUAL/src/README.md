# OpenQT manual generator

Generates the two PDFs in the parent `MANUAL/` folder:

- `OpenQT-Manual-EN.pdf` — English, left-to-right
- `OpenQT-Manual-HE.pdf` — Hebrew, right-to-left

Both editions are produced from the **same content source**, so they stay in
sync. The content is transcribed from the project's `README.md` / `CLAUDE.md`
and the in-editor `.HLP` text.

## Files

| File | Role |
|---|---|
| `engine.py` | A small block-flow PDF engine (headings, paragraphs, bullets, tables, code blocks, callouts, title page, TOC). Handles LTR and RTL, reshapes Arabic and runs the BiDi algorithm so Hebrew/Arabic display correctly. |
| `content.py` | The whole manual as data. `emit(doc, L)` emits every block for language `L` (`'en'` or `'he'`); each string is given in both languages via a local `t(en, he)` helper. |
| `make.py` | Driver. Renders each language twice (dry pass to collect chapter page numbers, real pass with a numbered TOC) and writes the PDFs to `MANUAL/`. |

## Dependencies

Python packages (`pip install -r requirements.txt`):

- `reportlab` — PDF drawing
- `python-bidi` — Unicode bidirectional algorithm (Hebrew/Arabic ordering)
- `arabic-reshaper` — Arabic positional shaping (joined letterforms)

System fonts — the **DejaVu** TrueType family, which is the single installed
face that covers Latin + Cyrillic + Hebrew + Arabic (base **and** presentation
forms). On Debian/Ubuntu:

```
sudo apt install fonts-dejavu-core
```

`engine.py` loads them from `/usr/share/fonts/truetype/dejavu/`; adjust the
`DV` path at the top of `engine.py` if your distribution stores them elsewhere.

## Build

```
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
./venv/bin/python make.py
```

Run it from this `src/` directory (the script finds `MANUAL/` as its parent and
writes the PDFs there). Editing the manual text means editing the `t(en, he)`
strings in `content.py` and re-running `make.py` — no other step is needed.

## Notes

- Each document is uniformly one direction, so every text draw goes through one
  `shape()` step: English-only strings pass through untouched, while strings
  containing Hebrew/Arabic are reshaped and BiDi-reordered for their base
  direction.
- The monospace face lacks Hebrew glyphs, so code-block lines that contain
  Hebrew/Arabic are drawn with the proportional font instead (see `Doc.code`).
- The TOC relies on the title page (1 page) + TOC (1 page) being a fixed
  2-page front matter so the dry and real passes paginate identically. If you
  add so many chapters that the TOC spills onto a second page, give it a fixed
  2-page reserve in both passes.
