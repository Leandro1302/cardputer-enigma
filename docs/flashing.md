# Install over USB

Use the standalone `enigma-cardputer-merged.bin` for an 8 MB ESP32-S3 Cardputer.
The directly tested model is the Cardputer-ADV. Verify the release SHA-256 and
back up the current firmware and settings before installation. A merged image
writes padding between components, including areas used by prior firmware.

The [Enigma web installer](web-installer.md) selects the merged image and offset
automatically and was tested on a physical Cardputer-ADV in Brave. To install a
downloaded image manually, use the following steps.

1. Open the [Espressif browser flasher](https://espressif.github.io/esptool-js/)
   in a Web Serial compatible browser such as Chrome or Edge.
2. Connect the Cardputer with a USB-C data cable and select its serial port. If
   automatic connection fails, set the side switch to OFF, unplug USB, hold G0
   while reconnecting USB, and release G0 when the device powers on. [M5Stack download-mode instructions](https://docs.m5stack.com/en/core/Cardputer-Adv#download-mode)
3. Select the merged image as the only file at address **0x0**. Keep the flash
   settings from the image and leave flash-wide erase unchecked. Start programming.
4. After the tool reports success, reset the Cardputer if it does not restart by
   itself. Enigma opens directly to the main menu.

| Component | Flash address |
|---|---|
| `bootloader.bin` | `0x0000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |
| `enigma-cardputer-merged.bin` | `0x0000`, instead of the four components |

From the project directory, use `bash tools/build.sh`, source `tools/env.sh`,
then run `python tools/package.py` to regenerate the release. The packaging script
compares each binary at its exact offset in the merged image and records SHA-256
digests. `python tools/flash.py` is the developer entry point. It requires a
[locally selected device](testing.md#select-the-hardware-test-device) and checks
the Bruce backup, release, target MAC, flash ID, security state and write readback.

For a future M5Burner submission, supply the merged application image at 0x0 and
select the matching device entry. Verify the upload format and run an install
through M5Burner against the physical board before listing it in the catalog.
