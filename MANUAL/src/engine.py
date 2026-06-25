# -*- coding: utf-8 -*-
"""Minimal block-flow PDF engine with LTR + RTL (BiDi) support."""
import os
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from bidi.algorithm import get_display
import arabic_reshaper

_reshaper = arabic_reshaper.ArabicReshaper(configuration={
    "delete_harakat": False, "support_ligatures": True})

DV = "/usr/share/fonts/truetype/dejavu/"
pdfmetrics.registerFont(TTFont("Body",  DV + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("BodyB", DV + "DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("Mono",  DV + "DejaVuSansMono.ttf"))
pdfmetrics.registerFont(TTFont("MonoB", DV + "DejaVuSansMono-Bold.ttf"))

PAGE_W, PAGE_H = A4
M_TOP = 20 * mm
M_BOT = 18 * mm
M_LEFT = 20 * mm
M_RIGHT = 20 * mm
CONTENT_L = M_LEFT
CONTENT_R = PAGE_W - M_RIGHT
CONTENT_W = CONTENT_R - CONTENT_L

ACCENT = colors.HexColor("#1f4e79")
ACCENT2 = colors.HexColor("#2e75b6")
GREYBG = colors.HexColor("#eef2f7")
CODEBG = colors.HexColor("#f4f4f2")
GRID = colors.HexColor("#b8c4d0")
HEADBG = colors.HexColor("#dbe5f1")


def has_rtl(s):
    for ch in s:
        o = ord(ch)
        if (0x0590 <= o <= 0x05FF) or (0x0600 <= o <= 0x06FF) or (0xFB1D <= o <= 0xFEFC):
            return True
    return False


def has_arabic(s):
    return any(0x0600 <= ord(c) <= 0x06FF or 0xFB50 <= ord(c) <= 0xFEFF for c in s)


def shape(s, rtl):
    """Reshape Arabic, then reorder a single visual line for drawing.

    `rtl` is the document's base direction. English-only strings come back
    unchanged; strings containing Hebrew/Arabic are reshaped + BiDi-reordered.
    """
    if not s:
        return ""
    if not has_rtl(s):
        return s
    txt = _reshaper.reshape(s) if has_arabic(s) else s
    base = "R" if rtl else "L"
    try:
        return get_display(txt, base_dir=base)
    except Exception:
        return txt


def wrap(text, font, size, max_w):
    if text == "":
        return [""]
    out = []
    for para in text.split("\n"):
        words = para.split(" ")
        cur = ""
        for w in words:
            trial = w if not cur else cur + " " + w
            if pdfmetrics.stringWidth(trial, font, size) <= max_w:
                cur = trial
            else:
                if cur:
                    out.append(cur)
                if pdfmetrics.stringWidth(w, font, size) > max_w:
                    part = ""
                    for ch in w:
                        if pdfmetrics.stringWidth(part + ch, font, size) <= max_w:
                            part += ch
                        else:
                            out.append(part)
                            part = ch
                    cur = part
                else:
                    cur = w
        out.append(cur)
    return out or [""]


class Doc:
    def __init__(self, path, rtl, title, subtitle, footer, dry=False):
        self.path = path
        self.rtl = rtl
        self.title = title
        self.subtitle = subtitle
        self.footer = footer
        self.dry = dry
        self.page = 1
        self.h1pages = {}   # heading text -> page number
        self.c = canvas.Canvas(path, pagesize=A4)
        self.y = PAGE_H - M_TOP
        self._page_started = False

    # --- low level ---
    def _line_r(self, text, font, size, color=colors.black, x_right=CONTENT_R):
        self.c.setFont(font, size)
        self.c.setFillColor(color)
        self.c.drawRightString(x_right, self.y, shape(text, True))

    def _line_l(self, text, font, size, color=colors.black, x_left=CONTENT_L):
        self.c.setFont(font, size)
        self.c.setFillColor(color)
        self.c.drawString(x_left, self.y, shape(text, self.rtl))

    def line(self, text, font, size, color=colors.black, x_left=CONTENT_L, x_right=CONTENT_R):
        if self.rtl:
            self._line_r(text, font, size, color, x_right)
        else:
            self._line_l(text, font, size, color, x_left)

    def need(self, h):
        if self.y - h < M_BOT:
            self.new_page()

    def new_page(self):
        self.page_footer()
        self.c.showPage()
        self.page += 1
        self.y = PAGE_H - M_TOP - 6
        self.page_header()

    def page_header(self):
        self.c.setFont("Body", 8)
        self.c.setFillColor(colors.HexColor("#8a98a8"))
        if self.rtl:
            self.c.drawRightString(CONTENT_R, PAGE_H - M_TOP + 9, shape(self.title, True))
        else:
            self.c.drawString(CONTENT_L, PAGE_H - M_TOP + 9, self.title)
        self.c.setStrokeColor(GRID)
        self.c.setLineWidth(0.5)
        self.c.line(CONTENT_L, PAGE_H - M_TOP + 5, CONTENT_R, PAGE_H - M_TOP + 5)

    def page_footer(self):
        self.c.setFont("Body", 8)
        self.c.setFillColor(colors.HexColor("#8a98a8"))
        label = self.footer + "  -  " + str(self.page)
        self.c.drawCentredString(PAGE_W / 2, M_BOT - 12, shape(label, self.rtl))

    # --- blocks ---
    def gap(self, h):
        self.need(h)
        self.y -= h

    def h1(self, text):
        self.need(40)
        if self.y < PAGE_H - M_TOP - 2:
            self.gap(10)
        self.h1pages[text] = self.page
        self.c.setFillColor(ACCENT)
        bar_h = 17
        self.c.setFont("BodyB", 16)
        tw = pdfmetrics.stringWidth(text, "BodyB", 16)
        if self.rtl:
            self.c.rect(CONTENT_R - 3, self.y - 4, 3, bar_h, fill=1, stroke=0)
            self._line_r(text, "BodyB", 16, ACCENT, CONTENT_R - 8)
        else:
            self.c.rect(CONTENT_L, self.y - 4, 3, bar_h, fill=1, stroke=0)
            self._line_l(text, "BodyB", 16, ACCENT, CONTENT_L + 8)
        self.y -= 22

    def h2(self, text):
        self.need(30)
        self.gap(6)
        self.line(text, "BodyB", 12.5, ACCENT2)
        self.y -= 16
        self.c.setStrokeColor(colors.HexColor("#d2dae4"))
        self.c.setLineWidth(0.4)
        self.c.line(CONTENT_L, self.y + 6, CONTENT_R, self.y + 6)

    def h3(self, text):
        self.need(24)
        self.gap(3)
        self.line(text, "BodyB", 10.5, colors.HexColor("#33404d"))
        self.y -= 14

    def para(self, text, size=9.5, gap_after=6, color=colors.black, font="Body"):
        lines = wrap(text, font, size, CONTENT_W)
        lh = size + 3.2
        for ln in lines:
            self.need(lh)
            self.line(ln, font, size, color)
            self.y -= lh
        self.y -= gap_after

    def bullet(self, text, size=9.5, indent=14, gap_after=3.5):
        lh = size + 3.2
        avail = CONTENT_W - indent
        lines = wrap(text, "Body", size, avail)
        first = True
        for ln in lines:
            self.need(lh)
            self.c.setFont("Body", size)
            self.c.setFillColor(colors.black)
            if self.rtl:
                if first:
                    self.c.setFillColor(ACCENT2)
                    self.c.drawRightString(CONTENT_R, self.y, "•")
                    self.c.setFillColor(colors.black)
                self.c.drawRightString(CONTENT_R - indent, self.y, shape(ln, True))
            else:
                if first:
                    self.c.setFillColor(ACCENT2)
                    self.c.drawString(CONTENT_L, self.y, "•")
                    self.c.setFillColor(colors.black)
                self.c.drawString(CONTENT_L + indent, self.y, shape(ln, self.rtl))
            self.y -= lh
            first = False
        self.y -= gap_after

    def code(self, text, gap_after=7):
        size = 9
        lh = size + 3
        pad = 6
        raw_lines = []
        for ln in text.split("\n"):
            raw_lines.extend(wrap(ln, "Mono", size, CONTENT_W - 2 * pad) or [""])
        block_h = lh * len(raw_lines) + 2 * pad
        self.need(block_h + 4)
        top = self.y + size
        self.c.setFillColor(CODEBG)
        self.c.setStrokeColor(GRID)
        self.c.setLineWidth(0.5)
        self.c.rect(CONTENT_L, top - block_h, CONTENT_W, block_h, fill=1, stroke=1)
        self.c.setFillColor(colors.HexColor("#0b3d2e"))
        ty = top - pad - size + 1
        for ln in raw_lines:
            # ASCII lines stay monospace; lines with Hebrew/Arabic use the
            # proportional font (the mono face lacks Hebrew) and are reshaped.
            if has_rtl(ln):
                self.c.setFont("Body", size)
                self.c.drawString(CONTENT_L + pad, ty, shape(ln, self.rtl))
            else:
                self.c.setFont("Mono", size)
                self.c.drawString(CONTENT_L + pad, ty, ln)
            ty -= lh
        self.y = top - block_h - 2
        self.y -= gap_after

    def table(self, headers, rows, widths=None, size=8.8, gap_after=8):
        n = len(headers)
        if widths is None:
            widths = [1.0] * n
        tot = sum(widths)
        colw = [CONTENT_W * w / tot for w in widths]
        # column x positions (left edges), visual order
        if self.rtl:
            order = list(range(n - 1, -1, -1))  # draw col0 at right
        else:
            order = list(range(n))
        # precompute left x for each visual slot
        xs = []
        x = CONTENT_L
        for slot in range(n):
            xs.append(x)
            x += colw[order[slot]]
        pad = 4
        lh = size + 3

        def row_height(cells):
            h = 0
            for ci in range(n):
                w = colw[ci] - 2 * pad
                lines = wrap(str(cells[ci]), "Body", size, w)
                h = max(h, lh * len(lines) + 2 * pad)
            return h

        def draw_row(cells, header=False):
            rh = row_height(cells)
            self.need(rh)
            top = self.y + size
            # background
            if header:
                self.c.setFillColor(HEADBG)
                self.c.rect(CONTENT_L, top - rh, CONTENT_W, rh, fill=1, stroke=0)
            # grid
            self.c.setStrokeColor(GRID)
            self.c.setLineWidth(0.4)
            self.c.rect(CONTENT_L, top - rh, CONTENT_W, rh, fill=0, stroke=1)
            x = CONTENT_L
            for slot in range(n):
                ci = order[slot]
                cw = colw[ci]
                if slot > 0:
                    self.c.line(x, top - rh, x, top)
                cell = str(cells[ci])
                lines = wrap(cell, "BodyB" if header else "Body", size, cw - 2 * pad)
                ty = top - pad - size + 1
                fnt = "BodyB" if header else "Body"
                self.c.setFont(fnt, size)
                self.c.setFillColor(ACCENT if header else colors.black)
                for ln in lines:
                    if self.rtl:
                        self.c.drawRightString(x + cw - pad, ty, shape(ln, self.rtl))
                    else:
                        self.c.drawString(x + pad, ty, shape(ln, self.rtl))
                    ty -= lh
                x += cw
            self.y = top - rh

        draw_row(headers, header=True)
        for r in rows:
            draw_row(r)
        self.y -= gap_after

    def note(self, text, kind="note", size=9, gap_after=7):
        # callout box
        col = {"note": colors.HexColor("#2e75b6"),
               "tip": colors.HexColor("#2e8b57"),
               "warn": colors.HexColor("#c0392b")}.get(kind, ACCENT2)
        bg = {"note": colors.HexColor("#eaf1f8"),
              "tip": colors.HexColor("#e9f5ee"),
              "warn": colors.HexColor("#fbecea")}.get(kind, GREYBG)
        pad = 7
        lines = wrap(text, "Body", size, CONTENT_W - 2 * pad - 6)
        lh = size + 3.2
        block_h = lh * len(lines) + 2 * pad
        self.need(block_h + 4)
        top = self.y + size
        self.c.setFillColor(bg)
        self.c.rect(CONTENT_L, top - block_h, CONTENT_W, block_h, fill=1, stroke=0)
        self.c.setFillColor(col)
        if self.rtl:
            self.c.rect(CONTENT_R - 3, top - block_h, 3, block_h, fill=1, stroke=0)
        else:
            self.c.rect(CONTENT_L, top - block_h, 3, block_h, fill=1, stroke=0)
        self.c.setFillColor(colors.HexColor("#222a33"))
        self.c.setFont("Body", size)
        ty = top - pad - size + 1
        for ln in lines:
            if self.rtl:
                self.c.drawRightString(CONTENT_R - pad - 3, ty, shape(ln, self.rtl))
            else:
                self.c.drawString(CONTENT_L + pad + 3, ty, shape(ln, self.rtl))
            ty -= lh
        self.y = top - block_h - 2
        self.y -= gap_after

    # --- title & toc ---
    def title_page(self, version_line, tagline, bullets):
        self.c.setFillColor(ACCENT)
        self.c.rect(0, PAGE_H - 78 * mm, PAGE_W, 78 * mm, fill=1, stroke=0)
        self.c.setFillColor(colors.white)
        self.c.setFont("BodyB", 46)
        self.c.drawCentredString(PAGE_W / 2, PAGE_H - 42 * mm, "OpenQT")
        self.c.setFont("Body", 15)
        self.c.drawCentredString(PAGE_W / 2, PAGE_H - 54 * mm, shape(tagline, self.rtl))
        self.c.setFont("Body", 11)
        self.c.setFillColor(colors.HexColor("#cfe0f2"))
        self.c.drawCentredString(PAGE_W / 2, PAGE_H - 64 * mm, shape(version_line, self.rtl))
        # title of manual
        self.c.setFillColor(ACCENT)
        self.c.setFont("BodyB", 20)
        self.c.drawCentredString(PAGE_W / 2, PAGE_H - 100 * mm, shape(self.title, self.rtl))
        self.c.setFillColor(colors.HexColor("#33404d"))
        self.c.setFont("Body", 11)
        self.c.drawCentredString(PAGE_W / 2, PAGE_H - 110 * mm, shape(self.subtitle, self.rtl))
        # highlight bullets centered block
        self.y = PAGE_H - 128 * mm
        for b in bullets:
            self.c.setFont("Body", 10.5)
            self.c.setFillColor(colors.HexColor("#33404d"))
            self.c.drawCentredString(PAGE_W / 2, self.y, shape(b, self.rtl))
            self.y -= 15
        self.c.setFont("Body", 9)
        self.c.setFillColor(colors.HexColor("#8a98a8"))
        self.c.drawCentredString(PAGE_W / 2, 22 * mm, shape(self.footer, self.rtl))
        self.c.showPage()
        self.page += 1
        self.y = PAGE_H - M_TOP

    def toc_page(self, entries, heading, pagemap):
        self.y = PAGE_H - M_TOP - 6
        self.c.setFillColor(ACCENT)
        self.line(heading, "BodyB", 18, ACCENT)
        self.y -= 30
        for txt in entries:
            self.need(16)
            pg = pagemap.get(txt)
            self.c.setFont("Body", 10.5)
            self.c.setFillColor(colors.HexColor("#222a33"))
            num = str(pg) if pg else ""
            if self.rtl:
                self.c.drawRightString(CONTENT_R, self.y, shape(txt, True))
                self.c.setFillColor(colors.HexColor("#8a98a8"))
                self.c.drawString(CONTENT_L, self.y, num)
            else:
                self.c.drawString(CONTENT_L, self.y, shape(txt, self.rtl))
                self.c.setFillColor(colors.HexColor("#8a98a8"))
                self.c.drawRightString(CONTENT_R, self.y, num)
            # dotted leader
            self.c.setFillColor(colors.HexColor("#c4ccd6"))
            self.y -= 16
        self.c.showPage()
        self.page += 1
        self.y = PAGE_H - M_TOP - 6

    def finish(self):
        self.page_footer()
        self.c.showPage()
        self.c.save()
