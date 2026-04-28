/*
 * bidi.h - Bidirectional Text Algorithm Implementation for OpenQT
 * 
 * Include this file in openqt.c and call bidi_reorder() before drawing lines.
 * 
 * Usage:
 *   1. Call bidi_reorder(line, strlen(line)) before displaying
 *   2. Use bidi.visual[] for screen output
 *   3. Use logical_to_visual() / visual_to_logical() for cursor positioning
 */

#ifndef BIDI_H
#define BIDI_H

/* BiDi character types */
#define BIDI_R          0   /* Right-to-left (Hebrew) */
#define BIDI_L          1   /* Left-to-right (English letters) */
#define BIDI_EN         2   /* European Number (always LTR) */
#define BIDI_WS         3   /* Whitespace */
#define BIDI_ON         4   /* Other Neutrals (punctuation) */

#define BIDI_MAX_LEN    256

/* BiDi reordering structure */
typedef struct {
    unsigned char visual[BIDI_MAX_LEN];     /* Visual order characters */
    int log_to_vis[BIDI_MAX_LEN];           /* Logical -> visual mapping */
    int vis_to_log[BIDI_MAX_LEN];           /* Visual -> logical mapping */
    unsigned char types[BIDI_MAX_LEN];      /* Character types */
    unsigned char levels[BIDI_MAX_LEN];     /* Embedding levels */
    int len;                                 /* Line length */
} BidiLine;

/* Global BiDi state */
static BidiLine bidi;

/* Determine BiDi character type */
static int get_bidi_type(unsigned char ch)
{
    /* Hebrew characters (CP862: 128-154) */
    if (ch >= 128 && ch <= 154) return BIDI_R;
    
    /* English letters */
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) return BIDI_L;
    
    /* Numbers - always LTR */
    if (ch >= '0' && ch <= '9') return BIDI_EN;
    
    /* Whitespace */
    if (ch == ' ' || ch == '\t') return BIDI_WS;
    
    /* Everything else (punctuation, etc.) */
    return BIDI_ON;
}

/*
 * bidi_reorder - Reorder a logical string to visual order
 *
 * Parameters:
 *   logical - Input string in logical (typing) order
 *   len - Length of string
 *   rtl_base - 1 for RTL paragraph, 0 for LTR paragraph
 *
 * After calling:
 *   bidi.visual[] contains characters in display order
 *   bidi.log_to_vis[] maps logical position to visual position
 *   bidi.vis_to_log[] maps visual position to logical position
 */
static void bidi_reorder(const char *logical, int len, int rtl_base)
{
    int i, j, k;
    int run_start, run_end;
    int max_level, level;
    int temp_log[BIDI_MAX_LEN];
    unsigned char temp_vis[BIDI_MAX_LEN];
    int base_level;
    
    if (len <= 0 || len >= BIDI_MAX_LEN) {
        bidi.len = 0;
        return;
    }
    
    bidi.len = len;
    base_level = rtl_base ? 1 : 0;
    
    /* ===== STEP 1: Assign character types ===== */
    for (i = 0; i < len; i++) {
        bidi.types[i] = get_bidi_type((unsigned char)logical[i]);
        bidi.levels[i] = base_level;
    }
    
    /* ===== STEP 2: Resolve types and assign embedding levels ===== */
    for (i = 0; i < len; i++) {
        if (bidi.types[i] == BIDI_R) {
            /* Hebrew: always level 1 (RTL) */
            bidi.levels[i] = 1;
        }
        else if (bidi.types[i] == BIDI_L) {
            /* English: level 2 in RTL context (embedded LTR) */
            bidi.levels[i] = rtl_base ? 2 : 0;
        }
        else if (bidi.types[i] == BIDI_EN) {
            /* Numbers: always LTR, level 2 in RTL context */
            bidi.levels[i] = rtl_base ? 2 : 0;
        }
        else {
            /* Neutrals: resolve by context */
            int prev_strong = base_level ? BIDI_R : BIDI_L;
            int next_strong = base_level ? BIDI_R : BIDI_L;
            
            /* Find previous strong type */
            for (j = i - 1; j >= 0; j--) {
                if (bidi.types[j] == BIDI_R) {
                    prev_strong = BIDI_R;
                    break;
                }
                if (bidi.types[j] == BIDI_L || bidi.types[j] == BIDI_EN) {
                    prev_strong = BIDI_L;
                    break;
                }
            }
            
            /* Find next strong type */
            for (j = i + 1; j < len; j++) {
                if (bidi.types[j] == BIDI_R) {
                    next_strong = BIDI_R;
                    break;
                }
                if (bidi.types[j] == BIDI_L || bidi.types[j] == BIDI_EN) {
                    next_strong = BIDI_L;
                    break;
                }
            }
            
            /* Same direction on both sides: take that direction */
            if (prev_strong == next_strong) {
                bidi.levels[i] = (prev_strong == BIDI_R) ? 1 : (rtl_base ? 2 : 0);
            } else {
                /* Mixed: use base direction */
                bidi.levels[i] = base_level;
            }
        }
    }
    
    /* ===== STEP 3: Initialize for reordering ===== */
    for (i = 0; i < len; i++) {
        temp_log[i] = i;
        temp_vis[i] = (unsigned char)logical[i];
    }
    
    /* ===== STEP 4: Reorder based on levels ===== */
    if (rtl_base) {
        /* Find maximum level */
        max_level = 0;
        for (i = 0; i < len; i++) {
            if (bidi.levels[i] > max_level) max_level = bidi.levels[i];
        }
        
        /* Reverse runs at each level from max down to 1 */
        for (level = max_level; level >= 1; level--) {
            i = 0;
            while (i < len) {
                if (bidi.levels[i] >= level) {
                    /* Found start of run at this level */
                    run_start = i;
                    while (i < len && bidi.levels[i] >= level) i++;
                    run_end = i - 1;
                    
                    /* Reverse this run in-place */
                    for (j = run_start, k = run_end; j < k; j++, k--) {
                        unsigned char tc = temp_vis[j];
                        int tl = temp_log[j];
                        temp_vis[j] = temp_vis[k];
                        temp_log[j] = temp_log[k];
                        temp_vis[k] = tc;
                        temp_log[k] = tl;
                    }
                } else {
                    i++;
                }
            }
        }
        
        /* For RTL base: reverse entire line */
        for (j = 0, k = len - 1; j < k; j++, k--) {
            unsigned char tc = temp_vis[j];
            int tl = temp_log[j];
            temp_vis[j] = temp_vis[k];
            temp_log[j] = temp_log[k];
            temp_vis[k] = tc;
            temp_log[k] = tl;
        }
    }
    
    /* ===== STEP 5: Store results ===== */
    for (i = 0; i < len; i++) {
        bidi.visual[i] = temp_vis[i];
        bidi.vis_to_log[i] = temp_log[i];
    }
    bidi.visual[len] = '\0';
    
    /* Build reverse mapping (logical to visual) */
    for (i = 0; i < len; i++) {
        bidi.log_to_vis[bidi.vis_to_log[i]] = i;
    }
}

/* Convert visual position to logical position */
static int visual_to_logical(int vis_pos)
{
    if (vis_pos < 0) return 0;
    if (vis_pos >= bidi.len) return bidi.len;
    return bidi.vis_to_log[vis_pos];
}

/* Convert logical position to visual position */
static int logical_to_visual(int log_pos)
{
    if (log_pos < 0) return 0;
    if (log_pos >= bidi.len) return bidi.len;
    return bidi.log_to_vis[log_pos];
}

#endif /* BIDI_H */

/*
 * INTEGRATION GUIDE FOR OPENQT.C:
 *
 * 1. Include this header after the other includes:
 *    #include "bidi.h"
 *
 * 2. Add to Document struct:
 *    int embedded_ltr;  // F10 toggle for embedded English
 *
 * 3. Initialize in init_document():
 *    doc.embedded_ltr = 0;
 *
 * 4. Modify draw_line() to use BiDi:
 *    - Before the drawing loop, call:
 *      bidi_reorder(line, len, doc.rtl_mode);
 *    - Use bidi.visual[i] instead of line[i] for characters
 *
 * 5. Modify update_cursor() for BiDi:
 *    - Call bidi_reorder() first
 *    - Convert doc.cursor_x using logical_to_visual()
 *
 * 6. Add toggle_embedded_ltr():
 *    void toggle_embedded_ltr(void) {
 *        if (doc.hebrew_mode) {
 *            doc.embedded_ltr = !doc.embedded_ltr;
 *            draw_menu_bar();
 *            draw_status_bar();
 *        }
 *    }
 *
 * 7. Map F10 to toggle_embedded_ltr() in process_key()
 *
 * 8. Modify character input in process_key():
 *    if (doc.hebrew_mode && !doc.embedded_ltr) {
 *        // Translate to Hebrew (except numbers)
 *        if (!(ch >= '0' && ch <= '9')) {
 *            unsigned char heb = translate_hebrew(ch);
 *            if (heb) ch = heb;
 *        }
 *    }
 *    // Otherwise type character as-is (English or embedded)
 *
 * Example usage scenario:
 *   User types: "שלום 123 " then F10 "HELLO" F10 " שלום 123"
 *   Logical:    "שלום 123 HELLO שלום 123"
 *   Visual:     "123 שלום HELLO 123 שלום" (right-aligned, RTL base)
 *   
 *   - Hebrew runs are reversed (displayed RTL)
 *   - Numbers stay 1-2-3 (not 3-2-1)
 *   - English stays H-E-L-L-O
 *   - Overall order is RTL because of rtl_mode
 */
