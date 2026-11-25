# Enigma M3 0.1.0 validation report

## Wi-Fi broadcast

On the physical Cardputer-ADV, the user confirmed that `BDZGO` appeared in
their phone’s Wi-Fi list and disappeared after **Stop Broadcasting**. The USB
hardware suite passed all 1,003 checks, including SSID length limits and
preservation of the message, rotor state, and undo history.

## Startup duration adjustment

After confirming the animation on the physical screen, the user requested one
additional second. The development build now runs the same animation for 2.6
seconds. The verified upload is recorded in `logs/flash-longer-intro.log`.
The device rendered 86 startup frames and passed all 937 hardware checks. See
`logs/hil-20261124T123414Z/`. Both native suites and all 84 Python tests passed.

## Vintage UI development baseline

The development application in `.pio/build/cardputer/firmware.bin` was uploaded
to the Cardputer-ADV and verified at `0x10000`. Its SHA-256 is
`79f87135e9695b865cdff8ed14b97e8934d9991066bb6a74fc826d4dddbecbe8`.
The uploader checked the Bruce backup, board identity, bootloader, and partition
table before writing the application. See `logs/flash-vintage.log`.

Both CTest executables and all 84 Python tests passed. The hardware suite passed
937 checks including the existing cipher tests, menu wraparound and selection,
reset, and exact undo. It measured 52 startup frames and captured each menu card
as well as the configuration, message, summary, and help screens. See
`logs/native-vintage.log`, `logs/build-vintage.log`, and
`logs/hil-20261124T114513Z/`. Physical inspection of this visual update is pending.

## Original UI baseline

The physical Cardputer-ADV booted the standalone firmware and passed the full USB
hardware suite and the user's physical display and keyboard checks. The tested
binary is the `firmware.bin` in the release directory.
Its SHA-256 is `9c6326ddfdab9a531e3bafbd83ed94a16a989d77a0a12bc28ca99a5e252ade41`.
The merged image embeds the same application bytes and matching boot files at
their verified offsets.

| Area | Result and evidence |
|---|---|
| Python | System Python 3.12.3 through the project `.venv`, managed by uv 0.9.18. `uv lock --check --offline` passed. PlatformIO 6.1.18, esptool 5.1.0, pyserial 3.5, pytest 8.4.2, Py-Enigma 1.0.2. |
| Embedded build | Espressif32 6.12.0, Arduino ESP32 2.0.17 (PlatformIO package `3.20017.241212+sha.dcc1105b`), Xtensa toolchain `8.4.0+2021r2-patch5`, M5Cardputer 1.1.1, M5Unified/M5GFX 0.3.0, IRremote 4.7.1, ArduinoJson 7.4.2. All build packages and caches are local to the project. |
| Device | ESP32-S3 QFN56 revision v0.2 in ROM diagnostics, USB Serial/JTAG `303a:1001`, Cardputer-ADV as detected by M5Unified, 8 MiB flash. Runtime reports 80 MHz QIO. |
| Bruce backup | Two separate 8,388,608-byte reads with matching SHA-256. The saved image passed an additional on-disk checksum check. Secure boot, flash encryption and secure download mode were off. See [recovery](recovery.md). |
| Native tests | Two passing CTest executables, including 287,041 cipher assertions and one UI suite. All 84 pytest cases passed, including 80 Py-Enigma comparisons and four serial transport cases. See `logs/native-reconnect.log`. |
| Firmware compile | PlatformIO completed successfully. It reported 24,100 bytes of static RAM and 550,553 bytes of application flash use. The binary file is 550,912 bytes. See `logs/build-usb-buffer.log`. |
| Upload | Three verified uploads after the Bruce backup. The third verified all four firmware components and rebooted the board. See `logs/flash-003.log`. |
| Hardware regression | 922 checks passed in 21.9 seconds against the installed release binary. The suite exercised historical vectors, 24 long independent reference comparisons, both reflectors and naval notches, malformed and oversize frames, UI state, undo, stability and serial reopen. See `logs/hil-20261123T162016Z/`. |
| USB reopen | Linux CDC's default paired DTR/RTS states preserved device uptime and rotor state. Uptime advanced from 123,380 to 123,404 ms across the reopen, with one processed letter and rotor windows AAB on both sides. The test then reset only the serial cipher. See the start of `logs/hil-20261123T162016Z/serial.jsonl`. |
| Display and keyboard | Hardware diagnostics reported 240 × 135 at rotation 0. Menu, setup, message, summary and help framebuffers were captured and inspected. The user confirmed the physical landscape orientation and keyboard behavior. See [manual validation](#manual-validation). |
| Release | The four individual flash components and merged image were inspected and their SHA-256 checks verified. See `releases/0.1.0/manifest.json`, `SHA256SUMS`, and [installation](flashing.md). |

The first HIL run reached a live firmware and stopped at the display geometry
assertion. Rotation 1 had selected a 135 × 240 panel direction while the canvas
used 240 × 135. The second run verified the rotation 0 fix and then exposed a
USB receive overflow. The Arduino default queue was 256 bytes, so a longer
request lost its newline and triggered the incomplete frame timeout. The third
build allocates 4096-byte USB queues and passed the complete suite.

A later diagnostic exposed host-side resets when the Python client deasserted
DTR and RTS one at a time. The client now leaves the native Linux CDC driver's
paired line states in place. One later regression run timed out on a framebuffer
row during capture. The test runner allows one logged retry per read-only row.
The final 922-check run completed without using that retry. The installed firmware
and release binaries are identical throughout these host-side changes.

The build logs include legacy esptool argument spelling warnings from the pinned
PlatformIO platform. Image generation succeeded. The original Cardputer model
uses the official library's detection and keyboard support, but this validation
used the Cardputer-ADV. The [web installer guide](web-installer.md#hardware-check-before-sharing)
records the separate physical installation test in Brave. M5Burner installation
has yet to be tested.

## Manual validation

The user performed these checks on the Cardputer-ADV running Enigma M3 0.1.0,
using the physical keyboard and reading the results on the built-in LCD.

| Check | Observed result |
|---|---|
| LCD orientation | Upright 240 × 135 landscape display. |
| Menu navigation | Pressing `.` followed by Enter opened the encryption screen. |
| Encryption and rotor stepping | Typing `AAAAA` produced `BDZGO` with rotor windows `AAF`. |
| Undo and rotor restoration | Pressing Backspace restored output `BDZG` and rotor windows `AAE`. |

All four checks passed as reported by the user.
