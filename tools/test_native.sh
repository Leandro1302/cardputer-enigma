#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/env.sh"
cd "$ENIGMA_ROOT"
cmake -S . -B .pio/native -DCMAKE_BUILD_TYPE=Debug
cmake --build .pio/native -j 2
ctest --test-dir .pio/native --output-on-failure
python -m pytest -q -o cache_dir="$ENIGMA_ROOT/.cache/pytest"
