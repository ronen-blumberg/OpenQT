/*
 * OpenQT Configuration Header
 * Hebrew Word Processor - QText 5.5 Clone
 *
 * Modify these settings to customize OpenQT behavior
 */

#ifndef CONFIG_H
#define CONFIG_H

/*---------------------------------------------------------------------------*/
/* Version Information                                                        */
/*---------------------------------------------------------------------------*/

#define OPENQT_VERSION      "3.6.0"
#define OPENQT_VERSION_NUM  0x0360
#define OPENQT_YEAR         "2026"

/*---------------------------------------------------------------------------*/
/* Document Limits                                                            */
/*---------------------------------------------------------------------------*/

#define MAX_LINES           5000        /* Maximum lines per document */
#define MAX_LINE_LEN        256         /* Maximum characters per line */
#define MAX_FILENAME        256         /* Maximum filename length */
#define UNDO_LEVELS         100         /* Number of undo steps */

/*---------------------------------------------------------------------------*/
/* Screen Layout                                                              */
/*---------------------------------------------------------------------------*/

#define SCREEN_WIDTH        80          /* Screen width in columns */
#define SCREEN_HEIGHT       25          /* Screen height in rows */
#define EDIT_TOP            2           /* First editing row */
#define EDIT_BOTTOM         23          /* Last editing row */
#define STATUS_LINE         24          /* Status bar row */

/*---------------------------------------------------------------------------*/
/* Editor Settings                                                            */
/*---------------------------------------------------------------------------*/

#define TAB_SIZE            4           /* Tab stop interval */
#define AUTO_SAVE_INTERVAL  300         /* Auto-save interval (seconds), 0=off */
#define BACKUP_FILES        1           /* Create .BAK files (0/1) */

/*---------------------------------------------------------------------------*/
/* Default Modes                                                              */
/*---------------------------------------------------------------------------*/

#define DEFAULT_HEBREW      1           /* Start in Hebrew mode (0/1) */
#define DEFAULT_RTL         1           /* Start in RTL mode (0/1) */
#define DEFAULT_INSERT      1           /* Start in insert mode (0/1) */
#define DEFAULT_WRAP        1           /* Start with word wrap (0/1) */
#define DEFAULT_RULER       1           /* Show ruler by default (0/1) */

/*---------------------------------------------------------------------------*/
/* Color Scheme                                                               */
/*---------------------------------------------------------------------------*/

/* Color format: 0xBF where B=background, F=foreground */
/* Colors: 0=Black, 1=Blue, 2=Green, 3=Cyan, 4=Red, 5=Magenta, 6=Brown, 7=White */
/* Add 8 for bright foreground */

#define CLR_NORMAL          0x1F        /* Normal text (White on Blue) */
#define CLR_STATUS          0x70        /* Status bar (Black on White) */
#define CLR_MENU            0x70        /* Menu (Black on White) */
#define CLR_MENU_HOT        0x74        /* Menu hotkey (Red on White) */
#define CLR_MENU_SEL        0x0F        /* Selected menu item (White on Black) */
#define CLR_BLOCK           0x3F        /* Selected block (White on Cyan) */
#define CLR_TITLE           0x1E        /* Title bar (Yellow on Blue) */
#define CLR_DIALOG          0x4F        /* Dialog box (White on Red) */
#define CLR_DIALOG_BTN      0x70        /* Dialog button (Black on White) */
#define CLR_RULER           0x1E        /* Ruler (Yellow on Blue) */

/*---------------------------------------------------------------------------*/
/* Hebrew Character Set                                                       */
/*---------------------------------------------------------------------------*/

/* IBM PC Hebrew Code Page 862 character range */
#define HEB_FIRST           128         /* First Hebrew character code */
#define HEB_LAST            154         /* Last Hebrew character code */

/*---------------------------------------------------------------------------*/
/* Font File Locations                                                        */
/*---------------------------------------------------------------------------*/

#define FONT_FILE_PRIMARY   "HEBVGA.COM"
#define FONT_FILE_FALLBACK  "C:\\DOS\\HEBVGA.COM"
#define FONT_FILE_ALT       "C:\\HEBREW\\HEBVGA.COM"

/*---------------------------------------------------------------------------*/
/* Box Drawing Characters (Code Page 437/862)                                 */
/*---------------------------------------------------------------------------*/

#define BOX_TL              0xDA        /* Top-left corner */
#define BOX_TR              0xBF        /* Top-right corner */
#define BOX_BL              0xC0        /* Bottom-left corner */
#define BOX_BR              0xD9        /* Bottom-right corner */
#define BOX_HORIZ           0xC4        /* Horizontal line */
#define BOX_VERT            0xB3        /* Vertical line */
#define BOX_CROSS           0xC5        /* Cross */
#define BOX_T_DOWN          0xC2        /* T pointing down */
#define BOX_T_UP            0xC1        /* T pointing up */
#define BOX_T_RIGHT         0xC3        /* T pointing right */
#define BOX_T_LEFT          0xB4        /* T pointing left */

/* Double-line box characters */
#define DBOX_TL             0xC9        /* Double top-left */
#define DBOX_TR             0xBB        /* Double top-right */
#define DBOX_BL             0xC8        /* Double bottom-left */
#define DBOX_BR             0xBC        /* Double bottom-right */
#define DBOX_HORIZ          0xCD        /* Double horizontal */
#define DBOX_VERT           0xBA        /* Double vertical */

/*---------------------------------------------------------------------------*/
/* Feature Toggles                                                            */
/*---------------------------------------------------------------------------*/

#define FEATURE_UNDO        0           /* Enable undo/redo (not implemented) */
#define FEATURE_SPELL       0           /* Enable spell checking (not implemented) */
#define FEATURE_PRINT       1           /* Enable printing */
#define FEATURE_CLIPBOARD   1           /* Enable clipboard operations */
#define FEATURE_SEARCH      1           /* Enable search/replace */
#define FEATURE_BLOCKS      1           /* Enable block operations */
#define FEATURE_MOUSE       0           /* Enable mouse support (not implemented) */

#endif /* CONFIG_H */



