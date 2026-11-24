#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
cd "$ENIGMA_ROOT"
UV_BIN="${UV_BIN:-$(command -v uv || true)}"
if [[ -z "$UV_BIN" ]]; then
  echo 'uv must already be installed; set UV_BIN to its executable.' >&2
  exit 1
fi
if [[ -f uv.lock ]]; then
  "$UV_BIN" sync --locked --python /usr/bin/python3
else
  "$UV_BIN" lock --python /usr/bin/python3
  "$UV_BIN" sync --locked --python /usr/bin/python3
fi
"$ENIGMA_ROOT/.venv/bin/python" -m platformio pkg install -e cardputer
