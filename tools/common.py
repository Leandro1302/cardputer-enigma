"""Project-local environment and guarded access to the one authorized device."""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import re
import time

ROOT = Path(__file__).resolve().parents[1]
FLASH_BYTES = 8 * 1024 * 1024

def configured_serial(*, required=True):
    path = ROOT / ".config/enigma-cardputer/device.json"
    try:
        config = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        if not required:
            return None
        raise RuntimeError("Set the authorized Cardputer serial in .config/enigma-cardputer/device.json") from None
    except ValueError:
        raise RuntimeError("Invalid JSON in .config/enigma-cardputer/device.json") from None
    serial = config.get("serial") if isinstance(config, dict) else None
    if not isinstance(serial, str) or not re.fullmatch(r"[0-9a-fA-F]{2}(?::[0-9a-fA-F]{2}){5}", serial):
        raise RuntimeError("Expected a colon-separated Cardputer serial in .config/enigma-cardputer/device.json")
    return serial.lower()

def local_environment():
    env = os.environ.copy()
    for key, value in {
        "PLATFORMIO_CORE_DIR": ".platformio", "PLATFORMIO_BUILD_DIR": ".pio/build",
        "PLATFORMIO_LIBDEPS_DIR": ".pio/libdeps", "UV_CACHE_DIR": ".uv-cache",
        "UV_PROJECT_ENVIRONMENT": ".venv", "TMPDIR": ".tmp",
        "XDG_CACHE_HOME": ".cache", "XDG_CONFIG_HOME": ".config",
        "XDG_DATA_HOME": ".local/share", "PYTHONPYCACHEPREFIX": ".cache/pycache",
    }.items():
        path = ROOT / value
        path.mkdir(parents=True, exist_ok=True)
        env[key] = str(path)
    env.update(PYTHONNOUSERSITE="1", UV_PYTHON_DOWNLOADS="never", PLATFORMIO_SETTING_ENABLE_TELEMETRY="no")
    env["PATH"] = str(ROOT / ".venv/bin") + os.pathsep + env.get("PATH", "")
    return env

def detect_device(timeout=10):
    from serial.tools import list_ports
    serial = configured_serial()
    stable_port = f"/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_{serial.upper()}-if00"
    deadline = time.monotonic() + timeout
    while True:
        matches = [p for p in list_ports.comports() if p.vid == 0x303A and p.pid == 0x1001
                   and (p.serial_number or "").lower() == serial]
        if len(matches) > 1:
            raise RuntimeError("Ambiguous device identity; refusing hardware access")
        if matches:
            p = matches[0]
            port = stable_port if Path(stable_port).exists() and os.path.realpath(stable_port) == p.device else p.device
            return {"port": port, "device": p.device, "usb_serial": p.serial_number,
                    "vid": p.vid, "pid": p.pid, "location": p.location}
        if time.monotonic() >= deadline:
            raise RuntimeError("Authorized Cardputer not visible; check USB access/host execution, not just cable")
        time.sleep(0.25)

def connect_verified():
    import esptool
    from esptool import cmds
    device = detect_device()
    esp = esptool.detect_chip(device["port"], baud=115200, connect_mode="usb-reset", connect_attempts=3)
    try:
        if esp.CHIP_NAME != "ESP32-S3":
            raise RuntimeError(f"Wrong chip: {esp.CHIP_NAME}")
        security = esp.get_security_info()
        if security["parsed_flags"]["SECURE_BOOT_EN"] or security["parsed_flags"]["SECURE_DOWNLOAD_ENABLE"]:
            raise RuntimeError("Secure boot/download enabled: backup and flashing are blocked")
        if security["flash_crypt_cnt"].bit_count() % 2 or esp.get_flash_encryption_enabled() or esp.get_secure_boot_enabled():
            raise RuntimeError("Flash encryption or secure boot enabled: no plaintext restore assumed")
        mac = ":".join(f"{b:02x}" for b in esp.read_mac())
        if mac != device["usb_serial"].lower():
            raise RuntimeError("Chip MAC does not match authorized USB identity")
        cmds.attach_flash(esp)
        size = cmds.detect_flash_size(esp)
        if size != "8MB":
            raise RuntimeError(f"Unexpected physical flash size {size}; expected Cardputer 8MB")
        info = {**device, "chip": esp.CHIP_NAME, "description": esp.get_chip_description(),
                "mac": mac, "flash_size": size, "flash_bytes": FLASH_BYTES,
                "flash_id": hex(esp.flash_id()), "security": security,
                "esptool_version": esptool.__version__}
        return esp, info
    except BaseException:
        esp._port.close()
        raise

def digest(path):
    with open(path, "rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()

def backup_directory():
    local = ROOT / 'backup/bruce'
    sibling = ROOT.parent / 'backup/bruce'
    # A checkout under cardputer/enigma keeps its private backup one level up.
    return sibling if not local.exists() and sibling.exists() else local

def verified_backup(directory=None):
    serial = configured_serial()
    directory = directory if directory is not None else backup_directory()
    info = json.loads((directory / "backup_info.json").read_text())
    image = directory / "flash_backup.bin"
    if not info.get("verified") or not info.get("restorable_plaintext"):
        raise RuntimeError("Bruce backup has not been verified as restorable")
    if info["mac"] != serial or image.stat().st_size != FLASH_BYTES or digest(image) != info["sha256"]:
        raise RuntimeError("Bruce backup identity, size or SHA-256 mismatch")
    return image, info
