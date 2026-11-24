"""Device selection rules exercised without real USB access."""
import json
from pathlib import Path
import sys
from types import SimpleNamespace

import pytest
from serial.tools import list_ports

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import common


@pytest.fixture
def device_config(tmp_path, monkeypatch):
    monkeypatch.setattr(common, "ROOT", tmp_path)
    path = tmp_path / ".config/enigma-cardputer/device.json"
    path.parent.mkdir(parents=True)
    return path


def test_missing_identity_stops_hardware_detection(device_config, monkeypatch):
    monkeypatch.setattr(list_ports, "comports", lambda: pytest.fail("Inspected USB without an identity"))
    assert common.configured_serial(required=False) is None
    with pytest.raises(RuntimeError, match="Set the authorized Cardputer serial"):
        common.detect_device(timeout=0)


def test_invalid_identity_is_rejected(device_config):
    device_config.write_text(json.dumps({"serial": "not a device serial"}))
    with pytest.raises(RuntimeError, match="Expected a colon-separated Cardputer serial"):
        common.configured_serial(required=False)


def test_detection_selects_only_the_configured_espressif_device(device_config, monkeypatch):
    serial = "02:00:00:00:00:AB"
    device_config.write_text(json.dumps({"serial": serial.lower()}))
    other_board = SimpleNamespace(vid=0x303A, pid=0x1001, serial_number="02:00:00:00:00:CD")
    other_usb = SimpleNamespace(vid=0x1234, pid=0x1001, serial_number=serial)
    board = SimpleNamespace(vid=0x303A, pid=0x1001, serial_number=serial,
                            device="/dev/ttyACM999", location="test")
    monkeypatch.setattr(list_ports, "comports", lambda: [other_board, other_usb, board])
    assert common.detect_device(timeout=0)["device"] == board.device

    monkeypatch.setattr(list_ports, "comports", lambda: [board, board])
    with pytest.raises(RuntimeError, match="Ambiguous device identity"):
        common.detect_device(timeout=0)
