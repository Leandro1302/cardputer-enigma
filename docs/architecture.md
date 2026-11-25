# Architecture

`lib/enigma` is standard C++17 with no Arduino, display, USB, filesystem or network
dependency. `Config` validates distinct rotor IDs, rings/positions, wide B/C
reflectors and an involutive plugboard. Configuration changes are atomic. `Machine`
holds the visible positions and a 64-bit processed-letter count. Its character
operation streams indefinitely with constant state memory; string processing is
only a convenience wrapper.

Before every letter, the middle and right notch states are sampled together. The
left rotor advances when the middle rotor is at a notch; the middle advances when
either it or the right rotor is at a notch; the right always advances. The notch
test uses the **window letter**, independent of Ringstellung. The electrical path
uses the window-minus-ring offset in both forward and inverse transformations.
VI–VIII have both Z and M notches. Slot ordering is always left–middle–right.

`lib/app` is the portable UI state machine. It owns a cipher instance, configuration
draft, screens, messages and an undo history. Each accepted character stores its
complete prior machine state; backspace does not try to invert non-injective
double-stepping. A 1024-character limit bounds memory and reports an explicit notice.

`src/ui/Keyboard` adapts new physical key presses to abstract UI actions. It tracks
key coordinates, avoiding the official API's key-count-only change detection.
M5Cardputer selects a matrix or TCA8418 keyboard reader for the detected board.
`src/ui/Display` renders a 240×135 RGB565 sprite and transfers it when state changes.
M5GFX 0.3.0 incorporates the panel mounting offset; logical rotation **0** is the
normal Cardputer landscape orientation. Initialization verifies the actual dimensions.

`src/serial` parses bounded newline-delimited JSON and dispatches to the same cipher
library. It owns an independent machine state. GUI instrumentation calls the real
UI model and renderer; it does not duplicate GUI or encryption logic. A 4096-byte
USB RX queue accommodates complete 2048-byte frames. Parsing has an inactivity
timeout, and work is serviced incrementally alongside the keyboard/display loop.

The application does not wait for a serial host at boot. Speaker and microphone
are disabled. `src/wifi/WifiBroadcast` implements the broadcast interface used
by Message Actions. It configures one visible, open 2.4 GHz SoftAP using the
ciphertext as its exact SSID. The configuration stays in RAM. The UI stops the
radio when leaving the broadcast screen and preserves the message and undo history.

The display renderer uses the same sprite for the startup animation and the UI.
The 2.6-second startup animation draws three counter-rotating gears and reveals
the `En?gma` wordmark below them. Frame timing runs in the main loop so hardware
input and serial polling continue during startup. The carousel keeps the original
five menu actions and adds left and right navigation with wraparound. The Configure
card reuses the startup gear drawing routine.

The build uses ESP32-S3 DevKitC-1 N8 parameters as recommended by M5Stack, with
8 MB flash, 240 MHz CPU, 80 MHz QIO operation and no PSRAM requirement. Bootloader
and application image headers use DIO for initial loading; the framework initializes
QIO at runtime. A single factory application starts at 0x10000. There is no OTA UI.
