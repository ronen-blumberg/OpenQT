@ECHO OFF
REM CHCP 864 so Ctrl+F6 clipboard paste converts UTF-8 Arabic to CP864.
CHCP 864
ARABVGA
OPENQT /A %1 %2 %3
