/*
 * OQT2QT - OpenQT to QText 5.5 Converter
 * Converts OpenQT files to QText 5.5 format
 * 
 * Usage: oqt2qt input.txt output.txt
 * 
 * Compile with Watcom: wcl386 -bt=dos -l=dos4g oqt2qt.c -fe=oqt2qt.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
#define LINE_WIDTH 71   /* QText line width */

/* OpenQT format codes (to remove) */
#define FMT_BOLD        0x02
#define FMT_UNDERLINE   0x03
#define FMT_BOLDUNDER   0x04

/* Check if character is Hebrew (CP862: 0x80-0x9A) */
int is_hebrew(unsigned char ch)
{
    return (ch >= 0x80 && ch <= 0x9A);
}

/* Check if character is OpenQT format code */
int is_format_code(unsigned char ch)
{
    return (ch == FMT_BOLD || ch == FMT_UNDERLINE || ch == FMT_BOLDUNDER);
}

/* Reverse a portion of the buffer */
void reverse_range(unsigned char *buf, int start, int end)
{
    unsigned char temp;
    while (start < end) {
        temp = buf[start];
        buf[start] = buf[end];
        buf[end] = temp;
        start++;
        end--;
    }
}

/* Process line: remove format codes and convert Hebrew to visual order */
int process_line(unsigned char *input, unsigned char *output)
{
    unsigned char clean[MAX_LINE];
    int i, j, clean_len = 0;
    int run_start;
    
    /* First pass: remove format codes */
    for (i = 0; input[i]; i++) {
        if (!is_format_code(input[i])) {
            clean[clean_len++] = input[i];
        }
    }
    clean[clean_len] = '\0';
    
    /* Second pass: reverse Hebrew runs (logical to visual order) */
    i = 0;
    while (i < clean_len) {
        if (is_hebrew(clean[i])) {
            run_start = i;
            
            /* Find end of Hebrew run (include spaces between Hebrew words) */
            while (i < clean_len) {
                if (is_hebrew(clean[i])) {
                    i++;
                } else if (clean[i] == ' ') {
                    int found_more = 0;
                    for (j = i + 1; j < clean_len; j++) {
                        if (is_hebrew(clean[j])) {
                            found_more = 1;
                            break;
                        }
                        if (clean[j] != ' ') {
                            break;
                        }
                    }
                    if (found_more) {
                        i++;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            
            /* Reverse the Hebrew run */
            reverse_range(clean, run_start, i - 1);
        } else {
            i++;
        }
    }
    
    /* Copy to output */
    memcpy(output, clean, clean_len);
    output[clean_len] = '\0';
    
    return clean_len;
}

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned char line[MAX_LINE];
    unsigned char output[MAX_LINE];
    unsigned char padded[MAX_LINE];
    int len, out_len, pad_left;
    int line_count = 0;
    
    printf("OQT2QT - OpenQT to QText 5.5 Converter\n");
    printf("======================================\n\n");
    
    if (argc != 3) {
        printf("Usage: oqt2qt input.txt output.txt\n\n");
        printf("Converts OpenQT files to QText 5.5 format.\n");
        printf("The output file can be opened in QText 5.5.\n");
        return 1;
    }
    
    fin = fopen(argv[1], "rb");
    if (!fin) {
        printf("Error: Cannot open input file '%s'\n", argv[1]);
        return 1;
    }
    
    fout = fopen(argv[2], "wb");
    if (!fout) {
        printf("Error: Cannot create output file '%s'\n", argv[2]);
        fclose(fin);
        return 1;
    }
    
    printf("Converting '%s' to '%s'...\n", argv[1], argv[2]);
    
    while (fgets((char *)line, sizeof(line), fin)) {
        len = strlen((char *)line);
        
        /* Remove CR/LF at end */
        while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
            len--;
        }
        line[len] = '\0';
        
        /* Process the line (remove format codes, reverse Hebrew) */
        out_len = process_line(line, output);
        
        /* Add left padding (6 spaces like QText default margin) */
        pad_left = 6;
        memset(padded, ' ', pad_left);
        memcpy(padded + pad_left, output, out_len);
        padded[pad_left + out_len] = '\0';
        
        /* Write line content */
        fputs((char *)padded, fout);
        
        /* Write CR LF line ending (standard QText format) */
        fputc('\r', fout);
        fputc('\n', fout);
        
        line_count++;
    }
    
    fclose(fin);
    fclose(fout);
    
    printf("Converted %d lines.\n", line_count);
    printf("Open '%s' in QText 5.5.\n", argv[2]);
    
    return 0;
}
