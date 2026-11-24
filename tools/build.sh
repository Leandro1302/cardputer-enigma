#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
cd "$ENIGMA_ROOT"
exec "$ENIGMA_ROOT/.venv/bin/python" -m platformio run -e cardputer -j 2 "$@"
