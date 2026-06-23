@ECHO OFF
REM CHCP 862 so Ctrl+F6 clipboard paste converts UTF-8 Hebrew to CP862.
CHCP 862
HEBVGA
OPENQT %1 %2 %3
