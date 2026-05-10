/*
 * TXT2OQT - Convert UTF-8 (or legacy DOS) text to OpenQT format.
 *
 * Default mode: UTF-8 input from modern editors (LibreOffice, Word,
 * Notepad++, ...). Use this to bring text BACK from those editors
 * into OpenQT — the natural inverse of oqt2word.
 *
 *   - Hebrew  U+05D0..U+05EA          -> CP862 0x80..0x9A
 *   - Arabic basic block U+0621..U+064A -> CP864 abstract isolated bytes
 *   - Arabic-Indic digits U+0660..U+0669 -> CP864 0xB0..0xB9
 *   - Arabic punctuation (U+060C, U+061B, U+061F) -> CP864
 *   - Arabic Presentation Forms-B Lam-Alef ligatures -> CP864 0xF9
 *   - ASCII passes through. UTF-8 BOM is stripped.
 *   - Logical order is assumed (LibreOffice/Word save logical).
 *
 * /R flag: target is OpenQT Russian (CP866). Cyrillic, Yo/Ye/Yi/Short-U,
 * box-drawing and the CP866-specific tail (No., currency, etc.) are
 * mapped to their CP866 byte values; Hebrew/Arabic codepoints are
 * dropped. Use this when bringing a Russian UTF-8 document back into
 * OpenQT for editing in OQTR mode.
 *
 * Markdown formatting markers in the input are translated to OpenQT
 * inline format toggle bytes:
 *     **bold**       -> 0x02 ... 0x02 (FMT_BOLD toggle)
 *     __underline__  -> 0x03 ... 0x03 (FMT_UNDERLINE toggle)
 *
 * Legacy DOS mode (/D flag, or no UTF-8 BOM detected): input is
 * already CP862/CP864 (or CP866 with /R) encoded. Visual order is
 * reversed to logical unless /V is given. CP866 (Russian) DOS files
 * are LTR-only - no order reversal is applied even without /V.
 *
 * Compile (Watcom, DOS4GW):
 *     wcl386 -bt=dos -l=dos4g txt2oqt.c -fe=txt2oqt.exe
 *
 * Usage: txt2oqt <input.txt> <output.txt> [/V] [/D] [/U] [/R]
 *     /V   keep visual order (legacy DOS mode only)
 *     /D   force legacy DOS encoding mode
 *     /U   force UTF-8 mode (skip BOM detection)
 *     /R   target OpenQT Russian (CP866) instead of Hebrew/Arabic
 *
 * by Ronen Blumberg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE        2048
#define TEXT_WIDTH      71

#define FMT_BOLD        0x02
#define FMT_UNDERLINE   0x03

#define HEB_FIRST       0x80
#define HEB_LAST        0x9A
#define ARAB_FIRST      0xA1
#define ARAB_LAST       0xFD

static int keep_visual = 0;
static int force_dos   = 0;
static int force_utf8  = 0;
static int russian_mode = 0;
static int input_is_utf8 = 1;   /* default mode */

/* ========================================================================
 * Unicode -> CP862 / CP864 mapping
 * ======================================================================== */

/* Hebrew U+05D0..05EA -> CP862 0x80..0x9A. Returns 0 if not Hebrew. */
static unsigned char unicode_to_cp862(unsigned int cp)
{
    if (cp >= 0x05D0 && cp <= 0x05EA) return (unsigned char)(0x80 + (cp - 0x05D0));
    return 0;
}

/* Arabic basic block + digits + punctuation + lam-alef ligatures
 * -> CP864 abstract isolated-form bytes. Returns 0 if not Arabic. */
static unsigned char unicode_to_cp864(unsigned int cp)
{
    /* Basic Arabic block U+0621..U+063A (Hamza..Ghain). The font in
     * this build has no separate Alef-Hamza-below glyph, so U+0625
     * falls back to U+0623 (Alef-Hamza-above) for visual fidelity. */
    static const unsigned char basic[0x3A - 0x21 + 1] = {
        /* 0x621 */ 0xC1, /* Hamza             */
        /* 0x622 */ 0xC2, /* Alef-Madda        */
        /* 0x623 */ 0xC3, /* Alef-Hamza-above  */
        /* 0x624 */ 0xC4, /* Waw-Hamza         */
        /* 0x625 */ 0xC3, /* Alef-Hamza-below  -> Alef-Hamza-above (font fallback) */
        /* 0x626 */ 0xC6, /* Yeh-Hamza         */
        /* 0x627 */ 0xC7, /* Alef              */
        /* 0x628 */ 0xA9, /* Beh               */
        /* 0x629 */ 0xC9, /* Teh-Marbuta       */
        /* 0x62A */ 0xAA, /* Teh               */
        /* 0x62B */ 0xAB, /* Theh              */
        /* 0x62C */ 0xAD, /* Jeem              */
        /* 0x62D */ 0xAE, /* Hah               */
        /* 0x62E */ 0xAF, /* Khah              */
        /* 0x62F */ 0xCF, /* Dal               */
        /* 0x630 */ 0xD0, /* Thal              */
        /* 0x631 */ 0xD1, /* Reh               */
        /* 0x632 */ 0xD2, /* Zain              */
        /* 0x633 */ 0xBC, /* Seen              */
        /* 0x634 */ 0xBD, /* Sheen             */
        /* 0x635 */ 0xBE, /* Sad               */
        /* 0x636 */ 0xEB, /* Dad               */
        /* 0x637 */ 0xD7, /* Tah               */
        /* 0x638 */ 0xD8, /* Zah               */
        /* 0x639 */ 0xDF, /* Ain               */
        /* 0x63A */ 0xEE  /* Ghain             */
    };
    /* U+0641..U+064A (Feh..Yeh). */
    static const unsigned char ext[0x4A - 0x41 + 1] = {
        /* 0x641 */ 0xBA, /* Feh           */
        /* 0x642 */ 0xF8, /* Qaf           */
        /* 0x643 */ 0xFC, /* Kaf           */
        /* 0x644 */ 0xFB, /* Lam           */
        /* 0x645 */ 0xEF, /* Meem          */
        /* 0x646 */ 0xF2, /* Noon          */
        /* 0x647 */ 0xF3, /* Heh           */
        /* 0x648 */ 0xE8, /* Waw           */
        /* 0x649 */ 0xE9, /* Alef-Maksura  */
        /* 0x64A */ 0xFD  /* Yeh           */
    };

    if (cp >= 0x0621 && cp <= 0x063A) return basic[cp - 0x0621];
    if (cp == 0x0640) return 0xE0;                       /* Tatweel */
    if (cp >= 0x0641 && cp <= 0x064A) return ext[cp - 0x0641];

    if (cp == 0x060C) return 0xAC;                       /* Arabic comma     */
    if (cp == 0x061B) return 0xBB;                       /* Arabic semicolon */
    if (cp == 0x061F) return 0xBF;                       /* Arabic qmark     */

    if (cp >= 0x0660 && cp <= 0x0669)
        return (unsigned char)(0xB0 + (cp - 0x0660));    /* Arabic-Indic digits */

    /* All Lam-Alef ligature presentation forms collapse to the one
     * glyph our font has at 0xF9 (Lam-Alef-Madda isolated). */
    if (cp >= 0xFEF5 && cp <= 0xFEFC) return 0xF9;

    /* Arabic Presentation Forms-B (U+FE70..FEFF) other than lam-alef.
     * Each "letter group" in this block has up to four positional
     * forms (iso, final, init, medial). We fold them back to their
     * base letter in the basic Arabic block, then recurse. */
    if (cp >= 0xFE80 && cp <= 0xFEFC) {
        /* Coarse fold by hand-listed mapping. */
        unsigned int base = 0;
        if      (cp >= 0xFE80 && cp <= 0xFE80) base = 0x0621;
        else if (cp >= 0xFE81 && cp <= 0xFE82) base = 0x0622;
        else if (cp >= 0xFE83 && cp <= 0xFE84) base = 0x0623;
        else if (cp >= 0xFE85 && cp <= 0xFE86) base = 0x0624;
        else if (cp >= 0xFE87 && cp <= 0xFE88) base = 0x0625;
        else if (cp >= 0xFE89 && cp <= 0xFE8C) base = 0x0626;
        else if (cp >= 0xFE8D && cp <= 0xFE8E) base = 0x0627;
        else if (cp >= 0xFE8F && cp <= 0xFE92) base = 0x0628;
        else if (cp >= 0xFE93 && cp <= 0xFE94) base = 0x0629;
        else if (cp >= 0xFE95 && cp <= 0xFE98) base = 0x062A;
        else if (cp >= 0xFE99 && cp <= 0xFE9C) base = 0x062B;
        else if (cp >= 0xFE9D && cp <= 0xFEA0) base = 0x062C;
        else if (cp >= 0xFEA1 && cp <= 0xFEA4) base = 0x062D;
        else if (cp >= 0xFEA5 && cp <= 0xFEA8) base = 0x062E;
        else if (cp >= 0xFEA9 && cp <= 0xFEAA) base = 0x062F;
        else if (cp >= 0xFEAB && cp <= 0xFEAC) base = 0x0630;
        else if (cp >= 0xFEAD && cp <= 0xFEAE) base = 0x0631;
        else if (cp >= 0xFEAF && cp <= 0xFEB0) base = 0x0632;
        else if (cp >= 0xFEB1 && cp <= 0xFEB4) base = 0x0633;
        else if (cp >= 0xFEB5 && cp <= 0xFEB8) base = 0x0634;
        else if (cp >= 0xFEB9 && cp <= 0xFEBC) base = 0x0635;
        else if (cp >= 0xFEBD && cp <= 0xFEC0) base = 0x0636;
        else if (cp >= 0xFEC1 && cp <= 0xFEC4) base = 0x0637;
        else if (cp >= 0xFEC5 && cp <= 0xFEC8) base = 0x0638;
        else if (cp >= 0xFEC9 && cp <= 0xFECC) base = 0x0639;
        else if (cp >= 0xFECD && cp <= 0xFED0) base = 0x063A;
        else if (cp >= 0xFED1 && cp <= 0xFED4) base = 0x0641;
        else if (cp >= 0xFED5 && cp <= 0xFED8) base = 0x0642;
        else if (cp >= 0xFED9 && cp <= 0xFEDC) base = 0x0643;
        else if (cp >= 0xFEDD && cp <= 0xFEE0) base = 0x0644;
        else if (cp >= 0xFEE1 && cp <= 0xFEE4) base = 0x0645;
        else if (cp >= 0xFEE5 && cp <= 0xFEE8) base = 0x0646;
        else if (cp >= 0xFEE9 && cp <= 0xFEEC) base = 0x0647;
        else if (cp >= 0xFEED && cp <= 0xFEEE) base = 0x0648;
        else if (cp >= 0xFEEF && cp <= 0xFEF0) base = 0x0649;
        else if (cp >= 0xFEF1 && cp <= 0xFEF4) base = 0x064A;
        if (base) return unicode_to_cp864(base);
    }

    return 0;
}

/* Cyrillic + CP866-specific symbols -> CP866 byte. Returns 0 if not in CP866. */
static unsigned char unicode_to_cp866(unsigned int cp)
{
    /* Cyrillic uppercase A..YA */
    if (cp >= 0x0410 && cp <= 0x042F) return (unsigned char)(0x80 + (cp - 0x0410));
    /* Cyrillic lowercase a..p (split: 0xA0..0xAF for first half, 0xE0..0xEF for second) */
    if (cp >= 0x0430 && cp <= 0x043F) return (unsigned char)(0xA0 + (cp - 0x0430));
    if (cp >= 0x0440 && cp <= 0x044F) return (unsigned char)(0xE0 + (cp - 0x0440));
    /* Yo */
    if (cp == 0x0401) return 0xF0;
    if (cp == 0x0451) return 0xF1;
    /* Ukrainian Ye, Yi; Belarusian Short U */
    if (cp == 0x0404) return 0xF2;
    if (cp == 0x0454) return 0xF3;
    if (cp == 0x0407) return 0xF4;
    if (cp == 0x0457) return 0xF5;
    if (cp == 0x040E) return 0xF6;
    if (cp == 0x045E) return 0xF7;
    /* CP866 graphics tail */
    if (cp == 0x00B0) return 0xF8;
    if (cp == 0x2219) return 0xF9;
    if (cp == 0x00B7) return 0xFA;
    if (cp == 0x221A) return 0xFB;
    if (cp == 0x2116) return 0xFC;
    if (cp == 0x00A4) return 0xFD;
    if (cp == 0x25A0) return 0xFE;
    if (cp == 0x00A0) return 0xFF;
    /* Box drawing: cover the canonical Unicode codepoints used at
     * CP866/CP437 0xB0..0xDF. Picky one-by-one rather than a range
     * because Unicode's box-drawing block isn't ordered the same way. */
    switch (cp) {
        case 0x2591: return 0xB0; case 0x2592: return 0xB1; case 0x2593: return 0xB2;
        case 0x2502: return 0xB3; case 0x2524: return 0xB4; case 0x2561: return 0xB5;
        case 0x2562: return 0xB6; case 0x2556: return 0xB7; case 0x2555: return 0xB8;
        case 0x2563: return 0xB9; case 0x2551: return 0xBA; case 0x2557: return 0xBB;
        case 0x255D: return 0xBC; case 0x255C: return 0xBD; case 0x255B: return 0xBE;
        case 0x2510: return 0xBF; case 0x2514: return 0xC0; case 0x2534: return 0xC1;
        case 0x252C: return 0xC2; case 0x251C: return 0xC3; case 0x2500: return 0xC4;
        case 0x253C: return 0xC5; case 0x255E: return 0xC6; case 0x255F: return 0xC7;
        case 0x255A: return 0xC8; case 0x2554: return 0xC9; case 0x2569: return 0xCA;
        case 0x2566: return 0xCB; case 0x2560: return 0xCC; case 0x2550: return 0xCD;
        case 0x256C: return 0xCE; case 0x2567: return 0xCF; case 0x2568: return 0xD0;
        case 0x2564: return 0xD1; case 0x2565: return 0xD2; case 0x2559: return 0xD3;
        case 0x2558: return 0xD4; case 0x2552: return 0xD5; case 0x2553: return 0xD6;
        case 0x256B: return 0xD7; case 0x256A: return 0xD8; case 0x2518: return 0xD9;
        case 0x250C: return 0xDA; case 0x2588: return 0xDB; case 0x2584: return 0xDC;
        case 0x258C: return 0xDD; case 0x2590: return 0xDE; case 0x2580: return 0xDF;
    }
    return 0;
}

/* ========================================================================
 * UTF-8 decoder
 * ======================================================================== */

/* Decode one codepoint at buf[pos]. Returns U+FFFD on error.
 * On return *consumed = bytes used (always >= 1 if pos < len). */
static unsigned int utf8_decode(const unsigned char *buf, int len, int pos,
                                int *consumed)
{
    unsigned char b0;
    unsigned int cp;
    int n, i;

    if (pos >= len) { *consumed = 0; return 0; }
    b0 = buf[pos];
    if (b0 < 0x80) { *consumed = 1; return b0; }

    if      ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; n = 2; }
    else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; n = 3; }
    else if ((b0 & 0xF8) == 0xF0) { cp = b0 & 0x07; n = 4; }
    else                          { *consumed = 1; return 0xFFFD; }

    if (pos + n > len) { *consumed = 1; return 0xFFFD; }
    for (i = 1; i < n; i++) {
        unsigned char b = buf[pos + i];
        if ((b & 0xC0) != 0x80) { *consumed = 1; return 0xFFFD; }
        cp = (cp << 6) | (b & 0x3F);
    }
    *consumed = n;
    return cp;
}

/* ========================================================================
 * UTF-8 line -> OpenQT line conversion
 * ======================================================================== */

/* Returns number of bytes written to out[]. */
static int convert_utf8_line(const unsigned char *in, int in_len,
                             unsigned char *out, int out_max)
{
    int pos = 0, op = 0;
    unsigned int cp;
    int consumed;
    unsigned char b;

    while (pos < in_len && op < out_max - 1) {
        /* Markdown formatting markers (consume two ASCII bytes).
         * Single * or _ falls through to ASCII passthrough. */
        if (pos + 1 < in_len && in[pos] == '*' && in[pos + 1] == '*') {
            out[op++] = FMT_BOLD;
            pos += 2;
            continue;
        }
        if (pos + 1 < in_len && in[pos] == '_' && in[pos + 1] == '_') {
            out[op++] = FMT_UNDERLINE;
            pos += 2;
            continue;
        }

        cp = utf8_decode(in, in_len, pos, &consumed);
        pos += consumed;

        if (cp == 0xFFFD || cp == 0xFEFF) {
            /* Replacement char and stray BOMs: drop. */
            continue;
        }

        if (cp < 0x80) {
            out[op++] = (unsigned char)cp;                    /* ASCII */
        } else if (russian_mode) {
            if ((b = unicode_to_cp866((unsigned int)cp)) != 0) {
                out[op++] = b;                                /* Russian */
            }
            /* else: codepoint not representable in CP866 — skip. */
        } else if ((b = unicode_to_cp862((unsigned int)cp)) != 0) {
            out[op++] = b;                                    /* Hebrew */
        } else if ((b = unicode_to_cp864((unsigned int)cp)) != 0) {
            out[op++] = b;                                    /* Arabic */
        }
        /* else: codepoint not representable — silently skip. */
    }
    return op;
}

/* ========================================================================
 * Legacy DOS mode: visual-to-logical reversal (Hebrew + Arabic)
 * ======================================================================== */

static int is_dos_rtl(unsigned char ch)
{
    return (ch >= HEB_FIRST  && ch <= HEB_LAST) ||
           (ch >= ARAB_FIRST && ch <= ARAB_LAST);
}

static void reverse_range(unsigned char *buf, int s, int e)
{
    while (s < e) {
        unsigned char t = buf[s]; buf[s] = buf[e]; buf[e] = t;
        s++; e--;
    }
}

/* Reverse the line, then re-reverse non-RTL runs to put English/digits
 * back in LTR order. Mirrors the original txt2oqt logic, but accepts
 * Arabic CP864 bytes alongside Hebrew CP862.
 *
 * Russian is LTR (CP866 is not flipped on disk in OpenQT) - this
 * function returns immediately when russian_mode is on. The
 * is_dos_rtl predicate would also misclassify Cyrillic letters as
 * RTL because their byte values overlap CP862/CP864. */
static void dos_visual_to_logical(unsigned char *line, int len)
{
    int i, run_start, has_rtl = 0;

    if (russian_mode) return;

    for (i = 0; i < len; i++) {
        if (is_dos_rtl(line[i])) { has_rtl = 1; break; }
    }
    if (!has_rtl || keep_visual) return;

    reverse_range(line, 0, len - 1);

    i = 0;
    while (i < len) {
        while (i < len && is_dos_rtl(line[i])) i++;
        if (i >= len) break;
        run_start = i;
        while (i < len && !is_dos_rtl(line[i])) i++;
        if (i > run_start) {
            int has_content = 0, j;
            for (j = run_start; j < i; j++) {
                if (line[j] != ' ') { has_content = 1; break; }
            }
            if (has_content) reverse_range(line, run_start, i - 1);
        }
    }
}

/* ========================================================================
 * Word-wrapped output writer
 * ======================================================================== */

static void write_wrapped(FILE *fout, const unsigned char *line, int len,
                          int *line_count)
{
    int pos = 0, wrap, i;

    if (len == 0) {
        fprintf(fout, "\r\n");
        (*line_count)++;
        return;
    }

    while (pos < len) {
        if (len - pos <= TEXT_WIDTH) {
            fwrite(line + pos, 1, len - pos, fout);
            fputs("\r\n", fout);
            (*line_count)++;
            return;
        }
        wrap = TEXT_WIDTH;
        for (i = pos + TEXT_WIDTH; i > pos; i--) {
            if (line[i] == ' ') { wrap = i - pos; break; }
        }
        if (i == pos) wrap = TEXT_WIDTH;
        fwrite(line + pos, 1, wrap, fout);
        fputs("\r\n", fout);
        (*line_count)++;
        pos += wrap;
        while (pos < len && line[pos] == ' ') pos++;
    }
}

/* ========================================================================
 * Mode auto-detection and main loop
 * ======================================================================== */

static int detect_utf8_bom(FILE *f)
{
    unsigned char bom[3];
    int got = fread(bom, 1, 3, f);
    if (got == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        return 1; /* BOM consumed; leave file pointer past it */
    }
    rewind(f);
    return 0;
}

static void process_file(const char *in_file, const char *out_file)
{
    FILE *fin, *fout;
    unsigned char raw[MAX_LINE * 4];
    unsigned char out_line[MAX_LINE];
    int line_count = 0, input_lines = 0;

    fin = fopen(in_file, "rb");
    if (!fin) { printf("Error: cannot open '%s'\n", in_file); return; }
    fout = fopen(out_file, "wb");
    if (!fout) { printf("Error: cannot create '%s'\n", out_file);
                 fclose(fin); return; }

    if (force_utf8) {
        input_is_utf8 = 1;
        detect_utf8_bom(fin);   /* consume BOM if present, else rewind */
    } else if (force_dos) {
        input_is_utf8 = 0;
    } else {
        input_is_utf8 = detect_utf8_bom(fin);
    }

    printf("Mode:   %s%s\n",
           input_is_utf8 ? "UTF-8" : "Legacy DOS",
           russian_mode  ? " (target CP866 Russian)" : "");
    if (!input_is_utf8) {
        printf("Order:  %s\n", russian_mode ? "Logical (Russian is LTR)"
                              : keep_visual ? "Visual (preserved)"
                              :                "Visual -> Logical");
    }
    printf("Wrap:  %d columns\n\n", TEXT_WIDTH);

    while (fgets((char *)raw, sizeof(raw), fin)) {
        int raw_len = strlen((char *)raw);
        int out_len;

        while (raw_len > 0 && (raw[raw_len - 1] == '\n' ||
                               raw[raw_len - 1] == '\r')) {
            raw[--raw_len] = '\0';
        }
        input_lines++;

        if (input_is_utf8) {
            out_len = convert_utf8_line(raw, raw_len,
                                        out_line, sizeof(out_line));
        } else {
            if (raw_len > (int)sizeof(out_line) - 1)
                raw_len = sizeof(out_line) - 1;
            memcpy(out_line, raw, raw_len);
            dos_visual_to_logical(out_line, raw_len);
            out_len = raw_len;
        }
        write_wrapped(fout, out_line, out_len, &line_count);
    }

    fclose(fin);
    fclose(fout);

    printf("Done!\n");
    printf("Input:  %d lines\n", input_lines);
    printf("Output: %d lines\n", line_count);
    if (input_is_utf8) {
        if (russian_mode) {
            printf("\nOpen in OpenQT via OQTR (Russian / CP866).\n");
        } else {
            printf("\nOpen in OpenQT (Hebrew via OQTH, Arabic via OQTA).\n");
        }
    } else if (russian_mode) {
        printf("\nOpen in OpenQT via OQTR (Russian, no order reversal).\n");
    } else if (keep_visual) {
        printf("\nOpen in OpenQT with RTL mode ON (F5).\n");
    } else {
        printf("\nOpen in OpenQT (RTL mode set automatically by language).\n");
    }
}

int main(int argc, char *argv[])
{
    char *in_file = NULL, *out_file = NULL;
    int i;

    printf("TXT2OQT - UTF-8/DOS to OpenQT Converter\n");
    printf("by Ronen Blumberg\n");
    printf("=======================================\n\n");

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '/' || argv[i][0] == '-') {
            char c = argv[i][1];
            if      (c == 'V' || c == 'v') keep_visual = 1;
            else if (c == 'D' || c == 'd') force_dos   = 1;
            else if (c == 'U' || c == 'u') force_utf8  = 1;
            else if (c == 'R' || c == 'r') russian_mode = 1;
        } else if (!in_file)  in_file  = argv[i];
        else  if (!out_file)  out_file = argv[i];
    }

    if (!in_file || !out_file) {
        printf("Usage: txt2oqt <input.txt> <output.txt> [/V] [/D] [/U] [/R]\n\n");
        printf("Default: auto-detects UTF-8 (via BOM) vs legacy DOS encoding.\n\n");
        printf("Options:\n");
        printf("  /V   Keep visual order (legacy DOS mode only)\n");
        printf("  /D   Force legacy DOS encoding mode\n");
        printf("  /U   Force UTF-8 mode (skip BOM auto-detect)\n");
        printf("  /R   Target OpenQT Russian (CP866) instead of Hebrew/Arabic\n\n");
        printf("UTF-8 mode handles:\n");
        printf("  - Hebrew  (U+05D0..U+05EA)\n");
        printf("  - Arabic  (basic block, digits, punctuation, ligatures)\n");
        printf("  - Russian (Cyrillic + Yo + box-drawing) with /R\n");
        printf("  - English (ASCII passthrough)\n");
        printf("  - Markdown formatting:  **bold**  __underline__\n\n");
        printf("Legacy DOS mode handles CP862 Hebrew + CP864 Arabic; with /R\n");
        printf("treats input as CP866 (no order reversal - Russian is LTR).\n");
        return 1;
    }

    process_file(in_file, out_file);
    return 0;
}
