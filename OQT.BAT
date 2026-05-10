@ECHO OFF
REM OpenQT launcher.
REM
REM Tri-script mode (Hebrew + Arabic + English on one screen) requires
REM ARABVGA built from the current source with /P (partial-upload) flag
REM support, so it overlays only 0xA0-0xFE on top of HEBVGA's full font.
REM Until you've rebuilt ARABVGA, use Hebrew or Arabic sessions
REM separately:
REM
REM   OQT H file.txt   == Hebrew + English (HEBVGA)
REM   OQT A file.txt   == Arabic + English (ARABVGA, /A flag for ASCII boxes)
REM   OQT R file.txt   == Russian + English (RUSVGA, /R flag for Russian start)
REM   OQT 3 file.txt   == tri-script Hebrew+Arabic+English (requires ARABVGA /P)
REM   OQT   file.txt   == defaults to Hebrew (legacy behavior)

IF "%1"=="H" GOTO HEB
IF "%1"=="h" GOTO HEB
IF "%1"=="A" GOTO ARA
IF "%1"=="a" GOTO ARA
IF "%1"=="R" GOTO RUS
IF "%1"=="r" GOTO RUS
IF "%1"=="3" GOTO TRI

REM --- default: Hebrew ---
HEBVGA
OPENQT %1 %2 %3
GOTO END

:HEB
HEBVGA
SHIFT
OPENQT %1 %2 %3
GOTO END

:ARA
ARABVGA
SHIFT
OPENQT /A %1 %2 %3
GOTO END

:RUS
RUSVGA
SHIFT
OPENQT /R %1 %2 %3
GOTO END

:TRI
HEBVGA
ARABVGA /P
SHIFT
OPENQT /A %1 %2 %3
GOTO END

:END
