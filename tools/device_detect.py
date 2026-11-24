#!/usr/bin/env python3
import json
from common import detect_device
if __name__ == "__main__":
    print(json.dumps(detect_device(), indent=2))
