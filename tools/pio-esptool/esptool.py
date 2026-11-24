#!/usr/bin/env python3
"""Run uv-managed esptool without PlatformIO invoking pip during installation."""
import os
import sys
# This wrapper's filename otherwise shadows the installed esptool package.
sys.path = [p for p in sys.path if os.path.abspath(p) != os.path.dirname(os.path.abspath(__file__))]
from esptool import _main
if __name__ == "__main__":
    _main()
