#!/usr/bin/env bash
# Source this file; never changes HOME or the system Python installation.
ENIGMA_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export ENIGMA_ROOT
export UV_CACHE_DIR="$ENIGMA_ROOT/.uv-cache"
export UV_PROJECT_ENVIRONMENT="$ENIGMA_ROOT/.venv"
export UV_PYTHON_DOWNLOADS=never
export UV_PYTHON_INSTALL_DIR="$ENIGMA_ROOT/.uv-python"
export UV_TOOL_DIR="$ENIGMA_ROOT/.uv-tools"
export PLATFORMIO_CORE_DIR="$ENIGMA_ROOT/.platformio"
export PLATFORMIO_BUILD_DIR="$ENIGMA_ROOT/.pio/build"
export PLATFORMIO_LIBDEPS_DIR="$ENIGMA_ROOT/.pio/libdeps"
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no
export XDG_CACHE_HOME="$ENIGMA_ROOT/.cache"
export XDG_CONFIG_HOME="$ENIGMA_ROOT/.config"
export XDG_DATA_HOME="$ENIGMA_ROOT/.local/share"
export TMPDIR="$ENIGMA_ROOT/.tmp"
export PYTHONPYCACHEPREFIX="$ENIGMA_ROOT/.cache/pycache"
export PYTHONNOUSERSITE=1
export PATH="$ENIGMA_ROOT/.venv/bin:$PATH"
mkdir -p "$UV_CACHE_DIR" "$PLATFORMIO_CORE_DIR" "$TMPDIR" "$XDG_CACHE_HOME" "$XDG_CONFIG_HOME" "$XDG_DATA_HOME" "$ENIGMA_ROOT/logs"
