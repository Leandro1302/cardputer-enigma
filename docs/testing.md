# Tests

Run `bash tools/test_native.sh` to build the C++ library and tests with GCC and
CMake, run CTest, and run the independently checked cases under pytest. This
command needs the project-local Python environment and runs without a Cardputer.
The native tests cover rotor and reflector wiring, inverses, offsets, notches,
double-stepping, plugboard validation, invalid settings, reset, consecutive
messages, navigation, message limits, and undo across a double-step.

| Published fixture | Configuration left to right | Expected result |
|---|---|---|
| [Simple cipher examples](https://github.com/gremmie/enigma/blob/master/enigma/tests/test_enigma.py) | I II III, rings AAA, start AAA, B, empty plugboard | `AAAAA` → `BDZGO`. The tests also check decryption and the independent `HELLOWORLD` → `ILBDAAMTAZ` example. |
| [Double-stepping](https://www.ciphermachinesandcryptology.com/en/enigmatech.htm#steppingmechanism) | III II I, rings AAA, start KDO, B, empty plugboard | Six keypresses yield KDP, KDQ, KER, LFS, LFT, LFU. The separate rings BUL regression checks that ring offsets leave the turnover window letters unchanged. |
| [Scharnhorst intercept, adjusted key](https://www.bytereef.org/m4-project-scharnhorst-break.html) | III VI VIII, rings AHM, start UZV, B, AN EZ HK IJ LR MQ OT PV SW UX | Full intercept decodes to the published message. Enciphering the recovered text from the same starting state reproduces the intercept. |
| [Py-Enigma key-sheet example](https://py-enigma.readthedocs.io/en/latest/overview.html#quick-example) | II IV V, rings BUL, B, AV BS CG DL FU HZ IN KM OW RX | At WXC, `KCH` → `BLA`. At BLA, `NIBLFMYMLLUFWCASCSSNVHAZ` → `THEXRUSSIANSXAREXCOMINGX`. |

The wiring and turnover fixtures are cross-checked against the [technical wiring
tables](https://www.ciphermachinesandcryptology.com/en/enigmatech.htm) and
[Py-Enigma rotor definitions](https://github.com/gremmie/enigma/blob/master/enigma/rotors/data.py).
The 80 seeded host comparisons generate configurations across all eight rotors,
rings, initial positions, both reflectors and up to ten plugboard pairs. Each
compares 800 letters and the final rotor windows against Py-Enigma 1.0.2. Four
separate host tests cover fragmented and batched serial replies, the handshake
deadline and a timeout after a mutating request.

## Hardware regression

### Select the hardware test device

Connect one Cardputer and find the serial for USB device `303a:1001`:

```bash
source tools/env.sh
python -m serial.tools.list_ports -v
```

Create the ignored file `.config/enigma-cardputer/device.json` in the checkout.
Replace the example value with the serial shown for your Cardputer, keeping the
colons:

```json
{"serial": "00:00:00:00:00:00"}
```

`python tools/device_detect.py` checks that the tools can find that exact
device. Native tests and firmware builds can run without this file.

### Run the hardware regression suite

Close serial monitors before running the suite. It detects the authorized USB
serial and reconnects within a deadline. It exercises both the serial cipher and
the GUI using test commands. The GUI ends at the main menu with default settings.

```bash
source tools/env.sh
python tools/hil_test.py
```

The report, JSONL serial transcript, boot diagnostics and 240 × 135 captures are
in `logs/hil-<timestamp>/`. The suite checks identity, display geometry, published
vectors, both naval notches, invalid configurations and text, parser timeout and
oversize recovery, 24 independent 1,200-letter streams, screen rendering, exact
undo, the startup frame count, carousel wraparound and menu actions, 200 repeated
pings by default, heap stability, and serial reopen. The reopen
check verifies continuous uptime and the same non-default cipher state. Each
framebuffer row is a read-only request and can be retried once after a timeout.

## Physical check

With the keyboard below the LCD, the main menu should be upright and all five
items visible. Press `.` once and Enter to open the message screen. With the
default configuration, type `AAAAA` and check for `BDZGO` and `AAF`. Backspace
should show `BDZG` and `AAE`. Fn+Backspace should clear the message and show AAA.
The top-left backtick/ESC key should return to the menu. Framebuffer captures and
synthetic key actions do not confirm the physical orientation or key scanning.

The user confirmed landscape orientation, menu navigation, encryption, and
Backspace on the physical Cardputer-ADV. See the [recorded manual results](validation.md#manual-validation).
