#!/usr/bin/env python3
"""Two complete reads; never overwrite an existing backup; no flash writes."""
import hashlib
import json
import os
import struct
from datetime import datetime, timezone
from esptool import cmds
from common import ROOT, FLASH_BYTES, backup_directory, connect_verified

def main():
    os.umask(0o077)
    directory = backup_directory()
    directory.mkdir(parents=True, exist_ok=True)
    target = directory / "flash_backup.bin"
    if (directory / "backup_info.json").exists():
        raise RuntimeError("Existing backup protected; refusing to overwrite")
    esp, info = connect_verified()
    print(json.dumps(info, indent=2), flush=True)
    try:
        esp = esp.run_stub()
        esp.change_baud(460800)
        # Resume verification of a completed first read after an interrupted run.
        # Its contents are never replaced; disagreement permanently blocks writes.
        resumed = target.exists()
        first = target.read_bytes() if resumed else cmds.read_flash(esp, 0, FLASH_BYTES, no_progress=True)
        if len(first) != FLASH_BYTES:
            raise RuntimeError("Short first flash read")
        if not resumed:
            with target.open("xb") as f:
                f.write(first)
                f.flush()
                os.fsync(f.fileno())
        print("First complete flash read saved. Reading all flash again...", flush=True)
        second = cmds.read_flash(esp, 0, FLASH_BYTES, no_progress=True)
        sha = hashlib.sha256(first).hexdigest()
        if len(second) != FLASH_BYTES or hashlib.sha256(second).hexdigest() != sha:
            raise RuntimeError("Independent full flash reads disagree; no flashing permitted")
        if first[0] != 0xE9:
            raise RuntimeError("Unexpected bootloader header; backup needs investigation")
        parts = []
        for offset in range(0x8000, 0x9000, 32):
            record = first[offset:offset + 32]
            if record[:2] != b"\xaa\x50":
                break
            _, kind, subtype, address, size, label, flags = struct.unpack("<HBBII16sI", record)
            parts.append({"type": kind, "subtype": subtype, "offset": address, "size": size,
                          "label": label.split(b"\0")[0].decode("ascii", "replace"), "flags": flags})
        if not parts or any(p["offset"] + p["size"] > FLASH_BYTES for p in parts):
            raise RuntimeError("Unrecognized partition layout; backup needs investigation")
        info.update(verified=True, restorable_plaintext=True, sha256=sha,
                    verification="Two separate complete SPI flash reads with matching SHA-256",
                    verification_resumed=resumed,
                    timestamp_utc=datetime.now(timezone.utc).isoformat(), read_offset=0,
                    read_length=FLASH_BYTES, baud=460800, stub=True, partitions=parts,
                    original_boot_header={"magic": first[0], "flash_mode_code": first[2],
                                          "flash_size_frequency_code": first[3]},
                    board_basis="User-identified M5Stack Cardputer; ESP32-S3 and 8MB verified; official M5Stack pinout/build configuration")
        with (directory / "flash_backup.sha256").open("x") as f:
            f.write(f"{sha}  flash_backup.bin\n")
        with (directory / "backup_info.json").open("x") as f:
            json.dump(info, f, indent=2)
            f.write("\n")
        with (directory / "restore.sh").open("x") as f:
            f.write('#!/usr/bin/env bash\nset -euo pipefail\nROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"\nsource "$ROOT/tools/env.sh"\nexec "$ROOT/.venv/bin/python" "$ROOT/tools/flash.py" --restore-bruce "$@"\n')
        os.chmod(directory / "restore.sh", 0o700)
        os.chmod(target, 0o400)
        print(f"BACKUP VERIFIED: {FLASH_BYTES} bytes, SHA-256 {sha}", flush=True)
    finally:
        # Return to the original application; no flash has been changed.
        try:
            esp.hard_reset()
        finally:
            esp._port.close()

if __name__ == "__main__":
    main()
