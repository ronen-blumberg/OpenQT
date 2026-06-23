/*
 * OpenQT - Open Source Hebrew/English/Arabic/Russian Word Processor
 * A QText 5.5 Clone for DOS
 * Version 3.6.0
 *
 * Changes in 3.6:
 *   - New converter txt2rtf: exports a Hebrew + English document to RTF,
 *     preserving bold / underline / bold+underline runs so the styled text
 *     opens correctly in Microsoft Word, LibreOffice and WordPad.
 *
 * Changes in 3.4:
 *   - Host-assisted language features in the Tools menu (Alt+T), bridged to
 *     a Linux/DOSBox host daemon (host_helper/oqt_helper.py): English spell
 *     check (aspell, interactive), Read Aloud (espeak-ng), Stop Speech,
 *     Translate to Hebrew (online), and Dictate/speech-to-text (whisper).
 *     The editor exchanges raw OpenQT bytes with the daemon through files in
 *     C:\OPENQT\BRIDGE; all UTF-8 conversion happens host-side. Features are
 *     inert (show "Helper not responding") when the daemon isn't running.
 *
 * Changes in 3.3:
 *   - Russian (CP866) input mode added. F4 cycle is now ENG -> HEB ->
 *     ARA -> RUS. New RUSVGA.EXE uploads a CP866 8x16 font (CP437 base
 *     from HEBVGA + Cyrillic from X11 fixed-misc 8x13). Russian is its
 *     own session - CP866 byte ranges conflict with CP862 Hebrew at
 *     0x80-0x9A and CP864 Arabic at 0xA0-0xFE, so RUSVGA does not
 *     overlay; OQTR/OQT R is a Russian+English-only session. New /R
 *     command-line flag starts the editor in Russian mode.
 *
 * Changes in 3.2:
 *   - Full 4-form Arabic positional shaping. Yeh, Ain, Ghain, Heh
 *     now use their CP864 final/medial form bytes when context calls
 *     for them; Alef-Maksura and Lam-Alef-Madda ligature also pick up
 *     final-form joining. Same display-only architecture as 3.1.
 *
 * Changes in 3.1:
 *   - Arabic positional shaping on the DOS screen: dual-joining letters
 *     are rendered in their CP864 INITIAL form when followed by a
 *     letter that accepts a right-side join, producing visually
 *     connected (cursive) Arabic. Display-only; file storage is
 *     unchanged (still abstract isolated CP864 bytes).
 *
 * by Ronen Blumberg
 *
 * Compile with Watcom C (32-bit DOS4GW):
 *   wcl386 -bt=dos -l=dos4g openqt.c -fe=openqt.exe
 *
 * Launchers:
 *   OQTH file.txt   -> loads Hebrew VGA font (HEBVGA.COM), then runs OpenQT
 *   OQTA file.txt   -> loads Arabic VGA font (ARABVGA + VIDEO.CPI), then runs OpenQT
 *   OQTR file.txt   -> loads Russian VGA font (RUSVGA.EXE, CP866), then runs OpenQT
 *   OQT  H file.txt -> dispatcher form (H=Hebrew, A=Arabic, R=Russian, 3=trilingual)
 *
 * Encoding:
 *   - English/ASCII : CP437 (0x00-0x7F)
 *   - Hebrew letters: CP862 range 0x80-0x9A
 *   - Arabic letters: CP864 range 0xC1-0xDA, 0xE0-0xE9
 *   Note: CP864 reuses 0xC0-0xDF (where CP437 puts box-drawing); menu/dialog
 *   lines may render with Arabic glyphs while ARABVGA is loaded. Acceptable
 *   for now; fixable later by remapping the loaded font or by switching
 *   menu drawing to ASCII when input_lang==LANG_ARA.
 *
 * Features:
 *   - 4-language input (English / Hebrew / Arabic / Russian) — F4 cycles
 *   - Full BiDi support (Hebrew + Arabic + English + Numbers)
 *   - Bold, Underline formatting
 *   - Undo/Redo (250 levels)
 *   - Search/Replace with Hebrew/Arabic
 *   - Block operations
 *   - Save reminder (10 min)
 *   - Page counter
 *   - Up to 500 pages per document
 *   - Password protection (encryption)
 *
 * (c) 2025-2026 - Public Domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <malloc.h>
#include <ctype.h>
#include <time.h>
#include <direct.h>

#define VERSION         "3.6.0"
#define MAX_LINES       30000  /* ~500 pages */
#define MAX_LINE_LEN    256
#define SAVE_REMIND_SEC 600   /* Save reminder interval in seconds (600 = 10 min) */
#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   25
#define LINES_PER_PAGE  60    /* Lines per printed page (A4) */
#define EDIT_TOP        2
#define EDIT_BOTTOM     23
#define EDIT_HEIGHT     (EDIT_BOTTOM - EDIT_TOP + 1)
#define STATUS_LINE     24
#define TAB_SIZE        4
#define LEFT_MARGIN     5
#define RIGHT_MARGIN    75
#define TEXT_WIDTH      (RIGHT_MARGIN - LEFT_MARGIN + 1)

#define CLR_NORMAL      0x1F
#define CLR_STATUS      0x70
#define CLR_BOLD        0x1E  /* Yellow on blue */
#define CLR_UNDERLINE   0x1B  /* Cyan on blue */
#define CLR_BOLDUNDER   0x1C  /* Red on blue */

/* Formatting codes (stored in text) */
#define FMT_BOLD        0x02  /* Toggle bold */
#define FMT_UNDERLINE   0x03  /* Toggle underline */
#define FMT_BOLDUNDER   0x04  /* Toggle bold+underline */
#define CLR_MENU        0x70
#define CLR_MENU_HOT    0x74
#define CLR_MENU_SEL    0x0F
#define CLR_BLOCK       0x3F
#define CLR_TITLE       0x1E
#define CLR_DIALOG      0x4F
#define CLR_DIALOG_BTN  0x70

#define KEY_ESC         27
#define KEY_ENTER       13
#define KEY_BACKSPACE   8
#define KEY_TAB         9
#define KEY_UP          72
#define KEY_DOWN        80
#define KEY_LEFT        75
#define KEY_RIGHT       77
#define KEY_HOME        71
#define KEY_END         79
#define KEY_PGUP        73
#define KEY_PGDN        81
#define KEY_INSERT      82
#define KEY_DELETE      83
#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68
#define KEY_CTRL_HOME   119
#define KEY_CTRL_END    117
#define KEY_CTRL_LEFT   115
#define KEY_CTRL_RIGHT  116
#define KEY_ALT_F       33
#define KEY_ALT_E       18
#define KEY_ALT_S       31
#define KEY_ALT_B       48
#define KEY_ALT_O       24
#define KEY_ALT_T       20
#define KEY_ALT_H       35
#define KEY_ALT_X       45
#define KEY_CTRL_G      7
#define KEY_CTRL_N      14
#define KEY_CTRL_S      19
#define KEY_CTRL_Y      25
#define KEY_CTRL_Z      26
#define KEY_CTRL_B      2
#define KEY_CTRL_U      21
#define KEY_ALT_U       22
#define KEY_CTRL_B      2
#define KEY_ALT_L       38
#define KEY_ALT_B_FMT   48
#define KEY_ALT_V       47

/* Host helper bridge (speech/spell/translate via the Linux host daemon).
 * Paths are absolute on the DOSBox C: mount (c: = /home/ronen/dos/eini2). */
#define BRIDGE_REQ_TMP  "C:\\OPENQT\\BRIDGE\\REQ.TMP"
#define BRIDGE_REQ      "C:\\OPENQT\\BRIDGE\\REQ.TXT"
#define BRIDGE_RESP     "C:\\OPENQT\\BRIDGE\\RESP.TXT"
#define BRIDGE_BUF      16384
#define ASSIST_MAX_FIND 300

/* Text formatting styles */
#define STYLE_NORMAL    0
#define STYLE_BOLD      1
#define STYLE_UNDERLINE 2
#define STYLE_BOLD_UL   3   /* Bold + Underline */

/* Formatting marker bytes (QText compatible) */
#define FMT_START       0xAF
#define FMT_END         0xAE

#define HEB_FIRST       128
#define HEB_LAST        154

/* Arabic letters in CP864. The codepage is non-uniform: it stores Arabic
 * letters in mixed positional forms (initial / medial / final / isolated)
 * scattered across the upper half. Letters and Arabic punctuation occupy
 * 0xA1..0xAF, 0xBA..0xBF, 0xC1..0xDA, 0xDF..0xEF, 0xF1..0xFD. Arabic-Indic
 * digits sit at 0xB0..0xB9 (handled as BIDI_EN, not BIDI_R). For the
 * "is this an Arabic byte?" predicate we use the inclusive range below
 * and rely on the BiDi classifier to handle the digit gap. */
#define ARAB_FIRST      0xA1
#define ARAB_LAST       0xFD

/* Language modes for doc.input_lang */
#define LANG_ENG        0
#define LANG_HEB        1
#define LANG_ARA        2
#define LANG_RUS        3
#define LANG_COUNT      4

/* BiDi character types */
#define BIDI_R          0
#define BIDI_L          1
#define BIDI_EN         2
#define BIDI_WS         3
#define BIDI_ON         4

/* Hebrew keyboard mapping - standard Israeli layout */
static unsigned char heb_map[128] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    ' ',0,0,0,0,0,0,',',')','(',0,0,154,'-',149,'.',
    '0','1','2','3','4','5','6','7','8','9',0,147,0,0,0,0,
    0,153,144,129,130,151,139,146,137,143,135,140,138,150,142,141,
    148,'/',152,131,128,133,132,'\'',145,136,134,']',0,'[',0,0,
    0,153,144,129,130,151,139,146,137,143,135,140,138,150,142,141,
    148,'/',152,131,128,133,132,'\'',145,136,134,0,0,0,0,0
};

/* Arabic keyboard mapping - standard Microsoft Arabic 101 layout. Each
 * ASCII key maps to a CP864 byte holding the ISOLATED form of the letter
 * (best for static rendering without positional shaping). Verified against
 * the Unicode CP864.TXT mapping.
 *
 *   q->ض(0xEB)   w->ص(0xBE)   e->ث(0xAB)   r->ق(0xF8)   t->ف(0xBA)
 *   y->غ(0xEE)   u->ع(0xDF)   i->ه(0xF3)   o->خ(0xAF)   p->ح(0xAE)
 *   [->ج(0xAD)   ]->د(0xCF)
 *   a->ش(0xBD)   s->س(0xBC)   d->ي(0xFD)   f->ب(0xA9)   g->ل(0xFB)
 *   h->ا(0xC7)   j->ت(0xAA)   k->ن(0xF2)   l->م(0xEF)   ;->ك(0xFC)
 *   '->ط(0xD7)
 *   z->ئ(0xC6)   x->ء(0xC1)   c->ؤ(0xC4)   v->ر(0xD1)   b->ﻻ(0xF9 lam-alef)
 *   n->ى(0xE9)   m->ة(0xC9)   ,->و(0xE8)   .->ز(0xD2)   /->ظ(0xD8)
 *   `->ذ(0xD0) */
static unsigned char arab_map[128] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    ' ',0,0,0,0,0,0,0xD7,')','(',0,0,0xE8,'-',0xD2,0xD8,
    '0','1','2','3','4','5','6','7','8','9',0,0xFC,0,0,0,0,
    0,0xBD,0xF9,0xC4,0xFD,0xAB,0xA9,0xFB,0xC7,0xF3,0xAA,0xF2,0xEF,0xC9,0xE9,0xAF,
    0xAE,0xEB,0xF8,0xBC,0xBA,0xDF,0xD1,0xBE,0xC1,0xEE,0xC6,0xAD,0,0xCF,0,0,
    0xD0,0xBD,0xF9,0xC4,0xFD,0xAB,0xA9,0xFB,0xC7,0xF3,0xAA,0xF2,0xEF,0xC9,0xE9,0xAF,
    0xAE,0xEB,0xF8,0xBC,0xBA,0xDF,0xD1,0xBE,0xC1,0xEE,0xC6,0,0,0,0,0
};

static unsigned char heb_final[256];

/* Russian keyboard mapping - standard JCUKEN layout (the layout printed on
 * Soviet/Russian PC keycaps). Each US-QWERTY key maps to a CP866 byte for
 * the corresponding Cyrillic letter. Both letter cases are mapped (Russian
 * has case, unlike Hebrew/Arabic). The two non-letter swaps follow the
 * traditional Russian-DOS layout: '/' -> '.', '?' -> ',', and Shift+`
 * gives capital Yo. CP866 letters: uppercase A..YA at 0x80..0x9F, lowercase
 * a..p at 0xA0..0xAF, lowercase r..ya at 0xE0..0xEF, Yo/yo at 0xF0/0xF1.
 *
 *   q->Й r->К t->Е u->Г o->Щ      a->Ф s->Ы d->В f->А g->П h->Р j->О
 *   w->Ц e->У y->Н i->Ш p->З      k->Л l->Д ;->ж '->э
 *   [->Х ]->Ъ                     z->Я x->Ч c->С v->М b->И n->Т m->Ь
 *   `->ё ~->Ё ,->б .->ю /->. ?->,
 */
static unsigned char rus_map[128] = {
    /* 0x00 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 0x10 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 0x20 */ ' ','!',0x9D,'#','$','%','&',0xED,'(',')','*','+',0xA1,'-',0xEE,'.',
    /* 0x30 */ '0','1','2','3','4','5','6','7','8','9',0x86,0xA6,0x81,'=',0x9E,',',
    /* 0x40 */ '@',0x94,0x88,0x91,0x82,0x93,0x80,0x8F,0x90,0x98,0x8E,0x8B,0x84,0x9C,0x92,0x99,
    /* 0x50 */ 0x87,0x89,0x8A,0x9B,0x85,0x83,0x8C,0x96,0x97,0x8D,0x9F,0xE5,'\\',0xEA,'^','_',
    /* 0x60 */ 0xF1,0xE4,0xA8,0xE1,0xA2,0xE3,0xA0,0xAF,0xE0,0xE8,0xAE,0xAB,0xA4,0xEC,0xE2,0xE9,
    /* 0x70 */ 0xA7,0xA9,0xAA,0xEB,0xA5,0xA3,0xAC,0xE6,0xE7,0xAD,0xEF,0x95,'|',0x9A,0xF0,0
};

/* Arabic shaping tables (filled by init_arabic_shaping).
 * arab_join[b] :  0 = does not join (space, punct, English, non-Arabic)
 *                 1 = right-joining only (Alef, Dal, Reh, Waw, lam-alef
 *                     ligatures, Teh-Marbuta, Alef-Maksura, Hamza-on-Waw,
 *                     ...). These letters accept a join on their right
 *                     side but never extend leftward, so they keep their
 *                     isolated-form byte regardless of context.
 *                 2 = dual-joining (Beh, Teh, Theh, Jeem, ..., Lam, Meem,
 *                     Noon, Heh, Yeh). These letters extend leftward when
 *                     followed by a letter that accepts a right-side join.
 * arab_initial[b]: CP864 byte for the INITIAL form of letter b. Equals
 *                 b for letters with no separate initial form.
 * arab_medial[b]:  CP864 byte for the MEDIAL form of letter b, or 0 if
 *                 the letter has no separate medial byte (in which case
 *                 the initial form is reused for medial position).
 * arab_final[b]:   CP864 byte for the FINAL form of letter b, or 0 if
 *                 there is no separate final byte (in which case the
 *                 isolated form is reused for final position). */
static unsigned char arab_join[256];
static unsigned char arab_initial[256];
static unsigned char arab_medial[256];
static unsigned char arab_final[256];

/* BiDi reordering structure */
typedef struct {
    unsigned char visual[MAX_LINE_LEN];
    unsigned char attrs[MAX_LINE_LEN];   /* Formatting attributes per char */
    int log_to_vis[MAX_LINE_LEN];
    int vis_to_log[MAX_LINE_LEN];
    unsigned char types[MAX_LINE_LEN];
    unsigned char levels[MAX_LINE_LEN];
    int len;
} BidiLine;

static BidiLine bidi;

typedef struct {
    char **lines;
    unsigned char **styles;  /* Formatting style for each character */
    int num_lines;
    int max_lines;
    int modified;
    char filename[256];
    int cursor_x;
    int cursor_y;
    int scroll_x;
    int scroll_y;
    int insert_mode;
    int input_lang;        /* LANG_ENG / LANG_HEB / LANG_ARA / LANG_RUS */
    int rtl_mode;
    int embedded_ltr;
    int word_wrap;
    int show_ruler;
    int block_active;
    int block_start_x;
    int block_start_y;
    int block_end_x;
    int block_end_y;
    char **clipboard;
    int clipboard_lines;
    int current_style;  /* Current formatting style for new text */
    int save_reminder;  /* Save reminder enabled */
    time_t last_save_time;  /* Time of last save */
    int encrypted;      /* Document is encrypted */
    char password[64];  /* Password for encryption */
} Document;

/* Encryption magic header */
#define ENCRYPT_MAGIC "OQT-ENC1"
#define ENCRYPT_MAGIC_LEN 8

static Document doc;
static char *video_mem = (char *)0xB8000;
static int running = 1;
/* Set when launched via OQTA / "/A" — ARABVGA loaded the CP864 font, which
 * overwrites CP437 box-drawing glyphs at 0xB0..0xDF. We fall back to ASCII
 * pseudo-boxes (-, |, +) so menus and dialogs stay legible. */
static int g_ascii_boxes = 0;
/* Set when launched via OQTR / "/R" — RUSVGA loaded the CP866 font; the
 * editor starts in Russian (LTR) input mode so the user can type
 * immediately. CP866 keeps CP437 box-drawing intact at 0xB0..0xDF, so no
 * ASCII-box fallback is needed. */
static int g_russian_start = 0;
#define MAX_UNDO        250

/* Undo/Redo system */
typedef struct {
    int type;           /* 0=insert_char, 1=delete_char, 2=insert_line, 3=delete_line, 4=backspace */
    int line;
    int col;
    unsigned char ch;
    char *saved_line;   /* For line operations */
} UndoItem;

static UndoItem undo_stack[MAX_UNDO];
static int undo_pos = 0;
static int undo_count = 0;
static int redo_count = 0;

static char search_text[256] = "";
static char replace_text[256] = "";
static int search_case = 0;

/* Forward declarations */
void draw_screen(void);
void draw_status_bar(void);
void draw_menu_bar(void);
void update_cursor(void);
int clean_line_len(const char *line);
int visible_to_raw_pos(const char *line, int visible_pos);
void show_dialog(const char *title, const char *msg);
int confirm_dialog(const char *title, const char *msg);
int input_dialog(const char *title, const char *prompt, char *buffer, int maxlen);
int password_dialog(const char *title, const char *prompt, char *buffer, int maxlen);
int search_dialog(const char *title, const char *prompt, char *buffer, int maxlen);
void show_help(void);
void show_about(void);
void show_docs(void);
void do_search(void);
void do_replace(void);
void do_replace_all(void);
void find_next(void);
void find_prev(void);
void start_block(void);
void end_block(void);
void copy_block(void);
void cut_block(void);
void paste_block(void);
void delete_block(void);
void unselect_block(void);
void show_file_menu(void);
void show_edit_menu(void);
void show_search_menu(void);
void show_block_menu(void);
void show_options_menu(void);
void show_help_menu(void);
void show_tools_menu(void);
void print_document(void);
void undo_action(void);
void redo_action(void);
void push_undo(int type, int line, int col, unsigned char ch, const char *saved);
void clear_undo_stack(void);
void word_count(void);
void goto_line(void);
void insert_date_time(void);
int file_dialog(const char *title, char *filename, int maxlen, int save_mode);
void bidi_reorder(const char *logical, int len);
void init_arabic_shaping(void);
void shape_arabic_inplace(unsigned char *buf, int len);
int visual_to_logical(int vis_pos);
int logical_to_visual(int log_pos);
int cursor_to_clean_pos(const char *line, int cursor_x);
int save_file_encrypted(const char *filename, const char *password);
int load_file_encrypted(const char *filename, const char *password);
int is_file_encrypted(const char *filename);
int save_with_password(void);
int remove_password(void);
/* Host helper bridge + language-assist features */
int bridge_request(const char *cmd, const char *lang, const char *payload,
                   int plen, char *resp, int rmax, int timeout_sec);
const char *bridge_err(int code, const char *resp);
const char *cur_lang_hint(void);
int gather_doc_text(char *buf, int maxlen);
void show_busy(const char *msg);
void assist_read_aloud(void);
void assist_stop_speech(void);
void assist_translate(void);
void assist_spell_check(void);
void assist_dictate(void);
void assist_paste_clipboard(void);

char *stristr(const char *haystack, const char *needle)
{
    size_t needle_len, haystack_len, i;
    if (!haystack || !needle) return NULL;
    needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;
    haystack_len = strlen(haystack);
    if (needle_len > haystack_len) return NULL;
    for (i = 0; i <= haystack_len - needle_len; i++) {
        if (strnicmp(haystack + i, needle, needle_len) == 0)
            return (char *)(haystack + i);
    }
    return NULL;
}

void write_char(int x, int y, unsigned char ch, unsigned char attr)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        video_mem[(y * SCREEN_WIDTH + x) * 2] = ch;
        video_mem[(y * SCREEN_WIDTH + x) * 2 + 1] = attr;
    }
}

void write_string(int x, int y, const char *str, unsigned char attr)
{
    while (*str && x < SCREEN_WIDTH) {
        write_char(x++, y, (unsigned char)*str++, attr);
    }
}

void fill_rect(int x1, int y1, int x2, int y2, unsigned char ch, unsigned char attr)
{
    int x, y;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            write_char(x, y, ch, attr);
}

void draw_box(int x1, int y1, int x2, int y2, unsigned char attr, const char *title)
{
    int i, len, tx;
    unsigned char tl, tr, bl, br, hz, vt;
    if (g_ascii_boxes) {
        tl = '+'; tr = '+'; bl = '+'; br = '+'; hz = '-'; vt = '|';
    } else {
        tl = 0xDA; tr = 0xBF; bl = 0xC0; br = 0xD9; hz = 0xC4; vt = 0xB3;
    }
    write_char(x1, y1, tl, attr);
    for (i = x1 + 1; i < x2; i++) write_char(i, y1, hz, attr);
    write_char(x2, y1, tr, attr);
    for (i = y1 + 1; i < y2; i++) {
        write_char(x1, i, vt, attr);
        write_char(x2, i, vt, attr);
    }
    write_char(x1, y2, bl, attr);
    for (i = x1 + 1; i < x2; i++) write_char(i, y2, hz, attr);
    write_char(x2, y2, br, attr);
    fill_rect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, ' ', attr);
    if (title && *title) {
        len = strlen(title);
        tx = x1 + (x2 - x1 - len - 2) / 2;
        write_char(tx, y1, ' ', attr);
        write_string(tx + 1, y1, title, attr);
        write_char(tx + len + 1, y1, ' ', attr);
    }
}

void set_cursor_pos(int x, int y)
{
    union REGS regs;
    regs.h.ah = 0x02;
    regs.h.bh = 0;
    regs.h.dh = (unsigned char)y;
    regs.h.dl = (unsigned char)x;
    int386(0x10, &regs, &regs);
}

void hide_cursor(void)
{
    union REGS regs;
    regs.h.ah = 0x01;
    regs.h.ch = 0x20;
    regs.h.cl = 0x00;
    int386(0x10, &regs, &regs);
}

void show_cursor(int ins_mode)
{
    union REGS regs;
    regs.h.ah = 0x01;
    regs.h.ch = (unsigned char)(ins_mode ? 6 : 0);
    regs.h.cl = 7;
    int386(0x10, &regs, &regs);
}

void init_hebrew_mapping(void)
{
    memset(heb_final, 0, sizeof(heb_final));
    heb_final[139] = 138;
    heb_final[142] = 141;
    heb_final[144] = 143;
    heb_final[148] = 147;
    heb_final[150] = 149;
}

/* Build arab_join[] and arab_initial[] for Arabic positional shaping.
 * Each abstract isolated-form byte that the user can type is classified
 * (right-only or dual) and (for dual-joining letters) paired with its
 * CP864 initial-form byte. Letters not listed default to non-joining. */
void init_arabic_shaping(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        arab_join[i] = 0;
        arab_initial[i] = (unsigned char)i;
        arab_medial[i] = 0;
        arab_final[i] = 0;
    }

    /* Final-form bytes for right-joining-only letters. Most right-joining
     * letters in CP864 (Dal, Reh, Waw, Teh-Marbuta, ...) have no separate
     * final byte; the isolated glyph already looks correct when joined
     * from the right. Only the Alef variants and Alef-Maksura get their
     * own final byte, plus the Lam-Alef-Madda ligature. */
    arab_final[0xC2] = 0xA2; /* Alef-Madda final */
    arab_final[0xC3] = 0xA5; /* Alef-Hamza-above final */
    arab_final[0xC7] = 0xA8; /* Alef final */
    arab_final[0xE9] = 0xF5; /* Alef-Maksura final */
    arab_final[0xF9] = 0xFA; /* Lam-Alef-Madda ligature final */

    /* Right-joining-only letters: keep isolated form, but DO accept a
     * right-side join (so a dual-joining letter to their logical-left
     * will take initial form). */
    arab_join[0xC2] = 1; /* Alef-Madda */
    arab_join[0xC3] = 1; /* Alef-Hamza-above */
    arab_join[0xC4] = 1; /* Waw-Hamza */
    arab_join[0xC5] = 1; /* Alef-Hamza-below */
    arab_join[0xC7] = 1; /* Alef */
    arab_join[0xC9] = 1; /* Teh-Marbuta */
    arab_join[0xCF] = 1; /* Dal */
    arab_join[0xD0] = 1; /* Thal */
    arab_join[0xD1] = 1; /* Reh */
    arab_join[0xD2] = 1; /* Zain */
    arab_join[0xE8] = 1; /* Waw */
    arab_join[0xE9] = 1; /* Alef-Maksura */
    arab_join[0xF9] = 1; /* Lam-Alef-Madda ligature (isolated) */
    arab_join[0xFA] = 1; /* Lam-Alef-Madda ligature (final) */

    /* Dual-joining letters: pair (isolated_byte, initial_byte) from CP864. */
    arab_join[0xA9] = 2; arab_initial[0xA9] = 0xC8; /* Beh */
    arab_join[0xAA] = 2; arab_initial[0xAA] = 0xCA; /* Teh */
    arab_join[0xAB] = 2; arab_initial[0xAB] = 0xCB; /* Theh */
    arab_join[0xAD] = 2; arab_initial[0xAD] = 0xCC; /* Jeem */
    arab_join[0xAE] = 2; arab_initial[0xAE] = 0xCD; /* Hah */
    arab_join[0xAF] = 2; arab_initial[0xAF] = 0xCE; /* Khah */
    arab_join[0xBA] = 2; arab_initial[0xBA] = 0xE1; /* Feh */
    arab_join[0xBC] = 2; arab_initial[0xBC] = 0xD3; /* Seen */
    arab_join[0xBD] = 2; arab_initial[0xBD] = 0xD4; /* Sheen */
    arab_join[0xBE] = 2; arab_initial[0xBE] = 0xD5; /* Sad */
    arab_join[0xEB] = 2; arab_initial[0xEB] = 0xD6; /* Dad */
    arab_join[0xEF] = 2; arab_initial[0xEF] = 0xE5; /* Meem */
    arab_join[0xF2] = 2; arab_initial[0xF2] = 0xE6; /* Noon */
    arab_join[0xF8] = 2; arab_initial[0xF8] = 0xE2; /* Qaf */
    arab_join[0xFB] = 2; arab_initial[0xFB] = 0xE4; /* Lam */
    arab_join[0xFC] = 2; arab_initial[0xFC] = 0xE3; /* Kaf */

    /* Dual-joining letters with all 4 CP864 forms (iso, init, medial, final). */
    arab_join[0xDF] = 2; arab_initial[0xDF] = 0xD9; /* Ain  */
    arab_medial[0xDF] = 0xEC; arab_final[0xDF] = 0xC5;
    arab_join[0xEE] = 2; arab_initial[0xEE] = 0xDA; /* Ghain */
    arab_medial[0xEE] = 0xF7; arab_final[0xEE] = 0xED;

    /* Heh — has a separate medial form, but no separate final. */
    arab_join[0xF3] = 2; arab_initial[0xF3] = 0xE7;
    arab_medial[0xF3] = 0xF4;

    /* Yeh — has a separate final form, but no separate medial. */
    arab_join[0xFD] = 2; arab_initial[0xFD] = 0xEA;
    arab_final[0xFD] = 0xF6;

    /* Dual-joining letters with NO separate CP864 initial form.
     * arab_initial[] stays equal to the byte itself, so the shaping
     * pass leaves them as-is. They still accept right-side joins so
     * neighbors are shaped correctly. */
    arab_join[0xC6] = 2; /* Yeh-Hamza (CP864 byte is already initial form) */
    arab_join[0xD7] = 2; /* Tah */
    arab_join[0xD8] = 2; /* Zah */
}

/* Apply full 4-form Arabic positional shaping in logical order. The
 * buffer is mutated byte-for-byte, length-preserving. Decision per
 * letter (using the ORIGINAL neighboring bytes via a snapshot, so
 * substitutions don't corrupt the class of subsequent decisions):
 *
 *   prev_joins_to_me  = previous logical byte is dual-joining (it has
 *                       a left-side connector extending toward me).
 *   next_accepts_join = next logical byte is dual-joining or
 *                       right-joining-only (it has a right-side
 *                       connector).
 *
 *   For dual-joining letters (class 2):
 *     prev=Y, next=Y -> MEDIAL  (fall back to INITIAL if no medial byte)
 *     prev=Y, next=N -> FINAL   (fall back to ISOLATED if no final byte)
 *     prev=N, next=Y -> INITIAL
 *     prev=N, next=N -> ISOLATED
 *
 *   For right-joining-only letters (class 1):
 *     prev=Y -> FINAL   (fall back to ISOLATED if no final byte)
 *     prev=N -> ISOLATED
 *
 *   Non-joining (class 0) bytes are untouched. */
void shape_arabic_inplace(unsigned char *buf, int len)
{
    static unsigned char orig[MAX_LINE_LEN];
    int i;
    unsigned char curr, prev, next;
    int prev_joins, next_joins;

    if (len <= 0 || len > MAX_LINE_LEN) return;
    for (i = 0; i < len; i++) orig[i] = buf[i];

    for (i = 0; i < len; i++) {
        curr = orig[i];
        if (arab_join[curr] == 0) continue;

        prev = (i > 0) ? orig[i - 1] : 0;
        next = (i + 1 < len) ? orig[i + 1] : 0;
        prev_joins = (arab_join[prev] == 2);
        next_joins = (arab_join[next] != 0);

        if (arab_join[curr] == 2) {
            if (prev_joins && next_joins) {
                buf[i] = arab_medial[curr] ? arab_medial[curr] : arab_initial[curr];
            } else if (prev_joins) {
                if (arab_final[curr]) buf[i] = arab_final[curr];
            } else if (next_joins) {
                buf[i] = arab_initial[curr];
            }
        } else { /* class 1 */
            if (prev_joins && arab_final[curr]) {
                buf[i] = arab_final[curr];
            }
        }
    }
}

unsigned char translate_hebrew(unsigned char ch)
{
    if (ch < 128) return heb_map[ch];
    return ch;
}

unsigned char translate_arabic(unsigned char ch)
{
    if (ch < 128) return arab_map[ch];
    return ch;
}

unsigned char translate_russian(unsigned char ch)
{
    if (ch < 128) return rus_map[ch];
    return ch;
}

int is_hebrew_char(unsigned char ch)
{
    return (ch >= HEB_FIRST && ch <= HEB_LAST);
}

int is_arabic_char(unsigned char ch)
{
    /* CP864 Arabic letters/punctuation, excluding the Arabic-Indic digits
     * (0xB0..0xB9) which are bidirectionally LTR. */
    if (ch >= 0xB0 && ch <= 0xB9) return 0;
    return (ch >= ARAB_FIRST && ch <= ARAB_LAST);
}

int is_rtl_char(unsigned char ch)
{
    return is_hebrew_char(ch) || is_arabic_char(ch);
}

/* Is the editor currently in any RTL input mode (Hebrew or Arabic)? */
int is_rtl_input(void)
{
    return doc.input_lang == LANG_HEB || doc.input_lang == LANG_ARA;
}

void apply_final_letters(char *line)
{
    int i, len, j;
    unsigned char ch, next;
    if (!line) return;
    len = strlen(line);
    for (i = 0; i < len; i++) {
        ch = (unsigned char)line[i];
        
        /* Skip format codes */
        if (ch == FMT_BOLD || ch == FMT_UNDERLINE || ch == FMT_BOLDUNDER)
            continue;
        
        /* Find next non-format character */
        next = ' ';  /* Default: end of word */
        for (j = i + 1; j < len; j++) {
            unsigned char nc = (unsigned char)line[j];
            if (nc != FMT_BOLD && nc != FMT_UNDERLINE && nc != FMT_BOLDUNDER) {
                next = nc;
                break;
            }
        }
        
        /* Convert to final form only if at end of word */
        if (heb_final[ch] && (next == ' ' || next == '\0' || !is_hebrew_char(next)))
            line[i] = heb_final[ch];
    }
}

/* =========== BiDi Algorithm =========== */

int get_bidi_type(unsigned char ch)
{
    if (ch >= HEB_FIRST && ch <= HEB_LAST) return BIDI_R;
    if (ch >= 0xB0 && ch <= 0xB9) return BIDI_EN;          /* CP864 Arabic-Indic digits */
    if (ch >= ARAB_FIRST && ch <= ARAB_LAST) return BIDI_R;
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) return BIDI_L;
    if (ch >= '0' && ch <= '9') return BIDI_EN;
    if (ch == ' ' || ch == '\t') return BIDI_WS;
    return BIDI_ON;
}

void bidi_reorder(const char *logical, int len)
{
    int i, j, k, run_start, run_end, ch_type, has_more_ltr, t;
    int src_i, dst_i;
    unsigned char current_attr = CLR_NORMAL;
    unsigned char clean[MAX_LINE_LEN];
    unsigned char clean_attrs[MAX_LINE_LEN];
    int full_to_clean[MAX_LINE_LEN];  /* Map full index to clean index */
    int clean_len = 0;
    
    if (len <= 0 || len >= MAX_LINE_LEN) { bidi.len = 0; return; }
    
    /* First pass: remove format codes and track attributes */
    for (src_i = 0; src_i < len; src_i++) {
        unsigned char ch = (unsigned char)logical[src_i];
        if (ch == FMT_BOLD) {
            current_attr = (current_attr == CLR_BOLD) ? CLR_NORMAL : CLR_BOLD;
            full_to_clean[src_i] = clean_len;  /* Point to next char */
        } else if (ch == FMT_UNDERLINE) {
            current_attr = (current_attr == CLR_UNDERLINE) ? CLR_NORMAL : CLR_UNDERLINE;
            full_to_clean[src_i] = clean_len;
        } else if (ch == FMT_BOLDUNDER) {
            current_attr = (current_attr == CLR_BOLDUNDER) ? CLR_NORMAL : CLR_BOLDUNDER;
            full_to_clean[src_i] = clean_len;
        } else {
            full_to_clean[src_i] = clean_len;
            clean[clean_len] = ch;
            clean_attrs[clean_len] = current_attr;
            clean_len++;
        }
    }
    
    bidi.len = clean_len;
    if (clean_len == 0) return;

    /* Arabic positional shaping (display-only, in logical order). Lines
     * with no Arabic letters are unaffected. Length-preserving so
     * clean_attrs[] alignment is kept.
     *
     * Skipped in Russian sessions: CP866 Cyrillic at 0xA0-0xEF overlaps
     * CP864 Arabic letter bytes, so without this gate the shaping table
     * would rewrite Cyrillic letters into other CP866 bytes (often
     * 0xB0-0xDF box-drawing). */
    if (doc.input_lang != LANG_RUS) {
        shape_arabic_inplace(clean, clean_len);
    }

    /* Russian is LTR regardless of doc.rtl_mode - CP866 bytes do not
     * carry script info, so the BiDi classifier (which only knows
     * Hebrew + Arabic ranges) would mis-flag Cyrillic as RTL. */
    if (!doc.rtl_mode || doc.input_lang == LANG_RUS) {
        /* LTR mode: copy as-is */
        for (i = 0; i < clean_len; i++) {
            bidi.visual[i] = clean[i];
            bidi.attrs[i] = clean_attrs[i];
            bidi.vis_to_log[i] = i;
        }
    } else {
        /* RTL mode: Simple 2-step algorithm */
        
        /* Step 1: Reverse entire line */
        for (i = 0; i < clean_len; i++) {
            bidi.visual[i] = clean[clean_len - 1 - i];
            bidi.attrs[i] = clean_attrs[clean_len - 1 - i];
            bidi.vis_to_log[i] = clean_len - 1 - i;
        }
        
        /* Step 2: Find LTR runs (English/numbers) and reverse them back */
        i = 0;
        while (i < clean_len) {
            ch_type = get_bidi_type(bidi.visual[i]);
            
            if (ch_type == BIDI_L || ch_type == BIDI_EN) {
                /* Start of LTR run */
                run_start = i;
                
                /* Find end of LTR run */
                while (i < clean_len) {
                    ch_type = get_bidi_type(bidi.visual[i]);
                    if (ch_type == BIDI_L || ch_type == BIDI_EN) {
                        i++;
                    } else if (ch_type == BIDI_WS || ch_type == BIDI_ON) {
                        /* Check if there's more LTR after this space/punctuation */
                        has_more_ltr = 0;
                        for (j = i + 1; j < clean_len; j++) {
                            t = get_bidi_type(bidi.visual[j]);
                            if (t == BIDI_L || t == BIDI_EN) { has_more_ltr = 1; break; }
                            if (t == BIDI_R) break;
                        }
                        if (has_more_ltr) i++;
                        else break;
                    } else {
                        break;
                    }
                }
                run_end = i - 1;
                
                /* Reverse this LTR run back to correct order */
                for (j = run_start, k = run_end; j < k; j++, k--) {
                    unsigned char tc = bidi.visual[j];
                    unsigned char ta = bidi.attrs[j];
                    int tl = bidi.vis_to_log[j];
                    bidi.visual[j] = bidi.visual[k];
                    bidi.attrs[j] = bidi.attrs[k];
                    bidi.vis_to_log[j] = bidi.vis_to_log[k];
                    bidi.visual[k] = tc;
                    bidi.attrs[k] = ta;
                    bidi.vis_to_log[k] = tl;
                }
            } else {
                i++;
            }
        }
    }
    
    /* Build reverse mapping */
    for (i = 0; i < clean_len; i++) {
        bidi.log_to_vis[bidi.vis_to_log[i]] = i;
    }
}

int visual_to_logical(int vis_pos)
{
    if (vis_pos < 0) return 0;
    if (vis_pos >= bidi.len) return bidi.len;
    return bidi.vis_to_log[vis_pos];
}

int logical_to_visual(int log_pos)
{
    if (log_pos < 0) return 0;
    if (log_pos >= bidi.len) return bidi.len;
    return bidi.log_to_vis[log_pos];
}

/* =========== Document Management =========== */

void clear_undo_stack(void)
{
    int i;
    for (i = 0; i < MAX_UNDO; i++) {
        if (undo_stack[i].saved_line) {
            free(undo_stack[i].saved_line);
            undo_stack[i].saved_line = NULL;
        }
    }
    undo_pos = 0;
    undo_count = 0;
    redo_count = 0;
}

void init_document(void)
{
    doc.lines = (char **)malloc(MAX_LINES * sizeof(char *));
    doc.styles = (unsigned char **)malloc(MAX_LINES * sizeof(unsigned char *));
    if (!doc.lines || !doc.styles) { printf("Memory allocation failed!\n"); exit(1); }
    memset(doc.lines, 0, MAX_LINES * sizeof(char *));
    memset(doc.styles, 0, MAX_LINES * sizeof(unsigned char *));
    doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
    doc.styles[0] = (unsigned char *)malloc(MAX_LINE_LEN);
    if (!doc.lines[0] || !doc.styles[0]) { printf("Memory allocation failed!\n"); exit(1); }
    doc.lines[0][0] = '\0';
    memset(doc.styles[0], STYLE_NORMAL, MAX_LINE_LEN);
    doc.num_lines = 1;
    doc.max_lines = MAX_LINES;
    doc.modified = 0;
    strcpy(doc.filename, "NONAME.TXT");
    doc.cursor_x = 0;
    doc.cursor_y = 0;
    doc.scroll_x = 0;
    doc.scroll_y = 0;
    doc.insert_mode = 1;
    doc.input_lang = LANG_HEB;
    doc.rtl_mode = 1;
    doc.embedded_ltr = 0;
    doc.word_wrap = 1;
    doc.show_ruler = 1;
    doc.block_active = 0;
    doc.clipboard = NULL;
    doc.clipboard_lines = 0;
    doc.current_style = STYLE_NORMAL;
    doc.save_reminder = 1;  /* Reminder enabled by default */
    doc.last_save_time = time(NULL);
    doc.encrypted = 0;
    doc.password[0] = '\0';
    clear_undo_stack();
}

void free_document(void)
{
    int i;
    if (doc.lines) {
        for (i = 0; i < doc.num_lines; i++)
            if (doc.lines[i]) free(doc.lines[i]);
        free(doc.lines);
        doc.lines = NULL;
    }
    if (doc.clipboard) {
        for (i = 0; i < doc.clipboard_lines; i++)
            if (doc.clipboard[i]) free(doc.clipboard[i]);
        free(doc.clipboard);
        doc.clipboard = NULL;
    }
}

void new_document(void)
{
    int i;
    if (doc.modified) {
        if (!confirm_dialog("New Document", "Discard changes?")) return;
    }
    for (i = 0; i < doc.num_lines; i++) {
        if (doc.lines[i]) { free(doc.lines[i]); doc.lines[i] = NULL; }
    }
    doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
    doc.lines[0][0] = '\0';
    doc.num_lines = 1;
    doc.modified = 0;
    doc.cursor_x = 0;
    doc.cursor_y = 0;
    doc.scroll_x = 0;
    doc.scroll_y = 0;
    doc.block_active = 0;
    doc.embedded_ltr = 0;
    doc.last_save_time = time(NULL);  /* Reset save timer */
    doc.encrypted = 0;
    doc.password[0] = '\0';
    strcpy(doc.filename, "NONAME.TXT");
    clear_undo_stack();
    draw_screen();
}

/* =========== Encryption Functions =========== */

/* Generate encryption key from password */
unsigned long generate_key(const char *password)
{
    unsigned long key = 5381;
    int i;
    for (i = 0; password[i]; i++) {
        key = ((key << 5) + key) + (unsigned char)password[i];
    }
    return key;
}

/* Encrypt/decrypt a byte (XOR with key stream) */
unsigned char crypt_byte(unsigned char byte, unsigned long *key_state)
{
    unsigned char key_byte;
    *key_state = (*key_state * 1103515245 + 12345) & 0x7FFFFFFF;
    key_byte = (unsigned char)((*key_state >> 16) & 0xFF);
    return byte ^ key_byte;
}

/* Encrypt buffer in place */
void encrypt_buffer(unsigned char *buf, int len, const char *password)
{
    unsigned long key_state = generate_key(password);
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = crypt_byte(buf[i], &key_state);
    }
}

/* Decrypt buffer in place (same as encrypt - XOR is reversible) */
void decrypt_buffer(unsigned char *buf, int len, const char *password)
{
    encrypt_buffer(buf, len, password);  /* XOR is its own inverse */
}

/* Check if file is encrypted */
int is_file_encrypted(const char *filename)
{
    FILE *fp;
    char magic[ENCRYPT_MAGIC_LEN + 1];
    
    fp = fopen(filename, "rb");
    if (!fp) return 0;
    
    if (fread(magic, 1, ENCRYPT_MAGIC_LEN, fp) != ENCRYPT_MAGIC_LEN) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    
    magic[ENCRYPT_MAGIC_LEN] = '\0';
    return (strcmp(magic, ENCRYPT_MAGIC) == 0);
}

/* Save file with encryption */
int save_file_encrypted(const char *filename, const char *password)
{
    FILE *fp;
    int i, len;
    unsigned char *buffer;
    int buf_size = 0, buf_pos = 0;
    
    /* Calculate total size needed */
    for (i = 0; i < doc.num_lines; i++) {
        buf_size += doc.lines[i] ? strlen(doc.lines[i]) : 0;
        buf_size += 2;  /* CR LF */
    }
    
    buffer = (unsigned char *)malloc(buf_size + 1);
    if (!buffer) { show_dialog("Error", "Memory error!"); return 0; }
    
    /* Copy all lines to buffer */
    for (i = 0; i < doc.num_lines; i++) {
        if (doc.lines[i]) {
            len = strlen(doc.lines[i]);
            memcpy(buffer + buf_pos, doc.lines[i], len);
            buf_pos += len;
        }
        buffer[buf_pos++] = '\r';
        buffer[buf_pos++] = '\n';
    }
    
    /* Encrypt the buffer */
    encrypt_buffer(buffer, buf_pos, password);
    
    /* Write to file */
    fp = fopen(filename, "wb");
    if (!fp) { free(buffer); show_dialog("Error", "Cannot create file!"); return 0; }
    
    /* Write magic header (not encrypted) */
    fwrite(ENCRYPT_MAGIC, 1, ENCRYPT_MAGIC_LEN, fp);
    
    /* Write encrypted data */
    fwrite(buffer, 1, buf_pos, fp);
    
    fclose(fp);
    free(buffer);
    
    doc.modified = 0;
    doc.encrypted = 1;
    strncpy(doc.password, password, sizeof(doc.password) - 1);
    doc.last_save_time = time(NULL);
    strncpy(doc.filename, filename, sizeof(doc.filename) - 1);
    
    show_dialog("Encrypted", "Document saved with password protection");
    return 1;
}

/* Load encrypted file */
int load_file_encrypted(const char *filename, const char *password)
{
    FILE *fp;
    unsigned char *buffer;
    long file_size;
    int line_count = 0, i, start, j;
    char magic[ENCRYPT_MAGIC_LEN + 1];
    
    fp = fopen(filename, "rb");
    if (!fp) { show_dialog("Error", "Cannot open file!"); return 0; }
    
    /* Read and verify magic header */
    if (fread(magic, 1, ENCRYPT_MAGIC_LEN, fp) != ENCRYPT_MAGIC_LEN) {
        fclose(fp);
        show_dialog("Error", "Invalid encrypted file!");
        return 0;
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp) - ENCRYPT_MAGIC_LEN;
    fseek(fp, ENCRYPT_MAGIC_LEN, SEEK_SET);
    
    buffer = (unsigned char *)malloc(file_size + 1);
    if (!buffer) { fclose(fp); show_dialog("Error", "Memory error!"); return 0; }
    
    /* Read encrypted data */
    fread(buffer, 1, file_size, fp);
    fclose(fp);
    
    /* Decrypt */
    decrypt_buffer(buffer, file_size, password);
    buffer[file_size] = '\0';
    
    /* Free old document */
    for (i = 0; i < doc.num_lines; i++) {
        if (doc.lines[i]) { free(doc.lines[i]); doc.lines[i] = NULL; }
    }
    
    /* Parse lines */
    start = 0;
    for (i = 0; i < file_size && line_count < MAX_LINES; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') {
            /* Found end of line */
            doc.lines[line_count] = (char *)malloc(MAX_LINE_LEN);
            if (!doc.lines[line_count]) { free(buffer); return 0; }
            
            /* Copy line */
            j = 0;
            while (start < i && j < MAX_LINE_LEN - 1) {
                doc.lines[line_count][j++] = buffer[start++];
            }
            doc.lines[line_count][j] = '\0';
            line_count++;
            
            /* Skip CR LF */
            if (buffer[i] == '\r' && i + 1 < file_size && buffer[i + 1] == '\n') {
                i++;
            }
            start = i + 1;
        }
    }
    
    free(buffer);
    
    if (line_count == 0) {
        doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
        doc.lines[0][0] = '\0';
        line_count = 1;
    }
    
    doc.num_lines = line_count;
    doc.modified = 0;
    doc.cursor_y = 0;
    doc.cursor_x = 0;
    doc.scroll_x = 0;
    doc.scroll_y = 0;
    doc.block_active = 0;
    doc.embedded_ltr = 0;
    doc.encrypted = 1;
    strncpy(doc.password, password, sizeof(doc.password) - 1);
    doc.last_save_time = time(NULL);
    strncpy(doc.filename, filename, sizeof(doc.filename) - 1);
    clear_undo_stack();
    
    return 1;
}

int load_file(const char *filename)
{
    FILE *fp;
    char buffer[MAX_LINE_LEN];
    char password[64];
    int line_count = 0, i, len;
    
    /* Check if file is encrypted */
    if (is_file_encrypted(filename)) {
        password[0] = '\0';
        if (!password_dialog("Password", "Enter password:", password, sizeof(password))) {
            return 0;  /* User cancelled */
        }
        if (password[0] == '\0') {
            show_dialog("Error", "Password required!");
            return 0;
        }
        return load_file_encrypted(filename, password);
    }
    
    fp = fopen(filename, "rb");
    if (!fp) { show_dialog("Error", "Cannot open file!"); return 0; }
    
    for (i = 0; i < doc.num_lines; i++) {
        if (doc.lines[i]) { free(doc.lines[i]); doc.lines[i] = NULL; }
    }
    
    while (fgets(buffer, MAX_LINE_LEN, fp) && line_count < MAX_LINES) {
        len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r'))
            buffer[--len] = '\0';
        doc.lines[line_count] = (char *)malloc(MAX_LINE_LEN);
        if (!doc.lines[line_count]) { fclose(fp); return 0; }
        strcpy(doc.lines[line_count], buffer);
        line_count++;
    }
    fclose(fp);
    
    if (line_count == 0) {
        doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
        doc.lines[0][0] = '\0';
        line_count = 1;
    }
    doc.num_lines = line_count;
    doc.modified = 0;
    doc.cursor_y = 0;
    doc.cursor_x = 0;
    doc.scroll_x = 0;
    doc.scroll_y = 0;
    doc.block_active = 0;
    doc.embedded_ltr = 0;
    doc.encrypted = 0;
    doc.password[0] = '\0';
    doc.last_save_time = time(NULL);  /* Reset save timer */
    strncpy(doc.filename, filename, sizeof(doc.filename) - 1);
    clear_undo_stack();
    return 1;
}

int save_file(const char *filename)
{
    FILE *fp;
    int i;
    
    fp = fopen(filename, "wb");
    if (!fp) { show_dialog("Error", "Cannot create file!"); return 0; }
    for (i = 0; i < doc.num_lines; i++) {
        if (doc.lines[i]) {
            fputs(doc.lines[i], fp);
        }
        fputs("\r\n", fp);
    }
    fclose(fp);
    doc.modified = 0;
    doc.last_save_time = time(NULL);  /* Reset save timer */
    strncpy(doc.filename, filename, sizeof(doc.filename) - 1);
    return 1;
}

int save_file_as(void)
{
    char filename[256];
    strcpy(filename, doc.filename);
    if (file_dialog("Save As", filename, sizeof(filename), 1)) {
        return save_file(filename);
    }
    return 0;
}

/* Save with password protection */
/* Change file extension */
void change_extension(char *filename, const char *new_ext)
{
    char *dot = strrchr(filename, '.');
    char *slash = strrchr(filename, '\\');
    
    /* Make sure dot is after last slash (part of filename, not path) */
    if (dot && (!slash || dot > slash)) {
        strcpy(dot, new_ext);
    } else {
        strcat(filename, new_ext);
    }
}

/* Get original extension (before .OQE) or default to .TXT */
void restore_extension(char *filename)
{
    char *dot = strrchr(filename, '.');
    char *slash = strrchr(filename, '\\');
    
    if (dot && (!slash || dot > slash)) {
        /* Change .OQE to .TXT */
        strcpy(dot, ".TXT");
    }
}

int save_with_password(void)
{
    char filename[256];
    char password[64];
    char confirm[64];
    
    strcpy(filename, doc.filename);
    
    /* Change extension to .OQE */
    change_extension(filename, ".OQE");
    
    /* Let user confirm/change filename */
    if (!file_dialog("Save Encrypted", filename, sizeof(filename), 1)) {
        return 0;
    }
    
    /* Make sure it has .OQE extension */
    if (strlen(filename) < 4 || stricmp(filename + strlen(filename) - 4, ".OQE") != 0) {
        change_extension(filename, ".OQE");
    }
    
    /* Get password - use password_dialog (English only, shows asterisks) */
    password[0] = '\0';
    if (!password_dialog("Password", "Enter password (English/numbers):", password, sizeof(password))) {
        return 0;
    }
    if (strlen(password) < 4) {
        show_dialog("Error", "Password must be at least 4 characters!");
        return 0;
    }
    
    /* Confirm password */
    confirm[0] = '\0';
    if (!password_dialog("Confirm", "Confirm password:", confirm, sizeof(confirm))) {
        return 0;
    }
    if (strcmp(password, confirm) != 0) {
        show_dialog("Error", "Passwords do not match!");
        return 0;
    }
    
    return save_file_encrypted(filename, password);
}

/* Remove password protection - save as regular file */
int remove_password(void)
{
    char filename[256];
    
    if (!doc.encrypted) {
        show_dialog("Info", "Document is not encrypted");
        return 0;
    }
    
    strcpy(filename, doc.filename);
    
    /* Change extension from .OQE to .TXT */
    restore_extension(filename);
    
    if (!file_dialog("Save Without Password", filename, sizeof(filename), 1)) {
        return 0;
    }
    
    doc.encrypted = 0;
    doc.password[0] = '\0';
    
    if (save_file(filename)) {
        show_dialog("Success", "Password protection removed");
        return 1;
    }
    return 0;
}

/* =========== Drawing Functions =========== */

void draw_menu_bar(void)
{
    int x = 1;
    char mode_str[16];
    
    fill_rect(0, 0, SCREEN_WIDTH - 1, 0, ' ', CLR_MENU);
    write_char(x, 0, 'F', CLR_MENU_HOT); write_string(x + 1, 0, "ile", CLR_MENU); x += 6;
    write_char(x, 0, 'E', CLR_MENU_HOT); write_string(x + 1, 0, "dit", CLR_MENU); x += 6;
    write_char(x, 0, 'S', CLR_MENU_HOT); write_string(x + 1, 0, "earch", CLR_MENU); x += 8;
    write_char(x, 0, 'B', CLR_MENU_HOT); write_string(x + 1, 0, "lock", CLR_MENU); x += 7;
    write_char(x, 0, 'O', CLR_MENU_HOT); write_string(x + 1, 0, "ptions", CLR_MENU); x += 9;
    write_char(x, 0, 'T', CLR_MENU_HOT); write_string(x + 1, 0, "ools", CLR_MENU); x += 7;
    write_char(x, 0, 'H', CLR_MENU_HOT); write_string(x + 1, 0, "elp", CLR_MENU);
    
    if (doc.input_lang == LANG_HEB) {
        if (doc.embedded_ltr) strcpy(mode_str, "[HEB+ENG]");
        else strcpy(mode_str, "[Hebrew]");
    } else if (doc.input_lang == LANG_ARA) {
        if (doc.embedded_ltr) strcpy(mode_str, "[ARA+ENG]");
        else strcpy(mode_str, "[Arabic]");
    } else if (doc.input_lang == LANG_RUS) {
        strcpy(mode_str, "[Russian]");
    } else {
        strcpy(mode_str, "[English]");
    }
    write_string(SCREEN_WIDTH - strlen(mode_str) - 1, 0, mode_str, CLR_MENU);
}

void draw_ruler(void)
{
    int i;
    char num[8];
    fill_rect(0, 1, SCREEN_WIDTH - 1, 1, g_ascii_boxes ? '-' : 0xC4, CLR_TITLE);
    
    /* Draw margin markers */
    write_char(LEFT_MARGIN, 1, '[', CLR_MENU);
    write_char(RIGHT_MARGIN, 1, ']', CLR_MENU);
    
    if (doc.rtl_mode) {
        for (i = 10; i < SCREEN_WIDTH; i += 10) {
            sprintf(num, "%d", i);
            write_string(SCREEN_WIDTH - i, 1, num, CLR_TITLE);
        }
    } else {
        for (i = 10; i < SCREEN_WIDTH; i += 10) {
            sprintf(num, "%d", i);
            write_string(i - (int)strlen(num) + 1, 1, num, CLR_TITLE);
        }
    }
}

void draw_line(int screen_row, int doc_line)
{
    int x, len, in_block, text_start, vis_idx, log_idx;
    int by1, by2, bx1, bx2;
    unsigned char ch, attr;
    char *line;
    
    /* Clear entire line first */
    fill_rect(0, screen_row, SCREEN_WIDTH - 1, screen_row, ' ', CLR_NORMAL);
    
    if (doc_line < 0 || doc_line >= doc.num_lines) {
        return;
    }
    line = doc.lines[doc_line];
    if (!line) {
        return;
    }
    len = strlen(line);
    if (len > TEXT_WIDTH) len = TEXT_WIDTH;
    
    /* Perform BiDi reordering */
    bidi_reorder(line, strlen(line));
    
    if (doc.rtl_mode) {
        /* RTL mode: right-align text to RIGHT_MARGIN */
        text_start = RIGHT_MARGIN - bidi.len;  /* Use clean length */
        if (text_start < LEFT_MARGIN) text_start = LEFT_MARGIN;
        
        for (x = LEFT_MARGIN; x <= RIGHT_MARGIN; x++) {
            vis_idx = x - text_start;
            log_idx = -1;
            
            if (vis_idx >= 0 && vis_idx < bidi.len) {
                log_idx = visual_to_logical(vis_idx);
            }
            
            in_block = 0;
            if (doc.block_active && log_idx >= 0 && log_idx < bidi.len) {
                by1 = doc.block_start_y < doc.block_end_y ? doc.block_start_y : doc.block_end_y;
                by2 = doc.block_start_y > doc.block_end_y ? doc.block_start_y : doc.block_end_y;
                if (doc_line >= by1 && doc_line <= by2) {
                    if (doc.block_start_y == doc.block_end_y) {
                        bx1 = doc.block_start_x < doc.block_end_x ? doc.block_start_x : doc.block_end_x;
                        bx2 = doc.block_start_x > doc.block_end_x ? doc.block_start_x : doc.block_end_x;
                        in_block = (log_idx >= bx1 && log_idx < bx2);
                    } else if (doc_line == by1) {
                        bx1 = (doc.block_start_y < doc.block_end_y) ? doc.block_start_x : doc.block_end_x;
                        in_block = (log_idx >= bx1);
                    } else if (doc_line == by2) {
                        bx2 = (doc.block_start_y > doc.block_end_y) ? doc.block_start_x : doc.block_end_x;
                        in_block = (log_idx < bx2);
                    } else {
                        in_block = 1;
                    }
                }
            }
            attr = in_block ? CLR_BLOCK : CLR_NORMAL;
            
            if (vis_idx >= 0 && vis_idx < bidi.len) {
                ch = bidi.visual[vis_idx];
                if (!in_block) attr = bidi.attrs[vis_idx];
            } else {
                ch = ' ';
            }
            write_char(x, screen_row, ch, attr);
        }
    } else {
        /* LTR mode: left-align text from LEFT_MARGIN */
        for (x = LEFT_MARGIN; x <= RIGHT_MARGIN; x++) {
            int i = x - LEFT_MARGIN + doc.scroll_x;
            in_block = 0;
            if (doc.block_active && i >= 0 && i < bidi.len) {
                by1 = doc.block_start_y < doc.block_end_y ? doc.block_start_y : doc.block_end_y;
                by2 = doc.block_start_y > doc.block_end_y ? doc.block_start_y : doc.block_end_y;
                if (doc_line >= by1 && doc_line <= by2) {
                    if (doc.block_start_y == doc.block_end_y) {
                        bx1 = doc.block_start_x < doc.block_end_x ? doc.block_start_x : doc.block_end_x;
                        bx2 = doc.block_start_x > doc.block_end_x ? doc.block_start_x : doc.block_end_x;
                        in_block = (i >= bx1 && i < bx2);
                    } else if (doc_line == by1) {
                        bx1 = (doc.block_start_y < doc.block_end_y) ? doc.block_start_x : doc.block_end_x;
                        in_block = (i >= bx1);
                    } else if (doc_line == by2) {
                        bx2 = (doc.block_start_y > doc.block_end_y) ? doc.block_start_x : doc.block_end_x;
                        in_block = (i < bx2);
                    } else {
                        in_block = 1;
                    }
                }
            }
            attr = in_block ? CLR_BLOCK : CLR_NORMAL;
            if (i >= 0 && i < bidi.len) {
                ch = bidi.visual[i];
                if (!in_block) attr = bidi.attrs[i];
            } else {
                ch = ' ';
            }
            write_char(x, screen_row, ch, attr);
        }
    }
}

void draw_status_bar(void)
{
    char status[128], pos[64], timebuf[8], pagebuf[16];
    time_t now;
    struct tm *t;
    int cur_page, total_pages;
    
    /* Calculate pages */
    total_pages = (doc.num_lines + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    if (total_pages < 1) total_pages = 1;
    cur_page = (doc.cursor_y / LINES_PER_PAGE) + 1;
    
    fill_rect(0, STATUS_LINE, SCREEN_WIDTH - 1, STATUS_LINE, ' ', CLR_STATUS);
    sprintf(status, " %s%s", doc.filename, doc.modified ? " *" : "");
    write_string(0, STATUS_LINE, status, CLR_STATUS);
    
    now = time(NULL);
    t = localtime(&now);
    sprintf(timebuf, "%02d:%02d", t->tm_hour, t->tm_min);
    write_string(SCREEN_WIDTH - 6, STATUS_LINE, timebuf, CLR_STATUS);
    
    /* Page indicator */
    sprintf(pagebuf, "Pg %d/%d", cur_page, total_pages);
    write_string(SCREEN_WIDTH - 15, STATUS_LINE, pagebuf, CLR_STATUS);
    
    sprintf(pos, "Ln %d/%d Col %d %s %s %s%s%s%s",
            doc.cursor_y + 1, doc.num_lines, doc.cursor_x + 1,
            doc.insert_mode ? "INS" : "OVR",
            doc.rtl_mode ? "RTL" : "LTR",
            (doc.input_lang == LANG_HEB) ? "HEB" :
            (doc.input_lang == LANG_ARA) ? "ARA" :
            (doc.input_lang == LANG_RUS) ? "RUS" : "ENG",
            doc.embedded_ltr ? "+E" : "",
            doc.block_active ? " BLK" : "",
            doc.encrypted ? " ENC" : "");
    write_string(SCREEN_WIDTH - 58, STATUS_LINE, pos, CLR_STATUS);
}

void draw_screen(void)
{
    int i;
    fill_rect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, ' ', CLR_NORMAL);
    draw_menu_bar();
    if (doc.show_ruler) draw_ruler();
    for (i = 0; i < EDIT_HEIGHT; i++)
        draw_line(EDIT_TOP + i, doc.scroll_y + i);
    draw_status_bar();
    update_cursor();
}

void update_cursor(void)
{
    int screen_x, screen_y, len, text_start, clean_cursor_x;
    char *line;
    
    screen_y = doc.cursor_y - doc.scroll_y + EDIT_TOP;
    line = doc.lines[doc.cursor_y];
    len = line ? (int)strlen(line) : 0;
    
    if (line && len > 0) bidi_reorder(line, len);
    
    /* Convert cursor_x to clean position (without format codes) */
    clean_cursor_x = cursor_to_clean_pos(line, doc.cursor_x);
    
    if (doc.rtl_mode) {
        /* RTL: text is right-aligned to RIGHT_MARGIN */
        text_start = RIGHT_MARGIN - bidi.len;
        if (text_start < LEFT_MARGIN) text_start = LEFT_MARGIN;
        
        if (clean_cursor_x >= bidi.len) {
            screen_x = text_start - 1;
            if (screen_x < LEFT_MARGIN) screen_x = LEFT_MARGIN;
        } else {
            int vis_pos = logical_to_visual(clean_cursor_x);
            screen_x = text_start + vis_pos;
        }
        if (screen_x < LEFT_MARGIN) screen_x = LEFT_MARGIN;
        if (screen_x > RIGHT_MARGIN) screen_x = RIGHT_MARGIN;
    } else {
        /* LTR: text starts at LEFT_MARGIN */
        screen_x = LEFT_MARGIN + clean_cursor_x - doc.scroll_x;
    }
    
    if (!doc.rtl_mode && (screen_x < LEFT_MARGIN || screen_x > RIGHT_MARGIN)) {
        doc.scroll_x = clean_cursor_x - TEXT_WIDTH / 2;
        if (doc.scroll_x < 0) doc.scroll_x = 0;
        screen_x = LEFT_MARGIN + clean_cursor_x - doc.scroll_x;
    }
    if (screen_y < EDIT_TOP || screen_y > EDIT_BOTTOM) {
        if (screen_y < EDIT_TOP) doc.scroll_y = doc.cursor_y;
        else doc.scroll_y = doc.cursor_y - EDIT_HEIGHT + 1;
        if (doc.scroll_y < 0) doc.scroll_y = 0;
        screen_y = doc.cursor_y - doc.scroll_y + EDIT_TOP;
        draw_screen();
    }
    set_cursor_pos(screen_x, screen_y);
    show_cursor(doc.insert_mode);
}

/* =========== Editing Functions =========== */

int get_key(void)
{
    int ch = getch();
    if (ch == 0 || ch == 0xE0) return 0x100 | getch();
    return ch;
}

/* Calculate visible length of line (without format codes) */
int clean_line_len(const char *line)
{
    int i, count = 0;
    if (!line) return 0;
    for (i = 0; line[i]; i++) {
        unsigned char ch = (unsigned char)line[i];
        if (ch != FMT_BOLD && ch != FMT_UNDERLINE && ch != FMT_BOLDUNDER) {
            count++;
        }
    }
    return count;
}

/* Find position in line that corresponds to visible position */
int visible_to_raw_pos(const char *line, int visible_pos)
{
    int i, count = 0;
    if (!line) return 0;
    for (i = 0; line[i]; i++) {
        unsigned char ch = (unsigned char)line[i];
        if (ch != FMT_BOLD && ch != FMT_UNDERLINE && ch != FMT_BOLDUNDER) {
            if (count == visible_pos) return i;
            count++;
        }
    }
    return i;  /* Return end of string if not found */
}

void do_word_wrap(void)
{
    char *line, *new_line;
    int len, visible_len, wrap_pos, raw_wrap_pos, i;
    int cursor_was_past_wrap;
    int space_at_wrap;
    
    if (!doc.word_wrap) return;
    if (doc.cursor_y >= doc.num_lines) return;
    
    line = doc.lines[doc.cursor_y];
    if (!line) return;
    
    /* Use visible length (without format codes) for wrap check */
    visible_len = clean_line_len(line);
    
    /* Check if visible line exceeds text width */
    if (visible_len <= TEXT_WIDTH) return;
    
    len = strlen(line);
    
    /* Find wrap position - look for last space before or at visible limit */
    /* We need to find the raw position that corresponds to visible TEXT_WIDTH */
    wrap_pos = TEXT_WIDTH;
    raw_wrap_pos = visible_to_raw_pos(line, TEXT_WIDTH);
    
    /* Search backwards for a space */
    for (i = raw_wrap_pos; i > 0; i--) {
        unsigned char ch = (unsigned char)line[i];
        if (ch == ' ') {
            raw_wrap_pos = i;
            break;
        }
    }
    
    /* If no space found, use the TEXT_WIDTH position */
    if (i == 0) raw_wrap_pos = visible_to_raw_pos(line, TEXT_WIDTH);
    
    /* Remember if cursor was past wrap point */
    cursor_was_past_wrap = (doc.cursor_x > raw_wrap_pos);
    
    /* Remember if there was a space at wrap point */
    space_at_wrap = (line[raw_wrap_pos] == ' ');
    
    /* Create new line with wrapped text */
    if (doc.num_lines >= MAX_LINES) return;
    
    new_line = (char *)malloc(MAX_LINE_LEN);
    if (!new_line) return;
    
    /* Copy text after wrap point to new line */
    if (space_at_wrap) {
        strcpy(new_line, line + raw_wrap_pos + 1);
    } else {
        strcpy(new_line, line + raw_wrap_pos);
    }
    line[raw_wrap_pos] = '\0';
    
    /* Insert new line */
    for (i = doc.num_lines; i > doc.cursor_y + 1; i--)
        doc.lines[i] = doc.lines[i - 1];
    doc.lines[doc.cursor_y + 1] = new_line;
    doc.num_lines++;
    
    /* Move cursor to new line if it was past wrap point */
    if (cursor_was_past_wrap) {
        doc.cursor_y++;
        if (space_at_wrap) {
            doc.cursor_x = doc.cursor_x - raw_wrap_pos - 1;
        } else {
            doc.cursor_x = doc.cursor_x - raw_wrap_pos;
        }
        if (doc.cursor_x < 0) doc.cursor_x = 0;
        
        /* Make sure cursor is visible (scroll if needed) */
        if (doc.cursor_y >= doc.scroll_y + EDIT_HEIGHT) {
            doc.scroll_y = doc.cursor_y - EDIT_HEIGHT + 1;
        }
    }
    
    /* Refresh screen to show the change */
    draw_screen();
}

void insert_char(unsigned char ch)
{
    char *line;
    int len;
    if (doc.cursor_y >= doc.num_lines) return;
    line = doc.lines[doc.cursor_y];
    if (!line) return;
    len = strlen(line);
    if (len >= MAX_LINE_LEN - 1) return;
    doc.modified = 1;
    
    /* Save for undo */
    push_undo(0, doc.cursor_y, doc.cursor_x, ch, NULL);
    
    if (doc.insert_mode) {
        memmove(line + doc.cursor_x + 1, line + doc.cursor_x, len - doc.cursor_x + 1);
        line[doc.cursor_x] = ch;
    } else {
        if (doc.cursor_x >= len) {
            while (len < doc.cursor_x) line[len++] = ' ';
            line[doc.cursor_x] = ch;
            line[doc.cursor_x + 1] = '\0';
        } else {
            line[doc.cursor_x] = ch;
        }
    }
    doc.cursor_x++;
    
    /* Apply word wrap if enabled and visible line is too long */
    if (doc.word_wrap && clean_line_len(line) > TEXT_WIDTH) {
        do_word_wrap();
    }
}

void delete_char(void)
{
    char *line;
    int len;
    unsigned char deleted_ch;
    int need_full_redraw = 0;
    if (doc.cursor_y >= doc.num_lines) return;
    line = doc.lines[doc.cursor_y];
    if (!line) return;
    len = strlen(line);
    
    if (doc.cursor_x < len) {
        deleted_ch = (unsigned char)line[doc.cursor_x];
        push_undo(1, doc.cursor_y, doc.cursor_x, deleted_ch, NULL);
        memmove(line + doc.cursor_x, line + doc.cursor_x + 1, len - doc.cursor_x);
        doc.modified = 1;
    } else if (doc.cursor_y < doc.num_lines - 1) {
        char *next = doc.lines[doc.cursor_y + 1];
        int next_len = next ? (int)strlen(next) : 0;
        if (len + next_len < MAX_LINE_LEN - 1) {
            if (next) strcat(line, next);
            free(doc.lines[doc.cursor_y + 1]);
            memmove(&doc.lines[doc.cursor_y + 1], &doc.lines[doc.cursor_y + 2],
                    (doc.num_lines - doc.cursor_y - 2) * sizeof(char *));
            doc.num_lines--;
            doc.modified = 1;
            need_full_redraw = 1;
        }
    }
    
    if (need_full_redraw) {
        draw_screen();
    } else {
        /* Only redraw current line */
        int screen_row = EDIT_TOP + (doc.cursor_y - doc.scroll_y);
        if (screen_row >= EDIT_TOP && screen_row < EDIT_TOP + EDIT_HEIGHT) {
            draw_line(screen_row, doc.cursor_y);
        }
    }
}

void backspace_char(void)
{
    char *line;
    unsigned char deleted_ch;
    
    if (doc.cursor_x > 0) {
        line = doc.lines[doc.cursor_y];
        if (line && doc.cursor_x <= (int)strlen(line)) {
            deleted_ch = (unsigned char)line[doc.cursor_x - 1];
            push_undo(4, doc.cursor_y, doc.cursor_x - 1, deleted_ch, NULL);
        }
        doc.cursor_x--;
        delete_char();
    } else if (doc.cursor_y > 0) {
        doc.cursor_y--;
        doc.cursor_x = strlen(doc.lines[doc.cursor_y]);
        delete_char();
    }
}

void insert_line(void)
{
    char *line, *new_line;
    int len, i;
    if (doc.num_lines >= MAX_LINES) return;
    line = doc.lines[doc.cursor_y];
    len = line ? (int)strlen(line) : 0;
    new_line = (char *)malloc(MAX_LINE_LEN);
    if (!new_line) return;
    
    if (line && doc.cursor_x < len) {
        strcpy(new_line, line + doc.cursor_x);
        line[doc.cursor_x] = '\0';
    } else {
        new_line[0] = '\0';
    }
    
    for (i = doc.num_lines; i > doc.cursor_y + 1; i--)
        doc.lines[i] = doc.lines[i - 1];
    doc.lines[doc.cursor_y + 1] = new_line;
    doc.num_lines++;
    
    /* Save for undo */
    push_undo(2, doc.cursor_y + 1, doc.cursor_x, 0, NULL);
    
    doc.cursor_y++;
    doc.cursor_x = 0;
    doc.modified = 1;
}

void delete_line(void)
{
    int i;
    char *saved_line;
    
    if (doc.num_lines <= 1) {
        doc.lines[0][0] = '\0';
        doc.cursor_x = 0;
        return;
    }
    
    /* Save line for undo */
    saved_line = doc.lines[doc.cursor_y];
    push_undo(3, doc.cursor_y, 0, 0, saved_line);
    
    free(doc.lines[doc.cursor_y]);
    for (i = doc.cursor_y; i < doc.num_lines - 1; i++)
        doc.lines[i] = doc.lines[i + 1];
    doc.num_lines--;
    if (doc.cursor_y >= doc.num_lines) doc.cursor_y = doc.num_lines - 1;
    if (doc.cursor_x > (int)strlen(doc.lines[doc.cursor_y]))
        doc.cursor_x = strlen(doc.lines[doc.cursor_y]);
    doc.modified = 1;
}

void move_cursor(int dx, int dy)
{
    char *line;
    int len;
    
    doc.cursor_y += dy;
    if (doc.cursor_y < 0) doc.cursor_y = 0;
    if (doc.cursor_y >= doc.num_lines) doc.cursor_y = doc.num_lines - 1;
    
    if (doc.rtl_mode) dx = -dx;
    
    doc.cursor_x += dx;
    line = doc.lines[doc.cursor_y];
    len = line ? (int)strlen(line) : 0;
    
    if (doc.cursor_x < 0) {
        if (doc.cursor_y > 0) {
            doc.cursor_y--;
            doc.cursor_x = strlen(doc.lines[doc.cursor_y]);
        } else {
            doc.cursor_x = 0;
        }
    }
    if (doc.cursor_x > len) {
        if (dx != 0 && doc.cursor_y < doc.num_lines - 1) {
            doc.cursor_y++;
            doc.cursor_x = 0;
        } else {
            doc.cursor_x = len;
        }
    }
    
    if (doc.block_active) {
        doc.block_end_x = doc.cursor_x;
        doc.block_end_y = doc.cursor_y;
    }
    update_cursor();
}

void home_key(void) { 
    if (doc.rtl_mode) {
        /* RTL: Home goes to end of line (right side visually) */
        doc.cursor_x = strlen(doc.lines[doc.cursor_y]);
    } else {
        doc.cursor_x = 0; 
    }
    update_cursor(); 
}

void end_key(void) { 
    if (doc.rtl_mode) {
        /* RTL: End goes to start of line (left side visually) */
        doc.cursor_x = 0;
    } else {
        doc.cursor_x = strlen(doc.lines[doc.cursor_y]); 
    }
    update_cursor(); 
}

void page_up(void)
{
    int len;
    doc.cursor_y -= EDIT_HEIGHT;
    if (doc.cursor_y < 0) doc.cursor_y = 0;
    doc.scroll_y -= EDIT_HEIGHT;
    if (doc.scroll_y < 0) doc.scroll_y = 0;
    len = strlen(doc.lines[doc.cursor_y]);
    if (doc.cursor_x > len) doc.cursor_x = len;
    draw_screen();
}

void page_down(void)
{
    int len;
    doc.cursor_y += EDIT_HEIGHT;
    if (doc.cursor_y >= doc.num_lines) doc.cursor_y = doc.num_lines - 1;
    doc.scroll_y += EDIT_HEIGHT;
    if (doc.scroll_y > doc.num_lines - EDIT_HEIGHT) {
        doc.scroll_y = doc.num_lines - EDIT_HEIGHT;
        if (doc.scroll_y < 0) doc.scroll_y = 0;
    }
    len = strlen(doc.lines[doc.cursor_y]);
    if (doc.cursor_x > len) doc.cursor_x = len;
    draw_screen();
}

void ctrl_home(void) { doc.cursor_y = doc.scroll_x = doc.scroll_y = doc.cursor_x = 0; draw_screen(); }
void ctrl_end(void) { 
    doc.cursor_y = doc.num_lines - 1; 
    doc.cursor_x = strlen(doc.lines[doc.cursor_y]); 
    draw_screen(); 
}

void toggle_insert(void) { doc.insert_mode = !doc.insert_mode; show_cursor(doc.insert_mode); draw_status_bar(); }
void cycle_input_lang(void) {
    doc.input_lang = (doc.input_lang + 1) % LANG_COUNT;
    doc.embedded_ltr = 0;
    /* Auto-set paragraph direction to match the new language so F4 alone
     * is enough to flip into a usable Russian (or English) session. The
     * user can still override with F5. */
    doc.rtl_mode = (doc.input_lang == LANG_HEB || doc.input_lang == LANG_ARA);
    doc.scroll_x = 0;
    draw_screen();
}
void toggle_embedded_ltr(void) { if (is_rtl_input()) { doc.embedded_ltr = !doc.embedded_ltr; draw_menu_bar(); draw_status_bar(); } }
void toggle_rtl(void) { doc.rtl_mode = !doc.rtl_mode; doc.scroll_x = 0; draw_screen(); }
void toggle_wrap(void) { doc.word_wrap = !doc.word_wrap; }
void toggle_save_reminder(void) { doc.save_reminder = !doc.save_reminder; show_dialog("Options", doc.save_reminder ? "Save reminder ON" : "Save reminder OFF"); }

/* Convert cursor position (with format codes) to clean position (without) */
int cursor_to_clean_pos(const char *line, int cursor_x)
{
    int i, clean_pos = 0;
    if (!line) return 0;
    for (i = 0; i < cursor_x && line[i]; i++) {
        unsigned char ch = (unsigned char)line[i];
        if (ch != FMT_BOLD && ch != FMT_UNDERLINE && ch != FMT_BOLDUNDER) {
            clean_pos++;
        }
    }
    return clean_pos;
}

void insert_format_code(unsigned char code)
{
    char *line;
    int len;
    if (doc.cursor_y >= doc.num_lines) return;
    line = doc.lines[doc.cursor_y];
    if (!line) return;
    len = strlen(line);
    if (len >= MAX_LINE_LEN - 1) return;
    
    memmove(line + doc.cursor_x + 1, line + doc.cursor_x, len - doc.cursor_x + 1);
    line[doc.cursor_x] = code;
    doc.cursor_x++;  /* Move past format code */
    doc.modified = 1;
    draw_screen();
}

void toggle_bold(void) { insert_format_code(FMT_BOLD); }
void toggle_underline(void) { insert_format_code(FMT_UNDERLINE); }
void toggle_boldunder(void) { insert_format_code(FMT_BOLDUNDER); }

/* =========== Dialog Functions =========== */

void show_dialog(const char *title, const char *msg)
{
    int w, h, x1, y1, x2, y2, msg_len, title_len;
    msg_len = strlen(msg);
    title_len = strlen(title);
    w = msg_len + 6;
    if (title_len + 4 > w) w = title_len + 4;
    if (w < 20) w = 20;
    if (w > 70) w = 70;
    h = 5;
    x1 = (SCREEN_WIDTH - w) / 2;
    y1 = (SCREEN_HEIGHT - h) / 2;
    x2 = x1 + w - 1;
    y2 = y1 + h - 1;
    draw_box(x1, y1, x2, y2, CLR_DIALOG, title);
    write_string(x1 + 3, y1 + 2, msg, CLR_DIALOG);
    write_string(x1 + w/2 - 2, y1 + 4, " OK ", CLR_DIALOG_BTN);
    hide_cursor();
    while (kbhit()) getch();
    getch();
    draw_screen();
}

int confirm_dialog(const char *title, const char *msg)
{
    int w, h, x1, y1, x2, y2, key, result = -1, msg_len, title_len, selected = 0;
    msg_len = strlen(msg);
    title_len = strlen(title);
    w = msg_len + 6;
    if (title_len + 4 > w) w = title_len + 4;
    if (w < 30) w = 30;
    if (w > 70) w = 70;
    h = 6;
    x1 = (SCREEN_WIDTH - w) / 2;
    y1 = (SCREEN_HEIGHT - h) / 2;
    x2 = x1 + w - 1;
    y2 = y1 + h - 1;
    while (kbhit()) getch();
    while (result == -1) {
        draw_box(x1, y1, x2, y2, CLR_DIALOG, title);
        write_string(x1 + 3, y1 + 2, msg, CLR_DIALOG);
        write_string(x1 + w/2 - 8, y1 + 4, selected == 0 ? "[Yes]" : " Yes ", selected == 0 ? CLR_DIALOG_BTN : CLR_DIALOG);
        write_string(x1 + w/2 + 3, y1 + 4, selected == 1 ? "[No]" : " No ", selected == 1 ? CLR_DIALOG_BTN : CLR_DIALOG);
        hide_cursor();
        key = getch();
        if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_LEFT || key == KEY_RIGHT) selected = 1 - selected; }
        else { key = toupper(key); if (key == 'Y') result = 1; else if (key == 'N') result = 0; else if (key == KEY_ENTER) result = (selected == 0) ? 1 : 0; else if (key == KEY_ESC) result = 0; else if (key == KEY_TAB || key == ' ') selected = 1 - selected; }
    }
    draw_screen();
    return result;
}

int input_dialog(const char *title, const char *prompt, char *buffer, int maxlen)
{
    int w = 60, h = 6, x1, y1, x2, y2, key, pos, len, field_x, field_w, ext_key, i;
    unsigned char ch;
    char display[256];
    
    x1 = (SCREEN_WIDTH - w) / 2; y1 = (SCREEN_HEIGHT - h) / 2; x2 = x1 + w - 1; y2 = y1 + h - 1;
    draw_box(x1, y1, x2, y2, CLR_DIALOG, title);
    write_string(x1 + 2, y1 + 2, prompt, CLR_DIALOG);
    field_x = x1 + 2; field_w = w - 4;
    pos = strlen(buffer);
    show_cursor(1);
    
    while (1) {
        len = strlen(buffer);
        
        /* Display: reverse for RTL (Hebrew or Arabic) input */
        fill_rect(field_x, y1 + 3, field_x + field_w - 1, y1 + 3, ' ', CLR_MENU);
        if (is_rtl_input() && len > 0) {
            /* Reverse display for RTL */
            for (i = 0; i < len && i < field_w; i++) {
                display[i] = buffer[len - 1 - i];
            }
            display[i] = '\0';
            write_string(field_x + field_w - len, y1 + 3, display, CLR_MENU);
            set_cursor_pos(field_x + field_w - pos, y1 + 3);
        } else {
            write_string(field_x, y1 + 3, buffer, CLR_MENU);
            set_cursor_pos(field_x + pos, y1 + 3);
        }

        key = getch();
        if (key == KEY_ENTER) { draw_screen(); return 1; }
        if (key == KEY_ESC) { draw_screen(); return 0; }
        if (key == KEY_BACKSPACE) {
            if (pos > 0) {
                memmove(buffer + pos - 1, buffer + pos, len - pos + 1);
                pos--;
            }
        }
        else if (key == 0 || key == 0xE0) {
            ext_key = getch();
            if (is_rtl_input()) {
                /* Reverse arrow directions for RTL */
                if (ext_key == KEY_LEFT && pos < len) pos++;
                if (ext_key == KEY_RIGHT && pos > 0) pos--;
            } else {
                if (ext_key == KEY_LEFT && pos > 0) pos--;
                if (ext_key == KEY_RIGHT && pos < len) pos++;
            }
            if (ext_key == KEY_HOME) pos = is_rtl_input() ? len : 0;
            if (ext_key == KEY_END) pos = is_rtl_input() ? 0 : len;
            if (ext_key == KEY_DELETE && pos < len) memmove(buffer + pos, buffer + pos + 1, len - pos);
        }
        else if (key >= 32 && key < 256 && len < maxlen - 1) {
            ch = (unsigned char)key;
            /* Translate to active script (Hebrew, Arabic, or Russian) */
            if (doc.input_lang == LANG_HEB) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_hebrew(ch);
                    if (m) ch = m;
                }
            } else if (doc.input_lang == LANG_ARA) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_arabic(ch);
                    if (m) ch = m;
                }
            } else if (doc.input_lang == LANG_RUS) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_russian(ch);
                    if (m) ch = m;
                }
            }
            memmove(buffer + pos + 1, buffer + pos, len - pos + 1);
            buffer[pos] = ch;
            pos++;
        }
    }
}

/* Password input dialog - English only, shows asterisks */
int password_dialog(const char *title, const char *prompt, char *buffer, int maxlen)
{
    int w = 50, h = 6, x1, y1, x2, y2, key, pos, len, field_x, field_w, ext_key, i;
    char display[256];
    
    x1 = (SCREEN_WIDTH - w) / 2; y1 = (SCREEN_HEIGHT - h) / 2; x2 = x1 + w - 1; y2 = y1 + h - 1;
    draw_box(x1, y1, x2, y2, CLR_DIALOG, title);
    write_string(x1 + 2, y1 + 2, prompt, CLR_DIALOG);
    field_x = x1 + 2; field_w = w - 4;
    pos = strlen(buffer);
    show_cursor(1);
    
    while (1) {
        len = strlen(buffer);
        
        /* Display asterisks instead of actual characters */
        fill_rect(field_x, y1 + 3, field_x + field_w - 1, y1 + 3, ' ', CLR_MENU);
        for (i = 0; i < len && i < field_w; i++) {
            display[i] = '*';
        }
        display[i] = '\0';
        write_string(field_x, y1 + 3, display, CLR_MENU);
        set_cursor_pos(field_x + pos, y1 + 3);
        
        key = getch();
        if (key == KEY_ENTER) { draw_screen(); return 1; }
        if (key == KEY_ESC) { draw_screen(); return 0; }
        if (key == KEY_BACKSPACE) { 
            if (pos > 0) { 
                memmove(buffer + pos - 1, buffer + pos, len - pos + 1); 
                pos--; 
            } 
        }
        else if (key == 0 || key == 0xE0) { 
            ext_key = getch(); 
            if (ext_key == KEY_LEFT && pos > 0) pos--; 
            if (ext_key == KEY_RIGHT && pos < len) pos++;
            if (ext_key == KEY_HOME) pos = 0; 
            if (ext_key == KEY_END) pos = len; 
            if (ext_key == KEY_DELETE && pos < len) memmove(buffer + pos, buffer + pos + 1, len - pos); 
        }
        else if (key >= 32 && key < 127 && len < maxlen - 1) {
            /* Only ASCII characters allowed for password */
            memmove(buffer + pos + 1, buffer + pos, len - pos + 1);
            buffer[pos] = (char)key;
            pos++;
        }
    }
}

/* =========== Search Functions =========== */

/* Search input dialog - supports English/Hebrew/Arabic with F4 cycling */
int search_dialog(const char *title, const char *prompt, char *buffer, int maxlen)
{
    int w = 60, h = 7, x1, y1, x2, y2, key, pos, len, field_x, field_w, ext_key, i;
    unsigned char ch;
    int search_lang = doc.input_lang;  /* Start with document's mode */
    char mode_str[12];
    char display[256];
    
    x1 = (SCREEN_WIDTH - w) / 2; y1 = (SCREEN_HEIGHT - h) / 2; x2 = x1 + w - 1; y2 = y1 + h - 1;
    field_x = x1 + 2; field_w = w - 4;
    pos = strlen(buffer);
    show_cursor(1);
    
    while (1) {
        len = strlen(buffer);
        
        /* Draw dialog */
        draw_box(x1, y1, x2, y2, CLR_DIALOG, title);
        write_string(x1 + 2, y1 + 2, prompt, CLR_DIALOG);
        
        /* Show current mode and F4 hint */
        sprintf(mode_str, "[%s] F4",
                (search_lang == LANG_HEB) ? "HEB" :
                (search_lang == LANG_ARA) ? "ARA" :
                (search_lang == LANG_RUS) ? "RUS" : "ENG");
        write_string(x2 - 12, y1 + 2, mode_str, CLR_DIALOG);
        
        /* Display text field with BiDi reordering */
        fill_rect(field_x, y1 + 3, field_x + field_w - 1, y1 + 3, ' ', CLR_MENU);
        if (len > 0) {
            bidi_reorder(buffer, len);
            for (i = 0; i < bidi.len && i < field_w; i++) {
                display[i] = bidi.visual[i];
            }
            display[i] = '\0';
            write_string(field_x, y1 + 3, display, CLR_MENU);
            /* Position cursor using visual position */
            set_cursor_pos(field_x + logical_to_visual(pos), y1 + 3);
        } else {
            set_cursor_pos(field_x, y1 + 3);
        }
        
        key = getch();
        if (key == KEY_ENTER) { draw_screen(); return 1; }
        if (key == KEY_ESC) { draw_screen(); return 0; }
        if (key == KEY_BACKSPACE) { 
            if (pos > 0) { 
                memmove(buffer + pos - 1, buffer + pos, len - pos + 1); 
                pos--; 
            } 
        }
        else if (key == 0 || key == 0xE0) { 
            ext_key = getch(); 
            if (ext_key == KEY_LEFT && pos > 0) pos--; 
            if (ext_key == KEY_RIGHT && pos < len) pos++;
            if (ext_key == KEY_HOME) pos = 0; 
            if (ext_key == KEY_END) pos = len; 
            if (ext_key == KEY_DELETE && pos < len) memmove(buffer + pos, buffer + pos + 1, len - pos);
            if (ext_key == KEY_F4) search_lang = (search_lang + 1) % LANG_COUNT;  /* Cycle ENG->HEB->ARA->RUS */
        }
        else if (key >= 32 && key < 256 && len < maxlen - 1) {
            ch = (unsigned char)key;
            /* Translate to active script */
            if (search_lang == LANG_HEB) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_hebrew(ch);
                    if (m) ch = m;
                }
            } else if (search_lang == LANG_ARA) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_arabic(ch);
                    if (m) ch = m;
                }
            } else if (search_lang == LANG_RUS) {
                if (ch >= 32 && ch < 128 && !(ch >= '0' && ch <= '9')) {
                    unsigned char m = translate_russian(ch);
                    if (m) ch = m;
                }
            }
            memmove(buffer + pos + 1, buffer + pos, len - pos + 1);
            buffer[pos] = ch;
            pos++;
        }
    }
}

void do_search(void) { 
    if (search_dialog("Find", "Search for:", search_text, sizeof(search_text))) 
        find_next(); 
}

void find_next(void)
{
    int i, start_x, has_hebrew = 0; 
    char *found;
    
    if (search_text[0] == '\0') { 
        show_dialog("Find", "No search text - use F7 first"); 
        return; 
    }
    
    /* Check if search text contains Hebrew */
    for (i = 0; search_text[i]; i++) {
        if ((unsigned char)search_text[i] >= 0x80) { has_hebrew = 1; break; }
    }
    
    start_x = doc.cursor_x + 1;
    for (i = doc.cursor_y; i < doc.num_lines; i++) {
        if (!doc.lines[i]) continue;
        /* Use exact match for Hebrew, case-insensitive for English only */
        if (has_hebrew) {
            found = strstr(doc.lines[i] + (i == doc.cursor_y ? start_x : 0), search_text);
        } else {
            found = search_case ? strstr(doc.lines[i] + (i == doc.cursor_y ? start_x : 0), search_text) 
                                : stristr(doc.lines[i] + (i == doc.cursor_y ? start_x : 0), search_text);
        }
        if (found) { doc.cursor_y = i; doc.cursor_x = found - doc.lines[i]; draw_screen(); return; }
    }
    show_dialog("Find", "Text not found");
}

void find_prev(void)
{
    int i, has_hebrew = 0; 
    char *line, *found, *last_found;
    
    if (search_text[0] == '\0') { 
        show_dialog("Find", "No search text - use F7 first"); 
        return; 
    }
    
    /* Check if search text contains Hebrew */
    for (i = 0; search_text[i]; i++) {
        if ((unsigned char)search_text[i] >= 0x80) { has_hebrew = 1; break; }
    }
    
    for (i = doc.cursor_y; i >= 0; i--) {
        line = doc.lines[i]; if (!line) continue; last_found = NULL; found = line;
        while (1) { 
            if (has_hebrew) {
                found = strstr(found, search_text);
            } else {
                found = search_case ? strstr(found, search_text) : stristr(found, search_text); 
            }
            if (!found) break; 
            if (i == doc.cursor_y && (found - line) >= doc.cursor_x) break; 
            last_found = found; 
            found++; 
        }
        if (last_found) { doc.cursor_y = i; doc.cursor_x = last_found - line; draw_screen(); return; }
    }
    show_dialog("Find", "Text not found");
}

void do_replace(void) 
{ 
    char *line, *found;
    int search_len, replace_len, len, j, has_hebrew = 0;
    
    /* Always ask for search and replace text */
    if (!search_dialog("Replace", "Search for:", search_text, sizeof(search_text))) return; 
    if (!search_dialog("Replace", "Replace with:", replace_text, sizeof(replace_text))) return;
    
    /* Check if search text contains Hebrew */
    for (j = 0; search_text[j]; j++) {
        if ((unsigned char)search_text[j] >= 0x80) { has_hebrew = 1; break; }
    }
    
    search_len = strlen(search_text);
    replace_len = strlen(replace_text);
    
    line = doc.lines[doc.cursor_y];
    if (!line) { find_next(); return; }
    
    /* Check if cursor is at search text */
    if (has_hebrew) {
        found = strstr(line + doc.cursor_x, search_text);
    } else {
        found = search_case ? strstr(line + doc.cursor_x, search_text) 
                            : stristr(line + doc.cursor_x, search_text);
    }
    
    /* If at the search text, replace it */
    if (found && found == line + doc.cursor_x) {
        len = strlen(line);
        if (len - search_len + replace_len < MAX_LINE_LEN - 1) {
            memmove(found + replace_len, found + search_len, strlen(found + search_len) + 1);
            memcpy(found, replace_text, replace_len);
            doc.cursor_x += replace_len;
            doc.modified = 1;
        }
    }
    
    /* Find next occurrence */
    find_next();
}

void do_replace_all(void)
{
    int i, j, count = 0, has_hebrew = 0; 
    char *line, *found; 
    int search_len, replace_len; 
    char msg[64];
    
    /* Always ask for search and replace text */
    if (!search_dialog("Replace All", "Search for:", search_text, sizeof(search_text))) return; 
    if (!search_dialog("Replace All", "Replace with:", replace_text, sizeof(replace_text))) return;
    
    /* Check if search text contains Hebrew */
    for (j = 0; search_text[j]; j++) {
        if ((unsigned char)search_text[j] >= 0x80) { has_hebrew = 1; break; }
    }
    
    search_len = strlen(search_text); 
    replace_len = strlen(replace_text);
    
    for (i = 0; i < doc.num_lines; i++) { 
        line = doc.lines[i]; 
        if (!line) continue; 
        while (1) { 
            if (has_hebrew) {
                found = strstr(line, search_text);
            } else {
                found = search_case ? strstr(line, search_text) : stristr(line, search_text); 
            }
            if (!found) break; 
            memmove(found + replace_len, found + search_len, strlen(found + search_len) + 1); 
            memcpy(found, replace_text, replace_len); 
            line = found + replace_len; 
            count++; 
            doc.modified = 1; 
        } 
    }
    sprintf(msg, "Replaced %d occurrences", count); 
    show_dialog("Replace All", msg); 
    draw_screen();
}

/* =========== Block Operations =========== */

void start_block(void) { 
    doc.block_active = 1; 
    doc.block_start_x = doc.cursor_x; 
    doc.block_start_y = doc.cursor_y; 
    doc.block_end_x = doc.cursor_x; 
    doc.block_end_y = doc.cursor_y; 
    show_dialog("Block", "Block started - move cursor then F6 to copy");
    draw_screen(); 
}
void end_block(void) { 
    if (doc.block_active) { 
        doc.block_end_x = doc.cursor_x; 
        doc.block_end_y = doc.cursor_y; 
        draw_screen(); 
    } 
}
void unselect_block(void) { doc.block_active = 0; draw_screen(); }

void copy_block(void)
{
    int i, by1, by2, bx1, bx2, line_count; char *src;
    if (!doc.block_active) { start_block(); return; }
    
    /* Update block end to current cursor position */
    doc.block_end_x = doc.cursor_x;
    doc.block_end_y = doc.cursor_y;
    
    /* Check if block has any content */
    if (doc.block_start_x == doc.block_end_x && doc.block_start_y == doc.block_end_y) {
        show_dialog("Block", "No text selected!");
        return;
    }
    
    if (doc.clipboard) { for (i = 0; i < doc.clipboard_lines; i++) if (doc.clipboard[i]) free(doc.clipboard[i]); free(doc.clipboard); }
    by1 = doc.block_start_y < doc.block_end_y ? doc.block_start_y : doc.block_end_y;
    by2 = doc.block_start_y > doc.block_end_y ? doc.block_start_y : doc.block_end_y;
    if (doc.block_start_y == doc.block_end_y) { bx1 = doc.block_start_x < doc.block_end_x ? doc.block_start_x : doc.block_end_x; bx2 = doc.block_start_x > doc.block_end_x ? doc.block_start_x : doc.block_end_x; }
    else if (doc.block_start_y < doc.block_end_y) { bx1 = doc.block_start_x; bx2 = doc.block_end_x; }
    else { bx1 = doc.block_end_x; bx2 = doc.block_start_x; }
    line_count = by2 - by1 + 1;
    doc.clipboard = (char **)malloc(line_count * sizeof(char *)); doc.clipboard_lines = line_count;
    for (i = 0; i < line_count; i++) { src = doc.lines[by1 + i]; doc.clipboard[i] = (char *)malloc(MAX_LINE_LEN); if (line_count == 1) { strncpy(doc.clipboard[i], src + bx1, bx2 - bx1); doc.clipboard[i][bx2 - bx1] = '\0'; } else if (i == 0) { strcpy(doc.clipboard[i], src + bx1); } else if (i == line_count - 1) { strncpy(doc.clipboard[i], src, bx2); doc.clipboard[i][bx2] = '\0'; } else { strcpy(doc.clipboard[i], src); } }
    show_dialog("Block", "Block copied to clipboard");
}

void cut_block(void) { 
    if (!doc.block_active) { show_dialog("Block", "No block selected!"); return; }
    copy_block(); 
    delete_block(); 
}

void paste_block(void)
{
    int i, len; char *line;
    if (!doc.clipboard || doc.clipboard_lines == 0) { 
        show_dialog("Block", "Clipboard is empty!"); 
        return; 
    }
    if (doc.clipboard_lines == 1) { line = doc.lines[doc.cursor_y]; len = strlen(line); if (len + strlen(doc.clipboard[0]) < MAX_LINE_LEN - 1) { memmove(line + doc.cursor_x + strlen(doc.clipboard[0]), line + doc.cursor_x, len - doc.cursor_x + 1); memcpy(line + doc.cursor_x, doc.clipboard[0], strlen(doc.clipboard[0])); doc.cursor_x += strlen(doc.clipboard[0]); doc.modified = 1; } }
    else { for (i = 0; i < doc.clipboard_lines && doc.num_lines < MAX_LINES; i++) { if (i == 0) { line = doc.lines[doc.cursor_y]; len = strlen(line); if (len + strlen(doc.clipboard[0]) < MAX_LINE_LEN - 1) strcat(line, doc.clipboard[0]); } else { memmove(&doc.lines[doc.cursor_y + i + 1], &doc.lines[doc.cursor_y + i], (doc.num_lines - doc.cursor_y - i) * sizeof(char *)); doc.lines[doc.cursor_y + i] = (char *)malloc(MAX_LINE_LEN); strcpy(doc.lines[doc.cursor_y + i], doc.clipboard[i]); doc.num_lines++; } } doc.cursor_y += doc.clipboard_lines - 1; doc.cursor_x = strlen(doc.clipboard[doc.clipboard_lines - 1]); doc.modified = 1; }
    doc.block_active = 0;  /* Clear block selection after paste */
    draw_screen();
}

void delete_block(void)
{
    int i, by1, by2, bx1, bx2; char *first_line, *last_line;
    if (!doc.block_active) { show_dialog("Block", "No block selected!"); return; }
    
    /* Check if block has any content */
    if (doc.block_start_x == doc.block_end_x && doc.block_start_y == doc.block_end_y) {
        show_dialog("Block", "No text selected!");
        doc.block_active = 0;
        return;
    }
    
    by1 = doc.block_start_y < doc.block_end_y ? doc.block_start_y : doc.block_end_y;
    by2 = doc.block_start_y > doc.block_end_y ? doc.block_start_y : doc.block_end_y;
    if (doc.block_start_y == doc.block_end_y) { bx1 = doc.block_start_x < doc.block_end_x ? doc.block_start_x : doc.block_end_x; bx2 = doc.block_start_x > doc.block_end_x ? doc.block_start_x : doc.block_end_x; }
    else if (doc.block_start_y < doc.block_end_y) { bx1 = doc.block_start_x; bx2 = doc.block_end_x; }
    else { bx1 = doc.block_end_x; bx2 = doc.block_start_x; }
    if (by1 == by2) { char *ln = doc.lines[by1]; int l = strlen(ln); memmove(ln + bx1, ln + bx2, l - bx2 + 1); }
    else { first_line = doc.lines[by1]; last_line = doc.lines[by2]; first_line[bx1] = '\0'; strcat(first_line, last_line + bx2); for (i = by1 + 1; i <= by2; i++) free(doc.lines[i]); memmove(&doc.lines[by1 + 1], &doc.lines[by2 + 1], (doc.num_lines - by2 - 1) * sizeof(char *)); doc.num_lines -= (by2 - by1); }
    doc.cursor_y = by1; doc.cursor_x = bx1; doc.block_active = 0; doc.modified = 1; draw_screen();
}

/* =========== Utility Functions =========== */

void word_count(void) { int i, j, words = 0, chars = 0, in_word = 0, pages; char msg[128], *line; for (i = 0; i < doc.num_lines; i++) { line = doc.lines[i]; if (!line) continue; for (j = 0; line[j]; j++) { chars++; if (line[j] == ' ' || line[j] == '\t') in_word = 0; else if (!in_word) { in_word = 1; words++; } } in_word = 0; } pages = (doc.num_lines + LINES_PER_PAGE - 1) / LINES_PER_PAGE; if (pages < 1) pages = 1; sprintf(msg, "Pages: %d  Lines: %d  Words: %d  Chars: %d", pages, doc.num_lines, words, chars); show_dialog("Word Count", msg); }

void goto_line(void) { char buf[16]; int line_num; buf[0] = '\0'; if (input_dialog("Go To Line", "Line number:", buf, sizeof(buf))) { line_num = atoi(buf); if (line_num < 1) line_num = 1; if (line_num > doc.num_lines) line_num = doc.num_lines; doc.cursor_y = line_num - 1; doc.cursor_x = 0; doc.scroll_y = doc.cursor_y - EDIT_HEIGHT / 2; if (doc.scroll_y < 0) doc.scroll_y = 0; draw_screen(); } }

void insert_date_time(void) { time_t now; struct tm *t; char buf[32]; int i; now = time(NULL); t = localtime(&now); sprintf(buf, "%02d/%02d/%04d %02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min); for (i = 0; buf[i]; i++) insert_char(buf[i]); draw_screen(); }

void show_help(void)
{
    int x1 = 2, y1 = 1, x2 = 77, y2 = 23;
    int page = 1, max_pages = 4;
    int key;
    
    while (1) {
        draw_box(x1, y1, x2, y2, CLR_DIALOG, " OpenQT Help ");
        
        if (page == 1) {
            write_string(x1 + 2, y1 + 2, "FILE OPERATIONS:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 3, "  Ctrl+N  - New document          F2      - Save file", CLR_DIALOG);
            write_string(x1 + 2, y1 + 4, "  F3      - Open file             Ctrl+S  - Save file", CLR_DIALOG);
            write_string(x1 + 2, y1 + 5, "  Alt+X   - Exit program", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 7, "NAVIGATION:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 8, "  Arrows      - Move cursor       Home/End   - Start/End of line", CLR_DIALOG);
            write_string(x1 + 2, y1 + 9, "  PgUp/PgDn   - Page up/down       Ctrl+Home  - Start of document", CLR_DIALOG);
            write_string(x1 + 2, y1 + 10, "  Ctrl+End    - End of document    Ctrl+G     - Go to line", CLR_DIALOG);
            write_string(x1 + 2, y1 + 11, "  Ctrl+Left   - 5 chars left       Ctrl+Right - 5 chars right", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 13, "EDITING:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 14, "  Insert  - Toggle Insert/Overwrite mode", CLR_DIALOG);
            write_string(x1 + 2, y1 + 15, "  Delete  - Delete character       Backspace  - Delete back", CLR_DIALOG);
            write_string(x1 + 2, y1 + 16, "  Enter   - New line               Tab        - Insert tab", CLR_DIALOG);
            write_string(x1 + 2, y1 + 17, "  Ctrl+Z  - Undo (250 levels)      Ctrl+Y     - Redo", CLR_DIALOG);
            
            write_string(x1 + 25, y2 - 1, " Page 1/4 - Press N for Next ", CLR_DIALOG_BTN);
        }
        else if (page == 2) {
            write_string(x1 + 2, y1 + 2, "TEXT FORMATTING:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 3, "  Ctrl+B  - Bold text              Alt+L   - Underline text", CLR_DIALOG);
            write_string(x1 + 2, y1 + 4, "  Ctrl+U  - Underline text         Alt+U   - Bold + Underline", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 6, "BLOCK OPERATIONS:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 7, "  F6 (1st)  - Start block selection", CLR_DIALOG);
            write_string(x1 + 2, y1 + 8, "  F6 (2nd)  - Copy block to clipboard", CLR_DIALOG);
            write_string(x1 + 2, y1 + 9, "  F9        - Paste block from clipboard", CLR_DIALOG);
            write_string(x1 + 2, y1 + 10, "  ESC       - Cancel block selection", CLR_DIALOG);
            write_string(x1 + 2, y1 + 11, "  Block menu: Alt+B (Copy/Move/Delete)", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 13, "SEARCH & REPLACE:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 14, "  F7      - Find text (F4 cycles ENG/HEB/ARA/RUS)", CLR_DIALOG);
            write_string(x1 + 2, y1 + 15, "  F8      - Replace text", CLR_DIALOG);
            write_string(x1 + 2, y1 + 16, "  Search menu: Alt+S (Find Next/Prev, Replace All)", CLR_DIALOG);
            
            write_string(x1 + 25, y2 - 1, " Page 2/4 - P=Prev N=Next ", CLR_DIALOG_BTN);
        }
        else if (page == 3) {
            write_string(x1 + 2, y1 + 2, "LANGUAGES & BIDI:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 3, "  F4   - Cycle keyboard: English -> Hebrew -> Arabic -> Russian", CLR_DIALOG);
            write_string(x1 + 2, y1 + 4, "  F5   - Toggle RTL/LTR paragraph direction", CLR_DIALOG);
            write_string(x1 + 2, y1 + 5, "  F10  - Embedded English (type English in RTL mode)", CLR_DIALOG);
            write_string(x1 + 2, y1 + 6, "  Numbers always display correctly (123, not 321)", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 8, "OPTIONS (Alt+O menu):", CLR_DIALOG);
            write_string(x1 + 2, y1 + 9, "  Word Wrap      - Auto wrap long lines", CLR_DIALOG);
            write_string(x1 + 2, y1 + 10, "  Show Ruler     - Display column ruler", CLR_DIALOG);
            write_string(x1 + 2, y1 + 11, "  Save Reminder  - Remind to save every 10 minutes", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 13, "TOOLS (Alt+T menu):", CLR_DIALOG);
            write_string(x1 + 2, y1 + 14, "  Word Count     - Count pages/lines/words/chars", CLR_DIALOG);
            write_string(x1 + 2, y1 + 15, "  Go To Line     - Jump to specific line", CLR_DIALOG);
            write_string(x1 + 2, y1 + 16, "  Insert Date    - Insert current date/time", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 18, "MENUS: Alt+F=File Alt+E=Edit Alt+S=Search Alt+B=Block", CLR_DIALOG);
            write_string(x1 + 2, y1 + 19, "       Alt+O=Options Alt+T=Tools Alt+H=Help", CLR_DIALOG);
            
            write_string(x1 + 25, y2 - 1, " Page 3/4 - P=Prev N=Next ", CLR_DIALOG_BTN);
        }
        else if (page == 4) {
            write_string(x1 + 2, y1 + 2, "PASSWORD PROTECTION (File menu):", CLR_DIALOG);
            write_string(x1 + 2, y1 + 4, "  Save Encrypted   - Save with password (extension .OQE)", CLR_DIALOG);
            write_string(x1 + 2, y1 + 5, "  Remove Password  - Save without encryption (.TXT)", CLR_DIALOG);
            write_string(x1 + 2, y1 + 7, "  - Password: English letters/numbers only, min 4 chars", CLR_DIALOG);
            write_string(x1 + 2, y1 + 8, "  - Encrypted files use .OQE extension", CLR_DIALOG);
            write_string(x1 + 2, y1 + 9, "  - 'ENC' shown in status bar when encrypted", CLR_DIALOG);
            write_string(x1 + 2, y1 + 10, "  - WARNING: If you forget password, file cannot be recovered!", CLR_DIALOG);
            
            write_string(x1 + 2, y1 + 12, "STATUS BAR INDICATORS:", CLR_DIALOG);
            write_string(x1 + 2, y1 + 13, "  INS/OVR - Insert/Overwrite mode", CLR_DIALOG);
            write_string(x1 + 2, y1 + 14, "  RTL/LTR - Text direction", CLR_DIALOG);
            write_string(x1 + 2, y1 + 15, "  ENG/HEB/ARA - Keyboard language", CLR_DIALOG);
            write_string(x1 + 2, y1 + 16, "  +E      - Embedded English mode active", CLR_DIALOG);
            write_string(x1 + 2, y1 + 17, "  BLK     - Block selection active", CLR_DIALOG);
            write_string(x1 + 2, y1 + 18, "  ENC     - Document is encrypted", CLR_DIALOG);
            
            write_string(x1 + 25, y2 - 1, " Page 4/4 - P=Prev ESC=Close ", CLR_DIALOG_BTN);
        }
        
        hide_cursor();
        key = getch();
        if (key == 0 || key == 0xE0) key = getch();
        
        if (key == 'n' || key == 'N' || key == ' ') {
            page++;
            if (page > max_pages) page = 1;
        }
        else if (key == 'p' || key == 'P' || key == 8) {  /* 8 = Backspace */
            page--;
            if (page < 1) page = max_pages;
        }
        else if (key == 27) {  /* ESC */
            break;
        }
    }
    draw_screen();
}

void show_about(void)
{
    int x1 = 15, y1 = 6, x2 = 64, y2 = 18;
    draw_box(x1, y1, x2, y2, CLR_DIALOG, " About OpenQT ");
    write_string(x1 + 5, y1 + 2, "OpenQT - Heb/Eng/Arab Word Processor", CLR_DIALOG);
    write_string(x1 + 15, y1 + 3, "Version " VERSION, CLR_DIALOG);
    write_string(x1 + 12, y1 + 5, "by Ronen Blumberg", CLR_DIALOG);
    write_string(x1 + 8, y1 + 7, "A QText 5.5 Compatible Clone", CLR_DIALOG);
    write_string(x1 + 4, y1 + 8, "Hebrew + English + Arabic, Full BiDi", CLR_DIALOG);
    write_string(x1 + 12, y1 + 10, "Open Source / Public Domain", CLR_DIALOG);
    write_string(x1 + 17, y2 - 1, " OK ", CLR_DIALOG_BTN);
    hide_cursor(); getch(); draw_screen();
}

/* Trilingual Docs viewer.
 * Loads OPENQT.HLP from current directory and presents a paged
 * read-only view with BiDi reordering + Arabic shaping per line.
 * The .HLP file is plain text in OpenQT's mixed CP862+CP864 byte
 * encoding (Hebrew at 0x80-0x9A, Arabic at 0xA0-0xFE), so it
 * displays natively when HEBVGA + ARABVGA fonts are loaded. */
void show_docs(void)
{
    static char doc_lines[200][256];
    static int  line_lens[200];
    FILE *fp;
    char tmp[300];
    int num_lines = 0, line_offset = 0;
    int len, key, i, j, idx;
    int x1 = 1, y1 = 1, x2 = 78, y2 = 24;
    int visible_rows = y2 - y1 - 2;
    int row_width    = x2 - x1 - 2;
    int saved_rtl;

    /* Pick the .HLP that matches the currently-loaded VGA font, since
     * a single file can't render Hebrew, Arabic, and Russian under
     * one font (byte ranges overlap). Each launcher's session font
     * has a matching .HLP; OPENQT.HLP is the trilingual fallback used
     * by OQT 3 and by English/default mode.
     *
     * Try the language-specific file in cwd, parent, and the usual
     * well-known DOS locations BEFORE falling back to OPENQT.HLP -
     * otherwise an OQTH/OQTA session launched from C:\ would never
     * find OPENQTH.HLP / OPENQTA.HLP in C:\OPENQT\ and would
     * incorrectly fall through to the trilingual file. */
    {
        const char *primary = "OPENQT.HLP";
        char path[64];
        switch (doc.input_lang) {
            case LANG_HEB: primary = "OPENQTH.HLP"; break;
            case LANG_ARA: primary = "OPENQTA.HLP"; break;
            case LANG_RUS: primary = "OPENQTR.HLP"; break;
            default: break;     /* LANG_ENG -> trilingual OPENQT.HLP */
        }
        fp = fopen(primary, "rb");
        if (!fp) {
            sprintf(path, "..\\%s", primary);
            fp = fopen(path, "rb");
        }
        if (!fp) {
            sprintf(path, "\\OPENQT\\%s", primary);
            fp = fopen(path, "rb");
        }
        if (!fp) {
            sprintf(path, "C:\\OPENQT\\%s", primary);
            fp = fopen(path, "rb");
        }
        /* Last resort: trilingual OPENQT.HLP at any of those paths. */
        if (!fp) fp = fopen("OPENQT.HLP", "rb");
        if (!fp) fp = fopen("..\\OPENQT.HLP", "rb");
        if (!fp) fp = fopen("\\OPENQT\\OPENQT.HLP", "rb");
        if (!fp) fp = fopen("C:\\OPENQT\\OPENQT.HLP", "rb");
    }
    if (!fp) {
        show_dialog("Docs",
            "OPENQT*.HLP not found. Tried language-specific name, "
            "OPENQT.HLP, parent, and \\OPENQT\\. In DOSBox try "
            "`rescan` to refresh the cache.");
        return;
    }
    while (num_lines < 200 && fgets(tmp, sizeof(tmp), fp)) {
        len = strlen(tmp);
        while (len > 0 && (tmp[len-1] == '\n' || tmp[len-1] == '\r')) len--;
        if (len > 255) len = 255;
        memcpy(doc_lines[num_lines], tmp, len);
        doc_lines[num_lines][len] = '\0';
        line_lens[num_lines] = len;
        num_lines++;
    }
    fclose(fp);
    if (num_lines == 0) {
        show_dialog("Docs", "OPENQT.HLP is empty.");
        return;
    }

    /* Force RTL paragraph direction for proper Hebrew/Arabic layout. */
    saved_rtl = doc.rtl_mode;
    doc.rtl_mode = 1;

    while (1) {
        const char *title;
        switch (doc.input_lang) {
            case LANG_HEB: title = " OpenQT Docs - Hebrew + English ";  break;
            case LANG_ARA: title = " OpenQT Docs - Arabic + English ";  break;
            case LANG_RUS: title = " OpenQT Docs - Russian + English "; break;
            default:       title = " OpenQT Docs - Trilingual Guide ";  break;
        }
        draw_box(x1, y1, x2, y2, CLR_DIALOG, title);

        for (i = 0; i < visible_rows; i++) {
            int row = y1 + 1 + i;
            int show_len;

            for (j = 0; j < row_width; j++) {
                video_mem[(row * SCREEN_WIDTH + (x1 + 1 + j)) * 2] = ' ';
                video_mem[(row * SCREEN_WIDTH + (x1 + 1 + j)) * 2 + 1] = CLR_DIALOG;
            }

            idx = line_offset + i;
            if (idx >= num_lines) continue;
            if (line_lens[idx] == 0) continue;

            bidi_reorder(doc_lines[idx], line_lens[idx]);
            show_len = bidi.len < row_width ? bidi.len : row_width;
            for (j = 0; j < show_len; j++) {
                video_mem[(row * SCREEN_WIDTH + (x1 + 1 + j)) * 2]     = bidi.visual[j];
                video_mem[(row * SCREEN_WIDTH + (x1 + 1 + j)) * 2 + 1] = CLR_DIALOG;
            }
        }

        sprintf(tmp, " Lines %d-%d / %d   ESC=close  Up/Dn  PgUp/PgDn  Home/End ",
                line_offset + 1,
                (line_offset + visible_rows < num_lines)
                    ? line_offset + visible_rows : num_lines,
                num_lines);
        write_string(x1 + 2, y2 - 1, tmp, CLR_DIALOG_BTN);

        hide_cursor();
        key = getch();
        if (key == 0 || key == 0xE0) {
            key = getch();
            if (key == KEY_DOWN) {
                if (line_offset + visible_rows < num_lines) line_offset++;
            } else if (key == KEY_UP) {
                if (line_offset > 0) line_offset--;
            } else if (key == KEY_PGDN) {
                line_offset += visible_rows;
                if (line_offset > num_lines - visible_rows)
                    line_offset = num_lines - visible_rows;
                if (line_offset < 0) line_offset = 0;
            } else if (key == KEY_PGUP) {
                line_offset -= visible_rows;
                if (line_offset < 0) line_offset = 0;
            } else if (key == KEY_HOME) {
                line_offset = 0;
            } else if (key == KEY_END) {
                line_offset = num_lines - visible_rows;
                if (line_offset < 0) line_offset = 0;
            }
        } else if (key == KEY_ESC || key == 'q' || key == 'Q') {
            break;
        }
    }

    doc.rtl_mode = saved_rtl;
    draw_screen();
}

void print_document(void) { FILE *prn; int i; if (!confirm_dialog("Print", "Send to printer?")) return; prn = fopen("PRN", "w"); if (!prn) { show_dialog("Error", "Cannot access printer!"); return; } for (i = 0; i < doc.num_lines; i++) { if (doc.lines[i]) fputs(doc.lines[i], prn); fputs("\r\n", prn); } fputc(12, prn); fclose(prn); show_dialog("Print", "Document sent"); }
void push_undo(int type, int line, int col, unsigned char ch, const char *saved)
{
    UndoItem *item;
    int i;
    
    /* Clear redo stack when new action is made */
    for (i = undo_pos; i < undo_pos + redo_count && i < MAX_UNDO; i++) {
        if (undo_stack[i % MAX_UNDO].saved_line) {
            free(undo_stack[i % MAX_UNDO].saved_line);
            undo_stack[i % MAX_UNDO].saved_line = NULL;
        }
    }
    redo_count = 0;
    
    item = &undo_stack[undo_pos % MAX_UNDO];
    
    /* Free old saved line if exists */
    if (item->saved_line) {
        free(item->saved_line);
        item->saved_line = NULL;
    }
    
    item->type = type;
    item->line = line;
    item->col = col;
    item->ch = ch;
    
    if (saved) {
        item->saved_line = (char *)malloc(strlen(saved) + 1);
        if (item->saved_line) strcpy(item->saved_line, saved);
    } else {
        item->saved_line = NULL;
    }
    
    undo_pos = (undo_pos + 1) % MAX_UNDO;
    if (undo_count < MAX_UNDO) undo_count++;
}

void undo_action(void)
{
    UndoItem *item;
    char *line;
    int len, i;
    
    if (undo_count == 0) {
        show_dialog("Undo", "Nothing to undo");
        return;
    }
    
    undo_pos = (undo_pos - 1 + MAX_UNDO) % MAX_UNDO;
    undo_count--;
    redo_count++;
    
    item = &undo_stack[undo_pos];
    
    switch (item->type) {
        case 0: /* Undo insert_char - delete the char */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                if (item->col < len) {
                    memmove(line + item->col, line + item->col + 1, len - item->col);
                }
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col;
            break;
            
        case 1: /* Undo delete_char - insert the char back */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                memmove(line + item->col + 1, line + item->col, len - item->col + 1);
                line[item->col] = item->ch;
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col;
            break;
            
        case 2: /* Undo insert_line - delete the line */
            if (item->line < doc.num_lines) {
                if (doc.lines[item->line]) free(doc.lines[item->line]);
                for (i = item->line; i < doc.num_lines - 1; i++)
                    doc.lines[i] = doc.lines[i + 1];
                doc.num_lines--;
                if (doc.num_lines == 0) {
                    doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
                    doc.lines[0][0] = '\0';
                    doc.num_lines = 1;
                }
            }
            doc.cursor_y = item->line > 0 ? item->line - 1 : 0;
            doc.cursor_x = item->col;
            break;
            
        case 3: /* Undo delete_line - restore the line */
            if (item->saved_line && doc.num_lines < MAX_LINES) {
                for (i = doc.num_lines; i > item->line; i--)
                    doc.lines[i] = doc.lines[i - 1];
                doc.lines[item->line] = (char *)malloc(MAX_LINE_LEN);
                if (doc.lines[item->line]) {
                    strcpy(doc.lines[item->line], item->saved_line);
                }
                doc.num_lines++;
            }
            doc.cursor_y = item->line;
            doc.cursor_x = 0;
            break;
            
        case 4: /* Undo backspace - insert char back */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                memmove(line + item->col + 1, line + item->col, len - item->col + 1);
                line[item->col] = item->ch;
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col + 1;
            break;
    }
    
    draw_screen();
}

void redo_action(void)
{
    UndoItem *item;
    char *line;
    int len, i;
    
    if (redo_count == 0) {
        show_dialog("Redo", "Nothing to redo");
        return;
    }
    
    item = &undo_stack[undo_pos];
    
    switch (item->type) {
        case 0: /* Redo insert_char */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                memmove(line + item->col + 1, line + item->col, len - item->col + 1);
                line[item->col] = item->ch;
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col + 1;
            break;
            
        case 1: /* Redo delete_char */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                if (item->col < len) {
                    memmove(line + item->col, line + item->col + 1, len - item->col);
                }
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col;
            break;
            
        case 2: /* Redo insert_line */
            if (doc.num_lines < MAX_LINES) {
                for (i = doc.num_lines; i > item->line; i--)
                    doc.lines[i] = doc.lines[i - 1];
                doc.lines[item->line] = (char *)malloc(MAX_LINE_LEN);
                if (doc.lines[item->line]) doc.lines[item->line][0] = '\0';
                doc.num_lines++;
            }
            doc.cursor_y = item->line;
            doc.cursor_x = 0;
            break;
            
        case 3: /* Redo delete_line */
            if (item->line < doc.num_lines) {
                if (doc.lines[item->line]) free(doc.lines[item->line]);
                for (i = item->line; i < doc.num_lines - 1; i++)
                    doc.lines[i] = doc.lines[i + 1];
                doc.num_lines--;
                if (doc.num_lines == 0) {
                    doc.lines[0] = (char *)malloc(MAX_LINE_LEN);
                    doc.lines[0][0] = '\0';
                    doc.num_lines = 1;
                }
            }
            doc.cursor_y = item->line > 0 ? item->line - 1 : 0;
            doc.cursor_x = 0;
            break;
            
        case 4: /* Redo backspace */
            line = doc.lines[item->line];
            if (line) {
                len = strlen(line);
                if (item->col < len) {
                    memmove(line + item->col, line + item->col + 1, len - item->col);
                }
            }
            doc.cursor_y = item->line;
            doc.cursor_x = item->col;
            break;
    }
    
    undo_pos = (undo_pos + 1) % MAX_UNDO;
    undo_count++;
    redo_count--;
    
    draw_screen();
}

/* =========== File Dialog =========== */

#define MAX_DIR_ENTRIES 200
#define FILE_LIST_HEIGHT 12

int file_dialog(const char *title, char *filename, int maxlen, int save_mode)
{
    struct find_t finfo;
    char *entries[MAX_DIR_ENTRIES];
    int is_dir[MAX_DIR_ENTRIES];
    int entry_count = 0, sel = 0, top = 0, key, ext_key, i;
    char cwd[256], pattern[64];
    int x1 = 5, y1 = 3, w = 70, h = 18;
    int list_x, list_y, list_w, list_h, name_x, name_y, name_w;
    int filter_idx = 0;
    const char *filters[] = { "*.TXT", "*.DOC", "*.HEB", "*.*" };
    int done = 0, result = 0, in_name_field = save_mode ? 1 : 0, name_pos;
    char name_buf[256];
    
    getcwd(cwd, sizeof(cwd));
    strcpy(pattern, filters[filter_idx]);
    strcpy(name_buf, filename);
    name_pos = strlen(name_buf);
    list_x = x1 + 2; list_y = y1 + 3; list_w = w - 4; list_h = FILE_LIST_HEIGHT;
    name_x = x1 + 12; name_y = y1 + h - 3; name_w = w - 16;
    
    while (!done) {
        entry_count = 0;
        entries[entry_count] = (char *)malloc(16); strcpy(entries[entry_count], ".."); is_dir[entry_count] = 1; entry_count++;
        if (_dos_findfirst("*.*", _A_SUBDIR, &finfo) == 0) { do { if ((finfo.attrib & _A_SUBDIR) && finfo.name[0] != '.') { if (entry_count < MAX_DIR_ENTRIES) { entries[entry_count] = (char *)malloc(16); strncpy(entries[entry_count], finfo.name, 12); entries[entry_count][12] = '\0'; is_dir[entry_count] = 1; entry_count++; } } } while (_dos_findnext(&finfo) == 0); }
        if (_dos_findfirst(pattern, _A_NORMAL, &finfo) == 0) { do { if (!(finfo.attrib & _A_SUBDIR)) { if (entry_count < MAX_DIR_ENTRIES) { entries[entry_count] = (char *)malloc(16); strncpy(entries[entry_count], finfo.name, 12); entries[entry_count][12] = '\0'; is_dir[entry_count] = 0; entry_count++; } } } while (_dos_findnext(&finfo) == 0); }
        if (sel >= entry_count) sel = entry_count - 1; if (sel < 0) sel = 0;
        
        draw_box(x1, y1, x1 + w - 1, y1 + h - 1, CLR_DIALOG, title);
        write_string(x1 + 2, y1 + 1, "Directory: ", CLR_DIALOG);
        fill_rect(x1 + 13, y1 + 1, x1 + w - 3, y1 + 1, ' ', CLR_DIALOG);
        write_string(x1 + 13, y1 + 1, cwd, CLR_DIALOG);
        draw_box(list_x - 1, list_y - 1, list_x + list_w, list_y + list_h, CLR_MENU, NULL);
        
        for (i = 0; i < list_h; i++) { int idx = top + i; fill_rect(list_x, list_y + i, list_x + list_w - 1, list_y + i, ' ', (idx == sel && !in_name_field) ? CLR_MENU_SEL : CLR_MENU); if (idx < entry_count) { char display[20]; if (is_dir[idx]) sprintf(display, "[%s]", entries[idx]); else strcpy(display, entries[idx]); write_string(list_x + 1, list_y + i, display, (idx == sel && !in_name_field) ? CLR_MENU_SEL : CLR_MENU); } }
        
        write_string(x1 + 2, name_y, "Filename:", CLR_DIALOG);
        fill_rect(name_x, name_y, name_x + name_w - 1, name_y, ' ', in_name_field ? CLR_MENU_SEL : CLR_MENU);
        write_string(name_x, name_y, name_buf, in_name_field ? CLR_MENU_SEL : CLR_MENU);
        write_string(x1 + 2, name_y + 1, "Filter:", CLR_DIALOG);
        write_string(x1 + 10, name_y + 1, pattern, CLR_DIALOG);
        write_string(x1 + w - 22, name_y + 1, save_mode ? "[ Save ]" : "[ Open ]", CLR_DIALOG_BTN);
        write_string(x1 + w - 12, name_y + 1, "[ Cancel ]", CLR_DIALOG_BTN);
        
        if (in_name_field) { show_cursor(1); set_cursor_pos(name_x + name_pos, name_y); } else hide_cursor();
        
        key = getch();
        if (key == KEY_ESC) { done = 1; result = 0; }
        else if (key == KEY_ENTER) { if (in_name_field) { if (name_buf[0] != '\0') { strncpy(filename, name_buf, maxlen - 1); filename[maxlen - 1] = '\0'; done = 1; result = 1; } } else if (sel < entry_count) { if (is_dir[sel]) { chdir(entries[sel]); getcwd(cwd, sizeof(cwd)); sel = 0; top = 0; } else { strcpy(name_buf, entries[sel]); name_pos = strlen(name_buf); if (!save_mode) { strncpy(filename, name_buf, maxlen - 1); filename[maxlen - 1] = '\0'; done = 1; result = 1; } } } }
        else if (key == KEY_TAB) { if (in_name_field) { filter_idx = (filter_idx + 1) % 4; strcpy(pattern, filters[filter_idx]); sel = 0; top = 0; } else in_name_field = 1; }
        else if (key == KEY_BACKSPACE) { if (in_name_field && name_pos > 0) { memmove(name_buf + name_pos - 1, name_buf + name_pos, strlen(name_buf) - name_pos + 1); name_pos--; } }
        else if (key == 0 || key == 0xE0) { ext_key = getch(); if (!in_name_field) { if (ext_key == KEY_UP && sel > 0) { sel--; if (sel < top) top = sel; } if (ext_key == KEY_DOWN && sel < entry_count - 1) { sel++; if (sel >= top + list_h) top = sel - list_h + 1; } if (ext_key == KEY_PGUP) { sel -= list_h; if (sel < 0) sel = 0; top = sel; } if (ext_key == KEY_PGDN) { sel += list_h; if (sel >= entry_count) sel = entry_count - 1; if (sel >= top + list_h) top = sel - list_h + 1; } } else { if (ext_key == KEY_LEFT && name_pos > 0) name_pos--; if (ext_key == KEY_RIGHT && name_pos < (int)strlen(name_buf)) name_pos++; if (ext_key == KEY_UP || ext_key == KEY_DOWN) in_name_field = 0; } }
        else if (key >= 32 && key < 256) { if (in_name_field && (int)strlen(name_buf) < maxlen - 1) { memmove(name_buf + name_pos + 1, name_buf + name_pos, strlen(name_buf) - name_pos + 1); name_buf[name_pos] = (char)key; name_pos++; } }
        
        for (i = 0; i < entry_count; i++) free(entries[i]);
    }
    draw_screen();
    return result;
}

/* =========== Host Helper Bridge =========== */

/* Round-trip a request through the host daemon. Writes a request file the
 * daemon picks up and polls the response file it writes back. Returns the
 * length of the response payload (>=0) on success; on success 'resp' holds
 * the raw payload bytes (NUL-terminated). Negative returns:
 *   -1 cannot access bridge folder   -2 user cancelled (ESC)
 *   -3 timed out (daemon not running) -4 daemon returned ERR (msg in resp) */
int bridge_request(const char *cmd, const char *lang, const char *payload,
                   int plen, char *resp, int rmax, int timeout_sec)
{
    FILE *fp;
    time_t start;
    int n, ok_status, plen2;
    char *eof, *nl;

    /* 1. pre-create the response file so its directory entry is cached by
     *    DOSBox before the daemon overwrites the contents in place. */
    fp = fopen(BRIDGE_RESP, "wb");
    if (!fp) return -1;
    fputs("WAIT\n", fp);
    fclose(fp);

    /* 2. write the request to a temp name then rename, so the daemon never
     *    reads a half-written request. */
    fp = fopen(BRIDGE_REQ_TMP, "wb");
    if (!fp) return -1;
    fprintf(fp, "%s\n%s\n---\n", cmd, lang);
    if (payload && plen > 0) fwrite(payload, 1, plen, fp);
    fclose(fp);
    remove(BRIDGE_REQ);
    rename(BRIDGE_REQ_TMP, BRIDGE_REQ);

    /* 3. poll the response file for the .EOF. completion sentinel. */
    start = time(NULL);
    while (1) {
        if (kbhit()) {
            int k = getch();
            if (k == 0 || k == 0xE0) getch();
            else if (k == KEY_ESC) return -2;
        }
        fp = fopen(BRIDGE_RESP, "rb");
        if (fp) {
            n = fread(resp, 1, rmax - 1, fp);
            fclose(fp);
            if (n < 0) n = 0;
            resp[n] = '\0';
            eof = strstr(resp, "\n.EOF.");
            if (eof) {
                *eof = '\0';                 /* drop sentinel */
                nl = strchr(resp, '\n');     /* end of status line */
                if (!nl) return -1;
                ok_status = (strncmp(resp, "OK", 2) == 0);
                plen2 = (int)(eof - (nl + 1));
                memmove(resp, nl + 1, plen2 + 1);
                if (!ok_status) return -4;
                return plen2;
            }
        }
        if ((long)(time(NULL) - start) > timeout_sec) return -3;
        delay(150);
    }
}

const char *bridge_err(int code, const char *resp)
{
    if (code == -1) return "Bridge folder missing. Start host_helper/run_helper.sh.";
    if (code == -3) return "Helper not responding. Is the host daemon running?";
    if (code == -4) return resp;             /* ERR message from the daemon */
    return "Helper error.";
}

const char *cur_lang_hint(void)
{
    if (doc.input_lang == LANG_HEB) return "HE";
    if (doc.input_lang == LANG_ARA) return "AR";
    if (doc.input_lang == LANG_RUS) return "RU";
    return "EN";
}

/* Copy the active block (if any) else the whole document into 'buf' as raw
 * OpenQT bytes, lines separated by '\n'. Returns the byte count. */
int gather_doc_text(char *buf, int maxlen)
{
    int i, n = 0, len, by1, by2, bx1, bx2, s, e, j;
    char *src;
    if (doc.block_active &&
        !(doc.block_start_x == doc.block_end_x &&
          doc.block_start_y == doc.block_end_y)) {
        by1 = doc.block_start_y < doc.block_end_y ? doc.block_start_y : doc.block_end_y;
        by2 = doc.block_start_y > doc.block_end_y ? doc.block_start_y : doc.block_end_y;
        if (doc.block_start_y == doc.block_end_y) {
            bx1 = doc.block_start_x < doc.block_end_x ? doc.block_start_x : doc.block_end_x;
            bx2 = doc.block_start_x > doc.block_end_x ? doc.block_start_x : doc.block_end_x;
        } else if (doc.block_start_y < doc.block_end_y) {
            bx1 = doc.block_start_x; bx2 = doc.block_end_x;
        } else { bx1 = doc.block_end_x; bx2 = doc.block_start_x; }
        for (i = by1; i <= by2 && n < maxlen - 2; i++) {
            src = doc.lines[i]; if (!src) src = "";
            len = strlen(src);
            s = (i == by1) ? bx1 : 0;
            e = (i == by2) ? bx2 : len;
            if (e > len) e = len;
            while (s < e && n < maxlen - 2) buf[n++] = src[s++];
            if (i < by2) buf[n++] = '\n';
        }
    } else {
        for (i = 0; i < doc.num_lines && n < maxlen - 2; i++) {
            src = doc.lines[i]; if (!src) src = "";
            len = strlen(src);
            for (j = 0; j < len && n < maxlen - 2; j++) buf[n++] = src[j];
            buf[n++] = '\n';
        }
    }
    buf[n] = '\0';
    return n;
}

/* Non-blocking centered "working" notice (erased by the next draw_screen). */
void show_busy(const char *msg)
{
    int w = strlen(msg) + 6, x1, y1;
    if (w < 24) w = 24;
    if (w > 70) w = 70;
    x1 = (SCREEN_WIDTH - w) / 2;
    y1 = SCREEN_HEIGHT / 2 - 1;
    draw_box(x1, y1, x1 + w - 1, y1 + 2, CLR_DIALOG, "Helper");
    write_string(x1 + 3, y1 + 1, msg, CLR_DIALOG);
    hide_cursor();
}

void assist_read_aloud(void)
{
    char *payload, resp[512];
    int plen, r;
    payload = (char *)malloc(BRIDGE_BUF);
    if (!payload) { show_dialog("Read Aloud", "Out of memory."); return; }
    plen = gather_doc_text(payload, BRIDGE_BUF);
    if (plen <= 0) { free(payload); show_dialog("Read Aloud", "Nothing to read."); return; }
    show_busy("Speaking... (audio plays on host)");
    r = bridge_request("TTS", cur_lang_hint(), payload, plen, resp, sizeof(resp), 30);
    free(payload);
    if (r < 0 && r != -2) show_dialog("Read Aloud", bridge_err(r, resp));
    draw_screen();
}

void assist_stop_speech(void)
{
    char resp[256];
    bridge_request("TTSSTOP", "EN", NULL, 0, resp, sizeof(resp), 10);
    draw_screen();
}

void assist_translate(void)
{
    char *payload, *resp;
    int plen, r, i, save_ins;
    payload = (char *)malloc(BRIDGE_BUF);
    resp = (char *)malloc(BRIDGE_BUF);
    if (!payload || !resp) { free(payload); free(resp); show_dialog("Translate", "Out of memory."); return; }
    plen = gather_doc_text(payload, BRIDGE_BUF);
    if (plen <= 0) { free(payload); free(resp); show_dialog("Translate", "Nothing to translate."); return; }
    show_busy("Translating to Hebrew...");
    r = bridge_request("XLATE", "EN", payload, plen, resp, BRIDGE_BUF, 45);
    if (r < 0) {
        if (r != -2) show_dialog("Translate", bridge_err(r, resp));
        free(payload); free(resp); draw_screen(); return;
    }
    save_ins = doc.insert_mode; doc.insert_mode = 1;
    for (i = 0; i < r; i++) {
        if (resp[i] == '\n') insert_line();
        else insert_char((unsigned char)resp[i]);
    }
    doc.insert_mode = save_ins;
    free(payload); free(resp);
    draw_screen();
    show_dialog("Translate", "Hebrew translation inserted at cursor.");
}

/* --- interactive spell check --- */
typedef struct { int line; int col; char word[48]; char sugg[480]; } SpellFind;

static int spell_nth_sugg(const char *suggs, int n, char *out, int max)
{
    const char *p = suggs;
    int idx = 1, l;
    const char *c;
    while (*p) {
        c = strchr(p, ',');
        l = c ? (int)(c - p) : (int)strlen(p);
        if (idx == n) {
            if (l > max - 1) l = max - 1;
            memcpy(out, p, l); out[l] = '\0';
            return 1;
        }
        idx++;
        if (!c) break;
        p = c + 1;
    }
    return 0;
}

/* Returns: -1 stop, 0 skip, 1..9 chosen suggestion number. */
static int spell_prompt(const char *word, const char *suggs)
{
    char items[9][40], hdr[64], ln[48];
    int nsug = 0, i, key, x1, y1, w, h, l;
    const char *p = suggs, *c;
    while (*p && nsug < 9) {
        c = strchr(p, ',');
        l = c ? (int)(c - p) : (int)strlen(p);
        if (l > 38) l = 38;
        memcpy(items[nsug], p, l); items[nsug][l] = '\0';
        nsug++;
        if (!c) break;
        p = c + 1;
    }
    w = 44; h = nsug + 6;
    x1 = (SCREEN_WIDTH - w) / 2; y1 = 2;
    draw_box(x1, y1, x1 + w - 1, y1 + h - 1, CLR_DIALOG, "Spell Check");
    sprintf(hdr, "Not in dictionary: %.20s", word);
    write_string(x1 + 2, y1 + 1, hdr, CLR_DIALOG);
    for (i = 0; i < nsug; i++) {
        sprintf(ln, " %d. %s", i + 1, items[i]);
        write_string(x1 + 2, y1 + 3 + i, ln, CLR_DIALOG);
    }
    if (nsug == 0) write_string(x1 + 2, y1 + 3, " (no suggestions)", CLR_DIALOG);
    write_string(x1 + 2, y1 + h - 2, "1-9=replace  S=skip  Esc=stop", CLR_DIALOG_BTN);
    hide_cursor();
    while (1) {
        key = getch();
        if (key == 0 || key == 0xE0) { getch(); continue; }
        if (key == KEY_ESC) return -1;
        if (key == 'S' || key == 's' || key == KEY_ENTER) return 0;
        if (key >= '1' && key <= '9') { int nn = key - '0'; if (nn <= nsug) return nn; }
    }
}

/* Replace oldlen bytes at (ln,col) with newstr, using the cursor edit path so
 * the change is undoable. */
static void spell_replace(int ln, int col, int oldlen, const char *newstr)
{
    char *line = doc.lines[ln];
    int len, nlen, i, save;
    if (!line) return;
    len = strlen(line);
    if (col > len) return;
    if (col + oldlen > len) oldlen = len - col;
    nlen = strlen(newstr);
    if (len - oldlen + nlen >= MAX_LINE_LEN - 1) return;
    doc.cursor_y = ln; doc.cursor_x = col;
    save = doc.insert_mode; doc.insert_mode = 1;
    for (i = 0; i < oldlen; i++) delete_char();
    for (i = 0; i < nlen; i++) insert_char((unsigned char)newstr[i]);
    doc.insert_mode = save;
    doc.modified = 1;
}

void assist_spell_check(void)
{
    char *payload, *resp;
    SpellFind *finds;
    int plen, r, nf = 0, i, j, corrected = 0;
    char *p, *eol, *t2, *t3, *t4, repl[48], msg[80];
    SpellFind tmp;

    payload = (char *)malloc(BRIDGE_BUF);
    resp = (char *)malloc(BRIDGE_BUF);
    finds = (SpellFind *)malloc(sizeof(SpellFind) * ASSIST_MAX_FIND);
    if (!payload || !resp || !finds) {
        free(payload); free(resp); free(finds);
        show_dialog("Spell Check", "Out of memory."); return;
    }
    plen = gather_doc_text(payload, BRIDGE_BUF);
    if (plen <= 0) { free(payload); free(resp); free(finds); show_dialog("Spell Check", "Nothing to check."); return; }
    show_busy("Checking spelling...");
    r = bridge_request("SPELL", "EN", payload, plen, resp, BRIDGE_BUF, 30);
    if (r < 0) {
        if (r != -2) show_dialog("Spell Check", bridge_err(r, resp));
        free(payload); free(resp); free(finds); draw_screen(); return;
    }
    /* parse "SPELL N\n" header then N tab-delimited records */
    p = strchr(resp, '\n');
    if (p) p++;
    while (p && *p && nf < ASSIST_MAX_FIND) {
        eol = strchr(p, '\n');
        if (eol) *eol = '\0';
        t2 = strchr(p, '\t');
        if (t2) {
            *t2++ = '\0';
            t3 = strchr(t2, '\t');
            if (t3) {
                *t3++ = '\0';
                t4 = strchr(t3, '\t');
                if (t4) *t4++ = '\0'; else t4 = "";
                finds[nf].line = atoi(p);
                finds[nf].col = atoi(t2);
                strncpy(finds[nf].word, t3, sizeof(finds[nf].word) - 1);
                finds[nf].word[sizeof(finds[nf].word) - 1] = '\0';
                strncpy(finds[nf].sugg, t4, sizeof(finds[nf].sugg) - 1);
                finds[nf].sugg[sizeof(finds[nf].sugg) - 1] = '\0';
                nf++;
            }
        }
        if (!eol) break;
        p = eol + 1;
    }
    if (nf == 0) { free(payload); free(resp); free(finds); show_dialog("Spell Check", "No spelling errors found."); return; }
    /* sort by line ascending, column descending: same-line edits keep earlier
     * columns valid after later (rightward) replacements. */
    for (i = 0; i < nf - 1; i++)
        for (j = 0; j < nf - 1 - i; j++)
            if (finds[j].line > finds[j + 1].line ||
                (finds[j].line == finds[j + 1].line && finds[j].col < finds[j + 1].col)) {
                tmp = finds[j]; finds[j] = finds[j + 1]; finds[j + 1] = tmp;
            }
    for (i = 0; i < nf; i++) {
        int ln = finds[i].line, col = finds[i].col, wlen = strlen(finds[i].word), act;
        if (ln < 0 || ln >= doc.num_lines) continue;
        doc.cursor_y = ln; doc.cursor_x = col;
        if (doc.cursor_y < doc.scroll_y) doc.scroll_y = doc.cursor_y;
        if (doc.cursor_y >= doc.scroll_y + EDIT_HEIGHT) doc.scroll_y = doc.cursor_y - EDIT_HEIGHT + 1;
        doc.block_active = 1;
        doc.block_start_y = ln; doc.block_start_x = col;
        doc.block_end_y = ln; doc.block_end_x = col + wlen;
        draw_screen();
        act = spell_prompt(finds[i].word, finds[i].sugg);
        if (act == -1) break;
        if (act >= 1 && spell_nth_sugg(finds[i].sugg, act, repl, sizeof(repl))) {
            spell_replace(ln, col, wlen, repl);
            corrected++;
        }
    }
    doc.block_active = 0;
    draw_screen();
    sprintf(msg, "Spell check done: %d corrected of %d flagged.", corrected, nf);
    free(payload); free(resp); free(finds);
    show_dialog("Spell Check", msg);
}

void assist_dictate(void)
{
    char *resp;
    int r, key, i, save_ins, x1, y1, w;
    resp = (char *)malloc(BRIDGE_BUF);
    if (!resp) { show_dialog("Dictate", "Out of memory."); return; }
    show_busy("Starting microphone...");
    r = bridge_request("RECSTART", cur_lang_hint(), NULL, 0, resp, BRIDGE_BUF, 15);
    if (r < 0) { show_dialog("Dictate", bridge_err(r, resp)); free(resp); return; }
    /* wait for the user to stop or cancel recording */
    w = 50; x1 = (SCREEN_WIDTH - w) / 2; y1 = SCREEN_HEIGHT / 2 - 1;
    draw_box(x1, y1, x1 + w - 1, y1 + 3, CLR_DIALOG, "Dictation");
    write_string(x1 + 2, y1 + 1, "Recording... speak now.", CLR_DIALOG);
    write_string(x1 + 2, y1 + 2, "Enter = transcribe    Esc = cancel", CLR_DIALOG_BTN);
    hide_cursor();
    while (1) {
        key = getch();
        if (key == 0 || key == 0xE0) { getch(); continue; }
        if (key == KEY_ENTER) break;
        if (key == KEY_ESC) {
            bridge_request("RECCANCEL", "EN", NULL, 0, resp, BRIDGE_BUF, 15);
            free(resp); draw_screen(); return;
        }
    }
    show_busy("Transcribing speech...");
    r = bridge_request("RECSTOP", cur_lang_hint(), NULL, 0, resp, BRIDGE_BUF, 300);
    if (r < 0) {
        if (r != -2) show_dialog("Dictate", bridge_err(r, resp));
        free(resp); draw_screen(); return;
    }
    if (r == 0) { show_dialog("Dictate", "No speech recognized."); free(resp); return; }
    save_ins = doc.insert_mode; doc.insert_mode = 1;
    for (i = 0; i < r; i++) {
        if (resp[i] == '\n') insert_line();
        else insert_char((unsigned char)resp[i]);
    }
    doc.insert_mode = save_ins;
    free(resp);
    draw_screen();
}

/* --- Smart Paste from the host clipboard via the helper bridge --- */
/* Unlike DOSBox-X's Ctrl+F6 paste, the helper reads the host clipboard as full
 * UTF-8, strips niqqud / harakat and folds smart punctuation, and returns clean
 * codepage bytes for the current language. We insert via insert_char (not the
 * keyboard path), so ASCII like '.' is NOT remapped through the Hebrew layout. */
void assist_paste_clipboard(void)
{
    char *resp;
    int r, i, save_ins;
    resp = (char *)malloc(BRIDGE_BUF);
    if (!resp) { show_dialog("Paste", "Out of memory."); return; }
    show_busy("Reading host clipboard...");
    r = bridge_request("CLIP", cur_lang_hint(), NULL, 0, resp, BRIDGE_BUF, 15);
    if (r < 0) {
        if (r != -2) show_dialog("Paste", bridge_err(r, resp));
        free(resp); draw_screen(); return;
    }
    if (r == 0) { show_dialog("Paste", "Clipboard empty."); free(resp); draw_screen(); return; }
    save_ins = doc.insert_mode; doc.insert_mode = 1;
    for (i = 0; i < r; i++) {
        if (resp[i] == '\n') insert_line();
        else insert_char((unsigned char)resp[i]);
    }
    doc.insert_mode = save_ins;
    free(resp);
    draw_screen();
}

/* =========== Menu Functions =========== */

void show_file_menu(void) { int x1 = 0, y1 = 1, w = 22, h = 12, sel = 0, key, i; char fn[256]; const char *items[] = {"New","Open          F3","Save          F2","Save As...","Save Encrypted...","Remove Password...","Print","----------------------","Exit       Alt+X"}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 9; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel && items[i][0] != '-') ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { do { sel--; if (sel < 0) sel = 8; } while (items[sel][0] == '-'); } if (key == KEY_DOWN) { do { sel++; if (sel > 8) sel = 0; } while (items[sel][0] == '-'); } if (key == KEY_LEFT) { draw_screen(); show_help_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_edit_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: new_document(); break; case 1: fn[0] = '\0'; if (file_dialog("Open File", fn, sizeof(fn), 0)) load_file(fn); draw_screen(); break; case 2: if (doc.encrypted) save_file_encrypted(doc.filename, doc.password); else save_file(doc.filename); draw_screen(); break; case 3: save_file_as(); draw_screen(); break; case 4: save_with_password(); draw_screen(); break; case 5: remove_password(); draw_screen(); break; case 6: print_document(); break; case 8: running = 0; break; } return; } } draw_screen(); }

void show_edit_menu(void) { int x1 = 6, y1 = 1, w = 18, h = 10, sel = 0, key, i; const char *items[] = {"Undo      Ctrl+Z","Redo      Ctrl+Y","------------------","Cut","Copy","Paste","Delete Line","Select All"}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 8; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel && items[i][0] != '-') ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { do { sel--; if (sel < 0) sel = 7; } while (items[sel][0] == '-'); } if (key == KEY_DOWN) { do { sel++; if (sel > 7) sel = 0; } while (items[sel][0] == '-'); } if (key == KEY_LEFT) { draw_screen(); show_file_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_search_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: undo_action(); break; case 1: redo_action(); break; case 3: cut_block(); break; case 4: copy_block(); break; case 5: paste_block(); break; case 6: delete_line(); draw_screen(); break; case 7: doc.block_active = 1; doc.block_start_x = 0; doc.block_start_y = 0; doc.block_end_y = doc.num_lines - 1; doc.block_end_x = strlen(doc.lines[doc.num_lines - 1]); draw_screen(); break; } return; } } draw_screen(); }

void show_search_menu(void) { int x1 = 12, y1 = 1, w = 20, h = 8, sel = 0, key, i; const char *items[] = {"Find         F7","Find Next","Find Previous","Replace      F8","Replace All","Go to Line"}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 6; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel) ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { sel--; if (sel < 0) sel = 5; } if (key == KEY_DOWN) { sel++; if (sel > 5) sel = 0; } if (key == KEY_LEFT) { draw_screen(); show_edit_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_block_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: do_search(); break; case 1: find_next(); break; case 2: find_prev(); break; case 3: do_replace(); break; case 4: do_replace_all(); break; case 5: goto_line(); break; } return; } } draw_screen(); }

void show_block_menu(void) { int x1 = 21, y1 = 1, w = 18, h = 8, sel = 0, key, i; const char *items[] = {"Start Block F6","End Block","Copy Block","Move Block","Delete Block","Unmark Block"}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 6; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel) ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { sel--; if (sel < 0) sel = 5; } if (key == KEY_DOWN) { sel++; if (sel > 5) sel = 0; } if (key == KEY_LEFT) { draw_screen(); show_search_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_options_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: start_block(); break; case 1: end_block(); break; case 2: copy_block(); break; case 3: cut_block(); paste_block(); break; case 4: delete_block(); break; case 5: unselect_block(); break; } return; } } draw_screen(); }

void show_options_menu(void) { int x1 = 29, y1 = 1, w = 22, h = 10, sel = 0, key, i; char items[8][25]; const char *langname; while (1) { langname = (doc.input_lang == LANG_HEB) ? "Hebrew" : (doc.input_lang == LANG_ARA) ? "Arabic" : (doc.input_lang == LANG_RUS) ? "Russian" : "English"; sprintf(items[0], "Lang: %-7s    F4", langname); sprintf(items[1], "[%c] Embed English F10", doc.embedded_ltr ? 'X' : ' '); sprintf(items[2], "[%c] RTL Mode       F5", doc.rtl_mode ? 'X' : ' '); sprintf(items[3], "[%c] Word Wrap", doc.word_wrap ? 'X' : ' '); sprintf(items[4], "[%c] Show Ruler", doc.show_ruler ? 'X' : ' '); sprintf(items[5], "[%c] Insert Mode   Ins", doc.insert_mode ? 'X' : ' '); sprintf(items[6], "[%c] Save Reminder 10m", doc.save_reminder ? 'X' : ' '); strcpy(items[7], "----------------------"); draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 8; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel && items[i][0] != '-') ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { do { sel--; if (sel < 0) sel = 6; } while (sel == 7); } if (key == KEY_DOWN) { do { sel++; if (sel > 6) sel = 0; } while (sel == 7); } if (key == KEY_LEFT) { draw_screen(); show_block_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_tools_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER || key == ' ') { switch (sel) { case 0: cycle_input_lang(); break; case 1: toggle_embedded_ltr(); break; case 2: toggle_rtl(); break; case 3: toggle_wrap(); break; case 4: doc.show_ruler = !doc.show_ruler; draw_screen(); break; case 5: toggle_insert(); break; case 6: toggle_save_reminder(); break; } } } draw_screen(); }

void show_tools_menu(void) { int x1 = 37, y1 = 1, w = 24, h = 12, sel = 0, key, i; const char *items[] = {"Word Count","Go To Line","Insert Date/Time","----------------------","Spell Check (Eng)","Read Aloud","Stop Speech","Translate to Hebrew","Dictate (Speech)","Paste Host   Alt+V"}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 10; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel && items[i][0] != '-') ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { do { sel--; if (sel < 0) sel = 9; } while (items[sel][0] == '-'); } if (key == KEY_DOWN) { do { sel++; if (sel > 9) sel = 0; } while (items[sel][0] == '-'); } if (key == KEY_LEFT) { draw_screen(); show_options_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_help_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: word_count(); break; case 1: goto_line(); break; case 2: insert_date_time(); break; case 4: assist_spell_check(); break; case 5: assist_read_aloud(); break; case 6: assist_stop_speech(); break; case 7: assist_translate(); break; case 8: assist_dictate(); break; case 9: assist_paste_clipboard(); break; } return; } } draw_screen(); }

void show_help_menu(void) { int x1 = 44, y1 = 1, w = 14, h = 5, sel = 0, key, i; const char *items[] = {"Help     F1","Docs...","About..."}; while (1) { draw_box(x1, y1, x1 + w, y1 + h, CLR_MENU, NULL); for (i = 0; i < 3; i++) write_string(x1 + 1, y1 + 1 + i, items[i], (i == sel) ? CLR_MENU_SEL : CLR_MENU); hide_cursor(); key = getch(); if (key == 0 || key == 0xE0) { key = getch(); if (key == KEY_UP) { sel--; if (sel < 0) sel = 2; } if (key == KEY_DOWN) { sel++; if (sel > 2) sel = 0; } if (key == KEY_LEFT) { draw_screen(); show_tools_menu(); return; } if (key == KEY_RIGHT) { draw_screen(); show_file_menu(); return; } } else if (key == KEY_ESC) break; else if (key == KEY_ENTER) { draw_screen(); switch (sel) { case 0: show_help(); break; case 1: show_docs(); break; case 2: show_about(); break; } return; } } draw_screen(); }

/* =========== Main Key Processing =========== */

void process_key(int key, int extended)
{
    unsigned char ch;
    char fn[256];
    int spaces;
    
    if (extended) {
        switch (key & 0xFF) {
            case KEY_UP: move_cursor(0, -1); break;
            case KEY_DOWN: move_cursor(0, 1); break;
            case KEY_LEFT: move_cursor(-1, 0); break;
            case KEY_RIGHT: move_cursor(1, 0); break;
            case KEY_HOME: home_key(); break;
            case KEY_END: end_key(); break;
            case KEY_PGUP: page_up(); break;
            case KEY_PGDN: page_down(); break;
            case KEY_INSERT: toggle_insert(); break;
            case KEY_DELETE: delete_char(); break;
            case KEY_CTRL_HOME: ctrl_home(); break;
            case KEY_CTRL_END: ctrl_end(); break;
            case KEY_CTRL_LEFT: move_cursor(-5, 0); break;
            case KEY_CTRL_RIGHT: move_cursor(5, 0); break;
            case KEY_F1: show_help(); break;
            case KEY_F2: if (doc.encrypted) save_file_encrypted(doc.filename, doc.password); else save_file(doc.filename); draw_screen(); break;
            case KEY_F3: fn[0] = '\0'; if (file_dialog("Open File", fn, sizeof(fn), 0)) { load_file(fn); draw_screen(); } break;
            case KEY_F4: cycle_input_lang(); draw_screen(); break;
            case KEY_F5: toggle_rtl(); break;
            case KEY_F6: 
                if (!doc.block_active) {
                    start_block();
                } else {
                    copy_block();
                }
                break;
            case KEY_F7: do_search(); break;
            case KEY_F8: do_replace(); break;
            case KEY_F9: paste_block(); break;
            case KEY_F10: toggle_embedded_ltr(); draw_screen(); break;
            case KEY_ALT_F: show_file_menu(); break;
            case KEY_ALT_E: show_edit_menu(); break;
            case KEY_ALT_S: show_search_menu(); break;
            case KEY_ALT_B: show_block_menu(); break;
            case KEY_ALT_O: show_options_menu(); break;
            case KEY_ALT_T: show_tools_menu(); break;
            case KEY_ALT_H: show_help_menu(); break;
            case KEY_ALT_X: running = 0; break;
            case KEY_ALT_L: toggle_underline(); break;
            case KEY_ALT_U: toggle_boldunder(); break;
            case KEY_ALT_V: assist_paste_clipboard(); break;
        }
    } else {
        switch (key) {
            case KEY_ESC: unselect_block(); break;
            case KEY_ENTER: insert_line(); draw_screen(); break;
            case KEY_BACKSPACE: backspace_char(); break;
            case KEY_TAB:
                spaces = TAB_SIZE - (doc.cursor_x % TAB_SIZE);
                while (spaces--) insert_char(' ');
                {
                    int screen_row = EDIT_TOP + (doc.cursor_y - doc.scroll_y);
                    if (screen_row >= EDIT_TOP && screen_row < EDIT_TOP + EDIT_HEIGHT) {
                        draw_line(screen_row, doc.cursor_y);
                    }
                }
                break;
            case KEY_CTRL_G: goto_line(); break;
            case KEY_CTRL_N: new_document(); break;
            case KEY_CTRL_S: if (doc.encrypted) save_file_encrypted(doc.filename, doc.password); else save_file(doc.filename); draw_status_bar(); break;
            case KEY_CTRL_Z: undo_action(); break;
            case KEY_CTRL_Y: redo_action(); break;
            case KEY_CTRL_B: toggle_bold(); break;
            case KEY_CTRL_U: toggle_underline(); break;
            default:
                if (key >= 32 && key < 256) {
                    ch = (unsigned char)key;
                    
                    /* BiDi character input:
                     * - Hebrew mode + embedded_ltr OFF: translate to Hebrew
                     * - Arabic mode + embedded_ltr OFF: translate to Arabic
                     * - English mode, or embedded_ltr ON (F10): type as-is
                     * Numbers are always typed as-is.
                     */
                    if (!doc.embedded_ltr && ch >= 32 && ch < 128 &&
                        !(ch >= '0' && ch <= '9')) {
                        if (doc.input_lang == LANG_HEB) {
                            unsigned char m = translate_hebrew(ch);
                            if (m) ch = m;
                        } else if (doc.input_lang == LANG_ARA) {
                            unsigned char m = translate_arabic(ch);
                            if (m) ch = m;
                        } else if (doc.input_lang == LANG_RUS) {
                            unsigned char m = translate_russian(ch);
                            if (m) ch = m;
                        }
                    }

                    insert_char(ch);
                    /* Only redraw current line instead of full screen */
                    {
                        int screen_row = EDIT_TOP + (doc.cursor_y - doc.scroll_y);
                        if (screen_row >= EDIT_TOP && screen_row < EDIT_TOP + EDIT_HEIGHT) {
                            draw_line(screen_row, doc.cursor_y);
                        }
                    }
                }
                break;
        }
    }
}

/* =========== Main Program =========== */

void init_editor(void)
{
    int i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i += 2) {
        video_mem[i] = ' ';
        video_mem[i + 1] = CLR_NORMAL;
    }
    init_hebrew_mapping();
    init_arabic_shaping();
    init_document();
}

void cleanup_editor(void)
{
    int i;
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i += 2) {
        video_mem[i] = ' ';
        video_mem[i + 1] = 0x07;
    }
    free_document();
    show_cursor(1);
    set_cursor_pos(0, 0);
}

int main(int argc, char *argv[])
{
    int key, i;
    time_t now;
    char *file_arg = NULL;

    /* Argument parse: /A or -A switches to Arabic mode + ASCII boxes
     * (used by OQTA.BAT after running ARABVGA). /R or -R starts in
     * Russian mode (used by OQTR.BAT after running RUSVGA - CP866 keeps
     * CP437 box-drawing intact, so no ASCII fallback is needed). The
     * first non-flag argument is treated as the file to open. */
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] == '/' || argv[i][0] == '-') &&
            (argv[i][1] == 'A' || argv[i][1] == 'a') && argv[i][2] == '\0') {
            g_ascii_boxes = 1;
        } else if ((argv[i][0] == '/' || argv[i][0] == '-') &&
            (argv[i][1] == 'R' || argv[i][1] == 'r') && argv[i][2] == '\0') {
            g_russian_start = 1;
        } else if (file_arg == NULL) {
            file_arg = argv[i];
        }
    }

    init_editor();
    if (g_ascii_boxes) doc.input_lang = LANG_ARA;
    if (g_russian_start) { doc.input_lang = LANG_RUS; doc.rtl_mode = 0; }
    if (file_arg) load_file(file_arg);
    draw_screen();
    
    while (running) {
        /* Check save reminder when no key is pressed */
        if (!kbhit()) {
            if (doc.save_reminder && doc.modified) {
                now = time(NULL);
                if (now - doc.last_save_time >= SAVE_REMIND_SEC) {
                    if (confirm_dialog("Save Reminder", "Time to save! Save now?")) {
                        if (doc.encrypted) save_file_encrypted(doc.filename, doc.password);
                        else save_file(doc.filename);
                        draw_screen();
                    } else {
                        doc.last_save_time = now;  /* Reset timer even if not saving */
                    }
                }
            }
            /* Update status bar (for clock) */
            draw_status_bar();
            update_cursor();
            /* Small delay to avoid 100% CPU usage */
            delay(100);
            continue;
        }
        
        key = get_key();
        if (key & 0x100)
            process_key(key, 1);
        else
            process_key(key, 0);
        draw_status_bar();
        update_cursor();
    }
    
    if (doc.modified) {
        if (confirm_dialog("Exit", "Save changes before exit?")) {
            if (doc.encrypted) save_file_encrypted(doc.filename, doc.password);
            else save_file(doc.filename);
        }
    }
    
    cleanup_editor();
    return 0;
}
