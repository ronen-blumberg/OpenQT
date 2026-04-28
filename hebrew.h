/*
 * OpenQT Hebrew Keyboard Mapping Table
 * Hebrew Word Processor - QText 5.5 Clone
 *
 * This file contains the keyboard mapping from English to Hebrew
 * Based on the standard Israeli keyboard layout
 */

#ifndef HEBREW_H
#define HEBREW_H

/*
 * Hebrew Alphabet in IBM PC Code Page 862
 *
 * Code  Letter       Name
 * ────  ──────       ────────────
 * 128   א           Alef
 * 129   ב           Bet
 * 130   ג           Gimel
 * 131   ד           Dalet
 * 132   ה           He
 * 133   ו           Vav
 * 134   ז           Zayin
 * 135   ח           Chet
 * 136   ט           Tet
 * 137   י           Yod
 * 138   ך           Kaf Sofit (final)
 * 139   כ           Kaf
 * 140   ל           Lamed
 * 141   ם           Mem Sofit (final)
 * 142   מ           Mem
 * 143   ן           Nun Sofit (final)
 * 144   נ           Nun
 * 145   ס           Samech
 * 146   ע           Ayin
 * 147   ף           Pe Sofit (final)
 * 148   פ           Pe
 * 149   ץ           Tsadi Sofit (final)
 * 150   צ           Tsadi
 * 151   ק           Qof
 * 152   ר           Resh
 * 153   ש           Shin
 * 154   ת           Tav
 */

/* Hebrew character codes */
#define HEB_ALEF        128
#define HEB_BET         129
#define HEB_GIMEL       130
#define HEB_DALET       131
#define HEB_HE          132
#define HEB_VAV         133
#define HEB_ZAYIN       134
#define HEB_CHET        135
#define HEB_TET         136
#define HEB_YOD         137
#define HEB_KAF_SOFIT   138
#define HEB_KAF         139
#define HEB_LAMED       140
#define HEB_MEM_SOFIT   141
#define HEB_MEM         142
#define HEB_NUN_SOFIT   143
#define HEB_NUN         144
#define HEB_SAMECH      145
#define HEB_AYIN        146
#define HEB_PE_SOFIT    147
#define HEB_PE          148
#define HEB_TSADI_SOFIT 149
#define HEB_TSADI       150
#define HEB_QOF         151
#define HEB_RESH        152
#define HEB_SHIN        153
#define HEB_TAV         154

/*
 * Standard Israeli Keyboard Layout
 *
 * English -> Hebrew mapping based on physical key position
 *
 *  US Keyboard:   Q  W  E  R  T  Y  U  I  O  P
 *  Hebrew:        /  '  ק  ר  א  ט  ו  ן  ם  פ
 *
 *  US Keyboard:   A  S  D  F  G  H  J  K  L  ;
 *  Hebrew:        ש  ד  ג  כ  ע  י  ח  ל  ך  ף
 *
 *  US Keyboard:   Z  X  C  V  B  N  M  ,  .  /
 *  Hebrew:        ז  ס  ב  ה  נ  מ  צ  ת  ץ  .
 */

/* English to Hebrew keyboard mapping table */
/* Index by ASCII code (0-127), value is Hebrew character or 0 */
static const unsigned char english_to_hebrew[128] = {
/*  0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  0x20 ' ' */ ' ', 0, 0, 0, 0, 0, 0, ',', ')', '(', 0, 0, HEB_TAV, 0, HEB_TSADI_SOFIT, '.',
/*  0x30 '0' */ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, HEB_PE_SOFIT, 0, 0, 0, 0,
/*  0x40 '@' */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  0x50 'P' */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  0x60 '`' */ ';', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  0x70 'p' */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Lowercase letter mappings (a-z) */
#define HEB_MAP_A   HEB_SHIN        /* a -> ש */
#define HEB_MAP_B   HEB_NUN         /* b -> נ */
#define HEB_MAP_C   HEB_BET         /* c -> ב */
#define HEB_MAP_D   HEB_GIMEL       /* d -> ג */
#define HEB_MAP_E   HEB_QOF         /* e -> ק */
#define HEB_MAP_F   HEB_KAF         /* f -> כ */
#define HEB_MAP_G   HEB_AYIN        /* g -> ע */
#define HEB_MAP_H   HEB_YOD         /* h -> י */
#define HEB_MAP_I   HEB_NUN_SOFIT   /* i -> ן */
#define HEB_MAP_J   HEB_CHET        /* j -> ח */
#define HEB_MAP_K   HEB_LAMED       /* k -> ל */
#define HEB_MAP_L   HEB_KAF_SOFIT   /* l -> ך */
#define HEB_MAP_M   HEB_TSADI       /* m -> צ */
#define HEB_MAP_N   HEB_MEM         /* n -> מ */
#define HEB_MAP_O   HEB_MEM_SOFIT   /* o -> ם */
#define HEB_MAP_P   HEB_PE          /* p -> פ */
#define HEB_MAP_Q   '/'             /* q -> / */
#define HEB_MAP_R   HEB_RESH        /* r -> ר */
#define HEB_MAP_S   HEB_DALET       /* s -> ד */
#define HEB_MAP_T   HEB_ALEF        /* t -> א */
#define HEB_MAP_U   HEB_VAV         /* u -> ו */
#define HEB_MAP_V   HEB_HE          /* v -> ה */
#define HEB_MAP_W   '\''            /* w -> ' */
#define HEB_MAP_X   HEB_SAMECH      /* x -> ס */
#define HEB_MAP_Y   HEB_TET         /* y -> ט */
#define HEB_MAP_Z   HEB_ZAYIN       /* z -> ז */

/*
 * Final Letters (Sofit) Mapping
 *
 * When a letter appears at the end of a word, it may change form:
 * כ (Kaf)   -> ך (Kaf Sofit)
 * מ (Mem)   -> ם (Mem Sofit)
 * נ (Nun)   -> ן (Nun Sofit)
 * פ (Pe)    -> ף (Pe Sofit)
 * צ (Tsadi) -> ץ (Tsadi Sofit)
 */

/* Convert regular letter to final form (0 if no final form) */
static const unsigned char to_final_letter[256] = {
    [HEB_KAF]   = HEB_KAF_SOFIT,
    [HEB_MEM]   = HEB_MEM_SOFIT,
    [HEB_NUN]   = HEB_NUN_SOFIT,
    [HEB_PE]    = HEB_PE_SOFIT,
    [HEB_TSADI] = HEB_TSADI_SOFIT,
    /* All other entries are 0 */
};

/* Convert final letter back to regular form (0 if not a final letter) */
static const unsigned char from_final_letter[256] = {
    [HEB_KAF_SOFIT]   = HEB_KAF,
    [HEB_MEM_SOFIT]   = HEB_MEM,
    [HEB_NUN_SOFIT]   = HEB_NUN,
    [HEB_PE_SOFIT]    = HEB_PE,
    [HEB_TSADI_SOFIT] = HEB_TSADI,
    /* All other entries are 0 */
};

/*
 * Inline helper functions
 */

/* Check if character is a Hebrew letter */
static inline int is_hebrew(unsigned char c) {
    return (c >= 128 && c <= 154);
}

/* Check if character is a Hebrew final letter */
static inline int is_final_letter(unsigned char c) {
    return (c == HEB_KAF_SOFIT || c == HEB_MEM_SOFIT ||
            c == HEB_NUN_SOFIT || c == HEB_PE_SOFIT ||
            c == HEB_TSADI_SOFIT);
}

/* Check if character has a final form */
static inline int has_final_form(unsigned char c) {
    return (c == HEB_KAF || c == HEB_MEM ||
            c == HEB_NUN || c == HEB_PE ||
            c == HEB_TSADI);
}

/* Convert English key to Hebrew (returns 0 if no mapping) */
static inline unsigned char key_to_hebrew(unsigned char key) {
    switch (key) {
        case 'a': case 'A': return HEB_MAP_A;
        case 'b': case 'B': return HEB_MAP_B;
        case 'c': case 'C': return HEB_MAP_C;
        case 'd': case 'D': return HEB_MAP_D;
        case 'e': case 'E': return HEB_MAP_E;
        case 'f': case 'F': return HEB_MAP_F;
        case 'g': case 'G': return HEB_MAP_G;
        case 'h': case 'H': return HEB_MAP_H;
        case 'i': case 'I': return HEB_MAP_I;
        case 'j': case 'J': return HEB_MAP_J;
        case 'k': case 'K': return HEB_MAP_K;
        case 'l': case 'L': return HEB_MAP_L;
        case 'm': case 'M': return HEB_MAP_M;
        case 'n': case 'N': return HEB_MAP_N;
        case 'o': case 'O': return HEB_MAP_O;
        case 'p': case 'P': return HEB_MAP_P;
        case 'q': case 'Q': return HEB_MAP_Q;
        case 'r': case 'R': return HEB_MAP_R;
        case 's': case 'S': return HEB_MAP_S;
        case 't': case 'T': return HEB_MAP_T;
        case 'u': case 'U': return HEB_MAP_U;
        case 'v': case 'V': return HEB_MAP_V;
        case 'w': case 'W': return HEB_MAP_W;
        case 'x': case 'X': return HEB_MAP_X;
        case 'y': case 'Y': return HEB_MAP_Y;
        case 'z': case 'Z': return HEB_MAP_Z;
        case ',': return HEB_TAV;
        case '.': return HEB_TSADI_SOFIT;
        case ';': return HEB_PE_SOFIT;
        case '/': return '.';
        case '\'': return ',';
        default: return 0;
    }
}

#endif /* HEBREW_H */







