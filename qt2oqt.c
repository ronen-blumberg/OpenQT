/*
 * QT2OQT - QText 5.5 to OpenQT Converter
 * 
 * Converts QText 5.5 files to plain text format for OpenQT
 * 
 * QText stores text in VISUAL order (entire line as displayed)
 * OpenQT expects LOGICAL order (applies BiDi for display)
 * This converter reverses entire Hebrew lines to convert visual->logical
 * 
 * Compile: wcl386 -bt=dos -l=dos4g qt2oqt.c -fe=qt2oqt.exe
 * Usage: qt2oqt input.txt output.txt
 */

#include <stdio.h>
#include <string.h>

#define MARKER_START    0xAF    /* QText block start */
#define MARKER_END      0xAE    /* QText block end */
#define MAX_LINE        512

/* Check if byte is Hebrew in CP862 (0x80-0x9A) */
int is_hebrew(unsigned char ch)
{
    return (ch >= 0x80 && ch <= 0x9A);
}

/* Check if line contains Hebrew */
int has_hebrew(unsigned char *line, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (is_hebrew(line[i])) return 1;
    }
    return 0;
}

/* Reverse entire line */
void reverse_line(unsigned char *line, int len)
{
    int i, j;
    unsigned char temp;
    for (i = 0, j = len - 1; i < j; i++, j--) {
        temp = line[i];
        line[i] = line[j];
        line[j] = temp;
    }
}

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char line[MAX_LINE];
    unsigned char *start;
    int i, len;
    
    if (argc != 3) {
        printf("QT2OQT - QText 5.5 to OpenQT Converter\n");
        printf("Converts visual Hebrew to logical order\n");
        printf("Usage: qt2oqt input.txt output.txt\n");
        return 1;
    }
    
    fin = fopen(argv[1], "rb");
    if (!fin) {
        printf("Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }
    
    fout = fopen(argv[2], "wb");
    if (!fout) {
        printf("Error: Cannot create output file %s\n", argv[2]);
        fclose(fin);
        return 1;
    }
    
    printf("Converting %s to %s...\n", argv[1], argv[2]);
    
    while (fgets((char *)line, MAX_LINE, fin)) {
        len = strlen((char *)line);
        
        /* Remove CR/LF at end */
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
            line[--len] = '\0';
        }
        
        /* Remove QText markers */
        for (i = 0; i < len; i++) {
            if (line[i] == MARKER_START || line[i] == MARKER_END) {
                memmove(line + i, line + i + 1, len - i);
                len--;
                i--;
            }
        }
        
        /* Skip leading spaces */
        start = line;
        while (*start == ' ') start++;
        len = strlen((char *)start);
        
        /* Skip trailing spaces */
        while (len > 0 && start[len-1] == ' ') {
            start[--len] = '\0';
        }
        
        /* Reverse entire line if it contains Hebrew */
        if (has_hebrew(start, len)) {
            reverse_line(start, len);
        }
        
        /* Write line */
        fputs((char *)start, fout);
        fputs("\r\n", fout);
    }
    
    fclose(fin);
    fclose(fout);
    
    printf("Conversion complete!\n");
    return 0;
}
