#!/usr/bin/env bash
# Launch the OpenQT host helper daemon.
# Run this on the Linux host (NOT inside DOSBox) before starting openqt.exe.
# Leave it running in its own terminal while you use the editor.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
export OQT_BRIDGE="${OQT_BRIDGE:-$(cd "$HERE/.." && pwd)/BRIDGE}"
# Use the locally pre-downloaded multilingual "medium" model (en + he dictation)
# if present, so faster-whisper loads from disk instead of hitting the HF Hub.
LOCAL_MODEL="$HERE/models/faster-whisper-medium"
if [ -f "$LOCAL_MODEL/model.bin" ]; then
  export OQT_WHISPER_MODEL="${OQT_WHISPER_MODEL:-$LOCAL_MODEL}"
fi
exec "$HERE/venv/bin/python" "$HERE/oqt_helper.py"
