/*
 * OQT2WORD - OpenQT to Word Converter
 * Converts OpenQT files to UTF-8 for Microsoft Word.
 *   - Hebrew letters (CP862, 0x80..0x9A) -> U+05D0..U+05EA
 *   - Arabic letters (CP864, 0xC1..0xDA, 0xE0..0xFE) -> U+0621.. (Arabic block)
 *   - /R flag: file was written in OpenQT Russian mode (CP866). Cyrillic
 *     letters -> U+0410..U+044F, Yo/Ye/Yi/Short-U -> their codepoints,
 *     box-drawing -> U+2500.. block, No./currency -> U+2116/U+00A4.
 *     CP866 byte ranges overlap CP862 Hebrew + CP864 Arabic, so this
 *     flag is REQUIRED for Russian files - the converter cannot detect
 *     the source codepage from byte values alone.
 *
 * Usage: oqt2word [/R] input.txt output.txt
 *
 * Compile with Watcom: wcl386 -bt=dos -l=dos4g oqt2word.c -fe=oqt2word.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

/* CP862 Hebrew to Unicode mapping (0x80-0x9A) */
unsigned int cp862_to_unicode[27] = {
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

/* CP864 Arabic byte -> Unicode codepoint, full table indexed by
 * (byte - 0xA0). Maps to the BASIC ARABIC BLOCK (U+0621..U+064A,
 * U+0660..U+0669) so that LibreOffice / Word can do proper positional
 * shaping and cursive joining. 0 = non-Arabic byte (skip).
 * The CP864 byte is treated as the abstract letter regardless of which
 * positional form (initial/medial/final/isolated) the codepoint
 * originally encoded. Lam-Alef ligatures expand to two codepoints,
 * handled separately by the caller. */
#define LAMALEF_PAIR 0xFFFE  /* sentinel: byte expands to U+0644 + alef-variant */

static const unsigned int cp864_to_unicode[96] = {
    /* 0xA0 */ 0x00A0, /* NO-BREAK SPACE */
    /* 0xA1 */ 0x00AD, /* SOFT HYPHEN */
    /* 0xA2 */ 0x0622, /* Alef with Madda above */
    /* 0xA3 */ 0x00A3, /* POUND */
    /* 0xA4 */ 0x00A4, /* CURRENCY */
    /* 0xA5 */ 0x0623, /* Alef with Hamza above */
    /* 0xA6 */ 0,
    /* 0xA7 */ 0,
    /* 0xA8 */ 0x0627, /* Alef */
    /* 0xA9 */ 0x0628, /* Beh */
    /* 0xAA */ 0x062A, /* Teh */
    /* 0xAB */ 0x062B, /* Theh */
    /* 0xAC */ 0x060C, /* Arabic comma */
    /* 0xAD */ 0x062C, /* Jeem */
    /* 0xAE */ 0x062D, /* Hah */
    /* 0xAF */ 0x062E, /* Khah */
    /* 0xB0 */ 0x0660, /* digit 0 */
    /* 0xB1 */ 0x0661,
    /* 0xB2 */ 0x0662,
    /* 0xB3 */ 0x0663,
    /* 0xB4 */ 0x0664,
    /* 0xB5 */ 0x0665,
    /* 0xB6 */ 0x0666,
    /* 0xB7 */ 0x0667,
    /* 0xB8 */ 0x0668,
    /* 0xB9 */ 0x0669, /* digit 9 */
    /* 0xBA */ 0x0641, /* Feh */
    /* 0xBB */ 0x061B, /* Arabic semicolon */
    /* 0xBC */ 0x0633, /* Seen */
    /* 0xBD */ 0x0634, /* Sheen */
    /* 0xBE */ 0x0635, /* Sad */
    /* 0xBF */ 0x061F, /* Arabic question mark */
    /* 0xC0 */ 0x00A2, /* CENT */
    /* 0xC1 */ 0x0621, /* Hamza */
    /* 0xC2 */ 0x0622, /* Alef-Madda */
    /* 0xC3 */ 0x0623, /* Alef-Hamza above */
    /* 0xC4 */ 0x0624, /* Waw-Hamza */
    /* 0xC5 */ 0x0639, /* Ain (final-form glyph in this font; CP864 0xC5
                          is documented as Alef-Hamza-below, but the
                          embedded FreeDOS ega17 font carries Ain-final
                          at this codepoint, which is what OpenQT v3.2
                          shaping emits) */
    /* 0xC6 */ 0x0626, /* Yeh-Hamza */
    /* 0xC7 */ 0x0627, /* Alef */
    /* 0xC8 */ 0x0628, /* Beh */
    /* 0xC9 */ 0x0629, /* Teh Marbuta */
    /* 0xCA */ 0x062A, /* Teh */
    /* 0xCB */ 0x062B, /* Theh */
    /* 0xCC */ 0x062C, /* Jeem */
    /* 0xCD */ 0x062D, /* Hah */
    /* 0xCE */ 0x062E, /* Khah */
    /* 0xCF */ 0x062F, /* Dal */
    /* 0xD0 */ 0x0630, /* Thal */
    /* 0xD1 */ 0x0631, /* Reh */
    /* 0xD2 */ 0x0632, /* Zain */
    /* 0xD3 */ 0x0633, /* Seen */
    /* 0xD4 */ 0x0634, /* Sheen */
    /* 0xD5 */ 0x0635, /* Sad */
    /* 0xD6 */ 0x0636, /* Dad */
    /* 0xD7 */ 0x0637, /* Tah */
    /* 0xD8 */ 0x0638, /* Zah */
    /* 0xD9 */ 0x0639, /* Ain */
    /* 0xDA */ 0x063A, /* Ghain */
    /* 0xDB */ 0x00A6, /* broken bar */
    /* 0xDC */ 0x00AC, /* not sign */
    /* 0xDD */ 0x00F7, /* division */
    /* 0xDE */ 0x00D7, /* multiplication */
    /* 0xDF */ 0x0639, /* Ain */
    /* 0xE0 */ 0x0640, /* Tatweel */
    /* 0xE1 */ 0x0641, /* Feh */
    /* 0xE2 */ 0x0642, /* Qaf */
    /* 0xE3 */ 0x0643, /* Kaf */
    /* 0xE4 */ 0x0644, /* Lam */
    /* 0xE5 */ 0x0645, /* Meem */
    /* 0xE6 */ 0x0646, /* Noon */
    /* 0xE7 */ 0x0647, /* Heh */
    /* 0xE8 */ 0x0648, /* Waw */
    /* 0xE9 */ 0x0649, /* Alef-Maksura */
    /* 0xEA */ 0x064A, /* Yeh */
    /* 0xEB */ 0x0636, /* Dad */
    /* 0xEC */ 0x0639, /* Ain */
    /* 0xED */ 0x063A, /* Ghain */
    /* 0xEE */ 0x063A, /* Ghain */
    /* 0xEF */ 0x0645, /* Meem */
    /* 0xF0 */ 0x0651, /* Shadda */
    /* 0xF1 */ 0x0651, /* Shadda */
    /* 0xF2 */ 0x0646, /* Noon */
    /* 0xF3 */ 0x0647, /* Heh */
    /* 0xF4 */ 0x0647, /* Heh */
    /* 0xF5 */ 0x0649, /* Alef-Maksura */
    /* 0xF6 */ 0x064A, /* Yeh */
    /* 0xF7 */ 0x063A, /* Ghain */
    /* 0xF8 */ 0x0642, /* Qaf */
    /* 0xF9 */ LAMALEF_PAIR, /* Lam-Alef-Madda ligature  -> U+0644 U+0622 */
    /* 0xFA */ LAMALEF_PAIR, /* Lam-Alef-Madda ligature  -> U+0644 U+0622 */
    /* 0xFB */ 0x0644, /* Lam */
    /* 0xFC */ 0x0643, /* Kaf */
    /* 0xFD */ 0x064A, /* Yeh */
    /* 0xFE */ 0xFFFD, /* Lam-Alef ligature -> U+0644 U+0627 (handled below) */
    /* 0xFF */ 0
};

#define LAMALEF_PLAIN 0xFFFD  /* sentinel: 0xFE -> U+0644 + U+0627 */

/* CP866 byte -> Unicode codepoint, full table for 0x80..0xFF.
 * Used when /R is given so a CP866 byte stream can be decoded
 * unambiguously (its high-byte ranges overlap both CP862 Hebrew at
 * 0x80-0x9A and CP864 Arabic at 0xA0-0xFE). */
static const unsigned int cp866_to_unicode[128] = {
    /* 0x80 */ 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    /* 0x88 */ 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    /* 0x90 */ 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    /* 0x98 */ 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    /* 0xA0 */ 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    /* 0xA8 */ 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    /* 0xB0 */ 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    /* 0xB8 */ 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    /* 0xC0 */ 0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    /* 0xC8 */ 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    /* 0xD0 */ 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    /* 0xD8 */ 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    /* 0xE0 */ 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    /* 0xE8 */ 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
    /* 0xF0 */ 0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E,
    /* 0xF8 */ 0x00B0, 0x2219, 0x00B7, 0x221A, 0x2116, 0x00A4, 0x25A0, 0x00A0
};

static int g_russian_mode = 0;

/* Write UTF-8 encoded character to buffer, return bytes written */
int encode_utf8(unsigned char *buf, unsigned int codepoint)
{
    if (codepoint < 0x80) {
        buf[0] = codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        buf[0] = 0xC0 | (codepoint >> 6);
        buf[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint < 0x10000) {
        buf[0] = 0xE0 | (codepoint >> 12);
        buf[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        buf[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    }
    return 0;
}

/* Check if character is Hebrew (CP862) */
int is_hebrew(unsigned char ch)
{
    return (ch >= 0x80 && ch <= 0x9A);
}

/* Check if character is in the OpenQT Arabic (CP864) range */
int is_arabic(unsigned char ch)
{
    return (ch >= 0xA0) && (cp864_to_unicode[ch - 0xA0] != 0);
}

/* Check if character is format code */
int is_format_code(unsigned char ch)
{
    return (ch == 0x02 || ch == 0x03 || ch == 0x04);
}

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char line[MAX_LINE];
    unsigned char output[MAX_LINE * 3];
    int i, len, out_pos;
    int line_count = 0;
    unsigned char ch;
    unsigned int unicode;
    char *in_path = NULL, *out_path = NULL;

    printf("OQT2WORD - OpenQT to Word Converter\n");
    printf("===================================\n\n");

    for (i = 1; i < argc; i++) {
        if ((argv[i][0] == '/' || argv[i][0] == '-') &&
            (argv[i][1] == 'R' || argv[i][1] == 'r') && argv[i][2] == '\0') {
            g_russian_mode = 1;
        } else if (!in_path) {
            in_path = argv[i];
        } else if (!out_path) {
            out_path = argv[i];
        }
    }

    if (!in_path || !out_path) {
        printf("Usage: oqt2word [/R] input.txt output.txt\n\n");
        printf("Converts OpenQT files to UTF-8 format for Microsoft Word.\n");
        printf("Hebrew/Arabic text will display correctly in Word.\n\n");
        printf("  /R   input file is OpenQT Russian (CP866) - use this for\n");
        printf("       any document created in OQTR / OQT R / openqt /R mode.\n");
        return 1;
    }

    fin = fopen(in_path, "rb");
    if (!fin) {
        printf("Error: Cannot open input file '%s'\n", in_path);
        return 1;
    }

    fout = fopen(out_path, "wb");
    if (!fout) {
        printf("Error: Cannot create output file '%s'\n", out_path);
        fclose(fin);
        return 1;
    }
    
    /* Write UTF-8 BOM for Word to recognize encoding */
    fputc(0xEF, fout);
    fputc(0xBB, fout);
    fputc(0xBF, fout);
    
    printf("Mode: %s\n", g_russian_mode ? "Russian (CP866)" : "Hebrew + Arabic");
    printf("Converting '%s' to '%s'...\n", in_path, out_path);
    
    while (fgets((char *)line, sizeof(line), fin)) {
        len = strlen((char *)line);
        
        /* Remove CR/LF at end */
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
            len--;
        }
        
        /* Convert to UTF-8 */
        out_pos = 0;
        for (i = 0; i < len; i++) {
            ch = line[i];
            
            /* Skip format codes */
            if (is_format_code(ch)) {
                continue;
            }
            
            if (g_russian_mode) {
                if (ch >= 0x80) {
                    /* CP866: every high byte has a Unicode equivalent
                     * (Cyrillic letter, box-drawing, or graphics). */
                    unicode = cp866_to_unicode[ch - 0x80];
                    if (unicode) out_pos += encode_utf8(output + out_pos, unicode);
                } else if (ch >= 32 && ch < 128) {
                    output[out_pos++] = ch;                /* ASCII */
                } else if (ch == '\t') {
                    output[out_pos++] = '\t';
                }
            } else if (is_hebrew(ch)) {
                /* Convert CP862 Hebrew to Unicode */
                unicode = cp862_to_unicode[ch - 0x80];
                out_pos += encode_utf8(output + out_pos, unicode);
            } else if (is_arabic(ch)) {
                /* Convert CP864 Arabic to Unicode (basic Arabic block,
                 * so the renderer can shape and join the letters). */
                unicode = cp864_to_unicode[ch - 0xA0];
                if (unicode == LAMALEF_PAIR) {
                    /* Lam-Alef-Madda ligature: U+0644 + U+0622 */
                    out_pos += encode_utf8(output + out_pos, 0x0644);
                    out_pos += encode_utf8(output + out_pos, 0x0622);
                } else if (unicode == LAMALEF_PLAIN) {
                    /* Lam-Alef ligature: U+0644 + U+0627 */
                    out_pos += encode_utf8(output + out_pos, 0x0644);
                    out_pos += encode_utf8(output + out_pos, 0x0627);
                } else {
                    out_pos += encode_utf8(output + out_pos, unicode);
                }
            } else if (ch >= 32 && ch < 128) {
                /* ASCII - write as-is */
                output[out_pos++] = ch;
            } else if (ch == '\t') {
                output[out_pos++] = '\t';
            }
        }
        
        /* Write line */
        fwrite(output, 1, out_pos, fout);
        fputc('\r', fout);
        fputc('\n', fout);
        
        line_count++;
    }
    
    fclose(fin);
    fclose(fout);
    
    printf("Converted %d lines.\n", line_count);
    printf("Open '%s' in Microsoft Word.\n", out_path);
    
    return 0;
}
