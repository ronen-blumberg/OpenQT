/*
 * TXT2RTF - OpenQT to RTF Converter (Hebrew + English)
 *
 * Converts an OpenQT document to Rich Text Format (.rtf) so it opens with
 * full formatting in Microsoft Word, LibreOffice, WordPad, etc. This tool
 * is intentionally scoped to HEBREW + ENGLISH files only (the OQT / OQT H
 * launchers); Arabic (CP864) and Russian (CP866) are not handled here.
 *
 *   - ASCII (0x20..0x7E)        -> emitted directly (RTF specials escaped)
 *   - Hebrew letters (CP862,
 *     0x80..0x9A)               -> \uNNNN? Unicode escapes (U+05D0..U+05EA),
 *                                  marked as right-to-left runs so Word does
 *                                  its own BiDi reordering.
 *   - Inline formatting toggles -> RTF \b / \ul runs:
 *         FMT_BOLD      (0x02) -> bold
 *         FMT_UNDERLINE (0x03) -> underline
 *         FMT_BOLDUNDER (0x04) -> bold + underline
 *     These are STATEFUL toggles, exactly as openqt.c interprets them
 *     (a single current-attribute that is one of NORMAL/BOLD/UNDERLINE/
 *     BOLDUNDER), and the attribute RESETS at the start of every line.
 *     QText 5.5 markers FMT_START (0xAF) / FMT_END (0xAE) are also honoured
 *     on read as a generic bold span, for compatibility.
 *
 * The OpenQT file stores text in LOGICAL order; RTF is also logical order,
 * so no reordering is done here - we only tag Hebrew runs \rtlch and Latin
 * runs \ltrch and let the word processor's BiDi engine lay them out.
 *
 * Usage: txt2rtf input.txt output.rtf
 *
 * Compile with Watcom: wcl386 -bt=dos -l=dos4g txt2rtf.c -fe=txt2rtf.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

/* OpenQT inline format toggle bytes (see openqt.c) */
#define FMT_BOLD       0x02
#define FMT_UNDERLINE  0x03
#define FMT_BOLDUNDER  0x04
#define FMT_START      0xAF   /* QText 5.5 emphasis start (treated as bold) */
#define FMT_END        0xAE   /* QText 5.5 emphasis end */

/* Current-attribute states, mirroring openqt.c's CLR_* values */
#define ATTR_NORMAL    0
#define ATTR_BOLD      1
#define ATTR_UNDERLINE 2
#define ATTR_BOLDUNDER 3

#define ENC_MAGIC "OQT-ENC1"

/* CP862 Hebrew to Unicode mapping (0x80-0x9A) */
static const unsigned int cp862_to_unicode[27] = {
    0x05D0, /* 0x80 = Alef */
    0x05D1, /* 0x81 = Bet */
    0x05D2, /* 0x82 = Gimel */
    0x05D3, /* 0x83 = Dalet */
    0x05D4, /* 0x84 = He */
    0x05D5, /* 0x85 = Vav */
    0x05D6, /* 0x86 = Zayin */
    0x05D7, /* 0x87 = Het */
    0x05D8, /* 0x88 = Tet */
    0x05D9, /* 0x89 = Yod */
    0x05DA, /* 0x8A = Kaf Sofit */
    0x05DB, /* 0x8B = Kaf */
    0x05DC, /* 0x8C = Lamed */
    0x05DD, /* 0x8D = Mem Sofit */
    0x05DE, /* 0x8E = Mem */
    0x05DF, /* 0x8F = Nun Sofit */
    0x05E0, /* 0x90 = Nun */
    0x05E1, /* 0x91 = Samekh */
    0x05E2, /* 0x92 = Ayin */
    0x05E3, /* 0x93 = Pe Sofit */
    0x05E4, /* 0x94 = Pe */
    0x05E5, /* 0x95 = Tsadi Sofit */
    0x05E6, /* 0x96 = Tsadi */
    0x05E7, /* 0x97 = Qof */
    0x05E8, /* 0x98 = Resh */
    0x05E9, /* 0x99 = Shin */
    0x05EA  /* 0x9A = Tav */
};

static int is_hebrew(unsigned char ch)
{
    return (ch >= 0x80 && ch <= 0x9A);
}

/* Direction of a run. -1 = none yet, 0 = LTR, 1 = RTL.
 * Neutrals (space / punctuation) keep the current direction. */
#define DIR_NONE -1
#define DIR_LTR   0
#define DIR_RTL   1

/* Open a fresh formatting group: '{', then the run direction + font, then
 * the attribute control words. Each styled run lives in its own {...} group
 * so formatting auto-resets at the closing brace - this is the pattern Word
 * itself emits and it avoids any cross-run bleed.
 *
 * Order matters: the direction/font (\rtlch / \ltrch) must come BEFORE the
 * bold/underline words, otherwise the reader binds the style to the wrong
 * run and bold lands on the next run instead of this one.
 *
 * Bold and underline are written in BOTH their normal form (\b, \ul) and
 * their "associated" form (\ab, \aul): RTF uses the normal properties for
 * left-to-right (\ltrch) runs and the ASSOCIATED properties for
 * right-to-left (\rtlch) runs, so without \ab bold is invisible on Hebrew.
 * Emitting both is correct in either direction; a reader that doesn't know
 * an associated keyword ignores it. */
static void open_run_group(FILE *fout, int dir, int attr)
{
    fputc('{', fout);
    fputs(dir == DIR_RTL ? "\\rtlch\\af1\\f1 " : "\\ltrch\\f0 ", fout);
    if (attr == ATTR_BOLD || attr == ATTR_BOLDUNDER)
        fputs("\\b\\ab ", fout);
    if (attr == ATTR_UNDERLINE || attr == ATTR_BOLDUNDER)
        fputs("\\ul\\aul ", fout);
}

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char line[MAX_LINE];
    int i, len;
    int line_count = 0;
    char *in_path = NULL, *out_path = NULL;
    unsigned char magic[8];
    size_t got;

    printf("TXT2RTF - OpenQT to RTF Converter (Hebrew + English)\n");
    printf("====================================================\n\n");

    for (i = 1; i < argc; i++) {
        if (!in_path) in_path = argv[i];
        else if (!out_path) out_path = argv[i];
    }

    if (!in_path || !out_path) {
        printf("Usage: txt2rtf input.txt output.rtf\n\n");
        printf("Converts an OpenQT Hebrew/English document to RTF, preserving\n");
        printf("bold, underline and bold+underline runs. Open the result in\n");
        printf("Microsoft Word, LibreOffice or WordPad.\n\n");
        printf("Note: Hebrew+English only. Use a different export for Arabic\n");
        printf("(CP864) or Russian (CP866) documents.\n");
        return 1;
    }

    fin = fopen(in_path, "rb");
    if (!fin) {
        printf("Error: Cannot open input file '%s'\n", in_path);
        return 1;
    }

    /* Refuse encrypted files - the cipher would have to be undone first. */
    got = fread(magic, 1, 8, fin);
    if (got == 8 && memcmp(magic, ENC_MAGIC, 8) == 0) {
        printf("Error: '%s' is encrypted (OQT-ENC1).\n", in_path);
        printf("Open it in OpenQT, save without a password, then convert.\n");
        fclose(fin);
        return 1;
    }
    rewind(fin);

    fout = fopen(out_path, "wb");
    if (!fout) {
        printf("Error: Cannot create output file '%s'\n", out_path);
        fclose(fin);
        return 1;
    }

    /* RTF header: ANSI base codepage Hebrew (1255); f0 Latin, f1 Hebrew.
     * \uc1 = one fallback char follows each \u escape. */
    fputs("{\\rtf1\\ansi\\ansicpg1255\\deff0\\uc1\n", fout);
    fputs("{\\fonttbl{\\f0\\fnil\\fcharset0 Arial;}"
          "{\\f1\\fnil\\fcharset177 David;}}\n", fout);
    fputs("\\viewkind4\n", fout);

    printf("Converting '%s' to '%s'...\n", in_path, out_path);

    while (fgets((char *)line, sizeof(line), fin)) {
        int attr = ATTR_NORMAL;     /* current-attribute, resets each line */
        int dir = DIR_NONE;         /* direction of the open run group */
        int group_attr = ATTR_NORMAL;
        int group_open = 0;
        int has_heb = 0;

        len = strlen((char *)line);
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'))
            len--;

        /* Pre-scan: does this line contain Hebrew? Sets paragraph base dir. */
        for (i = 0; i < len; i++) {
            if (is_hebrew(line[i])) { has_heb = 1; break; }
        }

        fputs("\\pard", fout);
        fputs(has_heb ? "\\rtlpar\n" : "\\ltrpar\n", fout);

        for (i = 0; i < len; i++) {
            unsigned char ch = line[i];
            int want_dir, eff_dir;

            /* --- format toggles: update state, emit nothing yet --- */
            if (ch == FMT_BOLD) {
                attr = (attr == ATTR_BOLD) ? ATTR_NORMAL : ATTR_BOLD;
                continue;
            } else if (ch == FMT_UNDERLINE) {
                attr = (attr == ATTR_UNDERLINE) ? ATTR_NORMAL : ATTR_UNDERLINE;
                continue;
            } else if (ch == FMT_BOLDUNDER) {
                attr = (attr == ATTR_BOLDUNDER) ? ATTR_NORMAL : ATTR_BOLDUNDER;
                continue;
            } else if (ch == FMT_START) {
                attr = ATTR_BOLD;
                continue;
            } else if (ch == FMT_END) {
                attr = ATTR_NORMAL;
                continue;
            }

            /* Skip bytes we don't emit, so they never force a group open. */
            if (!is_hebrew(ch) && ch != '\t' &&
                !(ch >= 0x20 && ch <= 0x7E))
                continue;

            /* --- run direction (neutrals keep the current direction) --- */
            if (is_hebrew(ch))
                want_dir = DIR_RTL;
            else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                     (ch >= '0' && ch <= '9'))
                want_dir = DIR_LTR;
            else
                want_dir = DIR_NONE;   /* space / punctuation: sticky */

            eff_dir = want_dir;
            if (eff_dir == DIR_NONE)
                eff_dir = (dir == DIR_NONE) ? (has_heb ? DIR_RTL : DIR_LTR)
                                            : dir;

            /* Open a new group whenever direction or attribute changes. */
            if (!group_open || eff_dir != dir || attr != group_attr) {
                if (group_open) fputc('}', fout);
                open_run_group(fout, eff_dir, attr);
                dir = eff_dir;
                group_attr = attr;
                group_open = 1;
            }

            /* --- the character itself --- */
            if (is_hebrew(ch))
                fprintf(fout, "\\u%u ?", cp862_to_unicode[ch - 0x80]);
            else if (ch == '\t')
                fputs("\\tab ", fout);
            else if (ch == '\\' || ch == '{' || ch == '}') {
                fputc('\\', fout);
                fputc(ch, fout);
            } else
                fputc(ch, fout);
        }

        if (group_open) fputc('}', fout);
        fputs("\\par\n", fout);
        line_count++;
    }

    fputs("}\n", fout);

    fclose(fin);
    fclose(fout);

    printf("Converted %d lines.\n", line_count);
    printf("Open '%s' in Word / LibreOffice / WordPad.\n", out_path);
    return 0;
}
