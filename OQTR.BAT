@ECHO OFF
REM CHCP 866 so Ctrl+F6 clipboard paste converts UTF-8 Russian to CP866.
CHCP 866
RUSVGA
OPENQT /R %1 %2 %3
