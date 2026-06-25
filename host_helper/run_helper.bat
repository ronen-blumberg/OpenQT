@echo off
rem Launch the OpenQT host helper daemon on Windows.
rem Run this on the host (NOT inside DOSBox-X) before starting openqt.exe.
rem Leave the window open while you use the editor.
setlocal
set "HERE=%~dp0"

rem Default the bridge folder to ..\BRIDGE (= the DOSBox-X C:\OPENQT\BRIDGE
rem mount). Override by setting OQT_BRIDGE before running this script.
if not defined OQT_BRIDGE set "OQT_BRIDGE=%HERE%..\BRIDGE"

rem Use the locally pre-downloaded multilingual "medium" model (en + he
rem dictation) if present, so faster-whisper loads from disk instead of the Hub.
set "LOCAL_MODEL=%HERE%models\faster-whisper-medium"
if exist "%LOCAL_MODEL%\model.bin" (
    if not defined OQT_WHISPER_MODEL set "OQT_WHISPER_MODEL=%LOCAL_MODEL%"
)

rem Prefer the venv interpreter; fall back to whatever "python" is on PATH.
set "PY=%HERE%venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%HERE%oqt_helper.py"
endlocal
