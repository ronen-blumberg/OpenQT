#!/usr/bin/env python3
"""
Generate rusvga.c (CP866 8x16 VGA font loader) for OpenQT.

Strategy:
  - Use HEBVGA.COM's embedded CP437 8x16 font as the base (it lives at
    offset 1442 in the .COM and has a tighter vertical alignment than
    arabvga.c's CP864 font - glyphs start at row 2 instead of row 3).
    This gives us correct CP437 ASCII at 0x00-0x7F, box-drawing at
    0xB0-0xDF, and graphics at 0xF0-0xFF for free.
  - Override 0x80-0xAF, 0xE0-0xEF, 0xF0-0xF7, 0xFC, 0xFD with Cyrillic
    and CP866-specific symbols, sourced from /usr/share/fonts/X11/misc/
    8x13.pcf (Unicode-encoded) and padded 8x13 -> 8x16 with 2 rows top,
    1 row bottom so the baseline aligns with HEBVGA's CP437 glyphs.
  - Emit rusvga.c in the same style as arabvga.c (embedded fontbuf[]
    plus the same INT 10h AX=1110h upload routine).
"""
import sys

import freetype

HEBVGA_COM     = "/home/ronen/dos/eini2/OPENQT/HEBVGA.COM"
HEBVGA_FONT_OFF = 1442          # verified by checking 0xB3 = vertical bar
PCF_8x13       = "/tmp/8x13.pcf"
OUT_C          = "/home/ronen/dos/eini2/OPENQT/rusvga.c"

# CP866 -> Unicode mapping. HEBVGA's CP437 base already covers ASCII,
# box-drawing (0xB0-0xDF), and the graphics tail (0xF8-0xFF), so we only
# need to override the Cyrillic ranges and the two CP866-specific positions
# that differ from CP437 (0xFC No., 0xFD currency).
CP866_OVERRIDES = {
    # 0x80-0x9F: Cyrillic uppercase A..YA
    0x80: 0x0410, 0x81: 0x0411, 0x82: 0x0412, 0x83: 0x0413,
    0x84: 0x0414, 0x85: 0x0415, 0x86: 0x0416, 0x87: 0x0417,
    0x88: 0x0418, 0x89: 0x0419, 0x8A: 0x041A, 0x8B: 0x041B,
    0x8C: 0x041C, 0x8D: 0x041D, 0x8E: 0x041E, 0x8F: 0x041F,
    0x90: 0x0420, 0x91: 0x0421, 0x92: 0x0422, 0x93: 0x0423,
    0x94: 0x0424, 0x95: 0x0425, 0x96: 0x0426, 0x97: 0x0427,
    0x98: 0x0428, 0x99: 0x0429, 0x9A: 0x042A, 0x9B: 0x042B,
    0x9C: 0x042C, 0x9D: 0x042D, 0x9E: 0x042E, 0x9F: 0x042F,
    # 0xA0-0xAF: Cyrillic lowercase a..p
    0xA0: 0x0430, 0xA1: 0x0431, 0xA2: 0x0432, 0xA3: 0x0433,
    0xA4: 0x0434, 0xA5: 0x0435, 0xA6: 0x0436, 0xA7: 0x0437,
    0xA8: 0x0438, 0xA9: 0x0439, 0xAA: 0x043A, 0xAB: 0x043B,
    0xAC: 0x043C, 0xAD: 0x043D, 0xAE: 0x043E, 0xAF: 0x043F,
    # 0xE0-0xEF: Cyrillic lowercase r..ya
    0xE0: 0x0440, 0xE1: 0x0441, 0xE2: 0x0442, 0xE3: 0x0443,
    0xE4: 0x0444, 0xE5: 0x0445, 0xE6: 0x0446, 0xE7: 0x0447,
    0xE8: 0x0448, 0xE9: 0x0449, 0xEA: 0x044A, 0xEB: 0x044B,
    0xEC: 0x044C, 0xED: 0x044D, 0xEE: 0x044E, 0xEF: 0x044F,
    # 0xF0-0xF7: Yo, Ye(uk), Yi(uk), Short-U(by) - upper/lower
    0xF0: 0x0401, 0xF1: 0x0451,
    0xF2: 0x0404, 0xF3: 0x0454,
    0xF4: 0x0407, 0xF5: 0x0457,
    0xF6: 0x040E, 0xF7: 0x045E,
    # CP866-specific tail: differs from CP437 only at 0xFC and 0xFD
    0xFC: 0x2116,  # numero sign (CP437 has sup-n here)
    0xFD: 0x00A4,  # currency (CP437 has sup-2 here)
}

# Vertical placement: pad 8x13 -> 8x16 with 1 row top and 2 rows bottom.
# HEBVGA's baseline sits at row 11 (verified: 'a' bottom at row 11, 'g'
# descender at rows 12-14). 8x13's baseline is at internal-row 10. With
# PAD_TOP=1 the 8x13 baseline lands at output row 11 - aligned with HEBVGA.
PAD_TOP = 1
PAD_BOTTOM = 2
assert PAD_TOP + 13 + PAD_BOTTOM == 16


def load_hebvga_base():
    """Read HEBVGA.COM's embedded 4096-byte CP437+Hebrew font.
    Returns a list of 256 lists, each 16 bytes (one per row, top-to-bottom).
    """
    data = open(HEBVGA_COM, "rb").read()
    font = data[HEBVGA_FONT_OFF : HEBVGA_FONT_OFF + 4096]
    if len(font) != 4096:
        raise SystemExit("HEBVGA.COM truncated")
    return [list(font[i*16 : (i+1)*16]) for i in range(256)]


def render_glyph_8x13(face, codepoint):
    """Return a 13-byte list (one byte per row) for the 8x13 raster of `codepoint`."""
    idx = face.get_char_index(codepoint)
    if idx == 0:
        return None
    face.load_glyph(idx, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
    bm = face.glyph.bitmap
    # bm.pitch is bytes per row; with 1bpp mono, pitch >= ceil(width/8).
    # Use the metrics y_offset (top) to position the glyph within the cell.
    # The X11 8x13 font: ascent=11, descent=2; char-cell height=13.
    # face.glyph.bitmap_top is the baseline-relative y of the topmost row.
    top   = face.glyph.bitmap_top
    left  = face.glyph.bitmap_left
    rows  = bm.rows
    width = bm.width
    pitch = bm.pitch
    raw   = bytes(bm.buffer)

    # Fit into an 8 wide x 13 tall cell. The cell's baseline is at y = ascent-1
    # in our zero-indexed grid (i.e. row index 11 from the top for ascent=11).
    ASCENT = 11
    cell_h = 13
    cell = [0] * cell_h
    for r in range(rows):
        # row r in the glyph bitmap is at (top - 1 - r) above baseline.
        cell_y = ASCENT - top + r          # 0 = top of cell, ASCENT = baseline row
        if cell_y < 0 or cell_y >= cell_h:
            continue
        # Read this bitmap row, packed MSB-first 1bpp.
        row_byte = 0
        for x in range(min(width, 8)):
            byte_idx = r * pitch + (x >> 3)
            bit_idx  = 7 - (x & 7)
            if raw[byte_idx] & (1 << bit_idx):
                # Place at column (left + x) in the cell, MSB-first.
                col = left + x
                if 0 <= col < 8:
                    row_byte |= 1 << (7 - col)
        cell[cell_y] |= row_byte
    return cell


def pad_8x13_to_8x16(rows13):
    """Pad a 13-row glyph to 16 rows (PAD_TOP blank rows top, PAD_BOTTOM blank bottom)."""
    return [0] * PAD_TOP + list(rows13) + [0] * PAD_BOTTOM


def main():
    print(f"Loading HEBVGA.COM base font (offset {HEBVGA_FONT_OFF}) ...", file=sys.stderr)
    glyphs = load_hebvga_base()

    print(f"Loading {PCF_8x13} ...", file=sys.stderr)
    face = freetype.Face(PCF_8x13)
    face.set_pixel_sizes(0, 13)

    overridden = []
    missing = []
    for cp866, ucode in CP866_OVERRIDES.items():
        rows = render_glyph_8x13(face, ucode)
        if rows is None:
            missing.append((cp866, ucode))
            continue
        glyphs[cp866] = pad_8x13_to_8x16(rows)
        overridden.append(cp866)
    print(f"Overrode {len(overridden)} positions; missing {len(missing)}", file=sys.stderr)
    if missing:
        for cp866, ucode in missing:
            print(f"  missing CP866 0x{cp866:02X} <- U+{ucode:04X}", file=sys.stderr)

    # Emit rusvga.c
    print(f"Writing {OUT_C} ...", file=sys.stderr)
    out = open(OUT_C, "w")
    out.write(HEADER)
    for i in range(256):
        bytes_str = ", ".join(f"0x{b:02X}" for b in glyphs[i])
        out.write(f"    {bytes_str}, /* 0x{i:02X} */\n")
    out.write(FOOTER)
    out.close()
    print("done.", file=sys.stderr)


HEADER = """\
/*
 * RUSVGA - Russian VGA Font Loader for OpenQT (CP866)
 *
 * Uploads a CP866 8x16 Russian font into the VGA character generator via
 * INT 10h AX=1110h. Companion to HEBVGA.COM and ARABVGA.EXE. Run before
 * OPENQT to switch screen glyphs to Cyrillic.
 *
 * The CP866 font is EMBEDDED -- no external file needed. The CP437
 * base (ASCII, box-drawing at 0xB0-0xDF, graphics at 0xF0-0xFF) is
 * lifted directly out of HEBVGA.COM. The Cyrillic letters at
 * 0x80-0xAF and 0xE0-0xEF, plus the eight Yo/Ye/Yi/Short-U letters
 * at 0xF0-0xF7, plus the CP866-specific No. (0xFC) and currency
 * (0xFD), are sourced from the X11 fixed-misc 8x13 font (xfonts-base,
 * ISO10646-1) padded into the 8x16 cell. Generated by make_rusvga.py.
 *
 * Russian mode in OpenQT is its own session - CP866 byte ranges
 * conflict with CP862 Hebrew at 0x80-0x9A and CP864 Arabic at
 * 0xA0-0xFE, so RUSVGA does NOT have a /P partial-overlay mode.
 *
 * Usage:
 *   RUSVGA                        -> upload embedded CP866 font (full 256)
 *   RUSVGA /F=path/to/CPI /CP=N   -> alternate: load codepage N from a CPI
 *
 * Compile (Watcom C, 16-bit real-mode .EXE):
 *   wcl -bt=dos -ml -ox rusvga.c -fe=rusvga.exe
 *
 * by Ronen Blumberg - 2026 - Public Domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <i86.h>

#define DEFAULT_CP      866
#define FONT_HEIGHT     16
#define NUM_CHARS       256
#define FONT_BYTES      (FONT_HEIGHT * NUM_CHARS)   /* 4096 */

/* Embedded CP866 8x16 font (CP437 base + Cyrillic from X11 8x13) */
static unsigned char fontbuf[FONT_BYTES] = {
"""

FOOTER = """\
};

static unsigned short rd16(const unsigned char *p)
{
    return (unsigned short)p[0] | ((unsigned short)p[1] << 8);
}

static unsigned long rd32(const unsigned char *p)
{
    return (unsigned long)p[0]
         | ((unsigned long)p[1] << 8)
         | ((unsigned long)p[2] << 16)
         | ((unsigned long)p[3] << 24);
}

static int read_at(FILE *fp, long off, void *dst, size_t n)
{
    if (fseek(fp, off, SEEK_SET) != 0) return 0;
    return fread(dst, 1, n, fp) == n;
}

/* Locate the cpih_offset for a given codepage id. Returns 0 on miss. */
static long find_codepage(FILE *fp, int want_cp)
{
    unsigned char hdr[24];
    unsigned char cpeh[40];
    long fih_off, p;
    int num_cps, i;

    if (!read_at(fp, 0, hdr, 24)) return 0;
    if (hdr[0] != 0xFF || memcmp(hdr+1, "FONT   ", 7) != 0) {
        fprintf(stderr, "Not a FONT-format CPI file\\n");
        return 0;
    }
    fih_off = (long)rd32(hdr + 0x13);

    if (!read_at(fp, fih_off, hdr, 2)) return 0;
    num_cps = rd16(hdr);

    p = fih_off + 2;
    for (i = 0; i < num_cps; i++) {
        if (!read_at(fp, p, cpeh, 28)) return 0;
        if ((int)rd16(cpeh + 0x10) == want_cp) {
            return (long)rd32(cpeh + 0x18);
        }
        p = (long)rd32(cpeh + 0x02);
        if (p == 0) break;
    }
    return 0;
}

static int load_8x16(FILE *fp, long cpih_off)
{
    unsigned char hdr[6];
    unsigned char fhdr[6];
    int num_fonts, i;
    long p;
    int height, width, numch;

    if (!read_at(fp, cpih_off, hdr, 6)) return 0;
    num_fonts = rd16(hdr + 2);
    p = cpih_off + 6;

    for (i = 0; i < num_fonts; i++) {
        if (!read_at(fp, p, fhdr, 6)) return 0;
        height = fhdr[0];
        width  = fhdr[1];
        numch  = rd16(fhdr + 4);
        p += 6;

        if (height == 16 && width == 8 && numch == 256) {
            if (!read_at(fp, p, fontbuf, FONT_BYTES)) return 0;
            return 1;
        }
        p += (long)height * numch;
    }
    fprintf(stderr, "No 8x16 font found in this codepage\\n");
    return 0;
}

/* INT 10h AX=1110h: upload user font to VGA char generator.
 * Uses REGPACK rather than `union REGS`/`int86x` because Open Watcom's
 * WORDREGS does not expose BP -- BP is required for this BIOS call
 * (it's the offset half of the ES:BP font-data pointer). */
static void upload_to_vga(void)
{
    union REGPACK r;

    memset(&r, 0, sizeof(r));
    r.h.ah = 0x11;
    r.h.al = 0x10;
    r.h.bh = FONT_HEIGHT;
    r.h.bl = 0;
    r.w.cx = NUM_CHARS;
    r.w.dx = 0;
    r.w.es = FP_SEG(fontbuf);
    r.w.bp = FP_OFF(fontbuf);
    intr(0x10, &r);
}

static int load_from_cpi(const char *path, int cp)
{
    FILE *fp;
    long cpih_off;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "RUSVGA: cannot open %s\\n", path);
        return 0;
    }
    cpih_off = find_codepage(fp, cp);
    if (!cpih_off) {
        fprintf(stderr, "RUSVGA: codepage %d not found in %s\\n", cp, path);
        fclose(fp);
        return 0;
    }
    if (!load_8x16(fp, cpih_off)) {
        fprintf(stderr, "RUSVGA: failed to read 8x16 font for CP %d\\n", cp);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

int main(int argc, char **argv)
{
    int   cp   = DEFAULT_CP;
    char *path = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        char *a = argv[i];
        if ((a[0]=='/' || a[0]=='-') &&
            (a[1]=='C'||a[1]=='c') && (a[2]=='P'||a[2]=='p') && a[3]=='=') {
            cp = atoi(a + 4);
        } else if ((a[0]=='/' || a[0]=='-') &&
                   (a[1]=='F'||a[1]=='f') && a[2]=='=') {
            path = a + 3;
        } else if (strcmp(a,"/?")==0 || strcmp(a,"-?")==0 ||
                   strcmp(a,"-h")==0 || strcmp(a,"--help")==0) {
            printf("RUSVGA - Russian VGA font loader for OpenQT\\n");
            printf("Usage: RUSVGA              -> upload embedded CP866 (full 256)\\n");
            printf("       RUSVGA /F=file.cpi /CP=nnn -> load from CPI\\n");
            return 0;
        }
    }

    if (path) {
        if (!load_from_cpi(path, cp)) return 1;
        upload_to_vga();
        printf("RUSVGA: loaded CP%d from %s\\n", cp, path);
    } else {
        upload_to_vga();
        printf("RUSVGA: loaded embedded CP866 Russian font\\n");
    }
    return 0;
}
"""


if __name__ == "__main__":
    main()
