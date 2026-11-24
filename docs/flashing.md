# Install Enigma on Cardputer-ADV

## Install from your browser

1. Back up the firmware and settings you want to keep. Installing Enigma can
   overwrite settings even with full flash erase unchecked.
2. Open the [Enigma web installer](https://leandro1302.github.io/cardputer-enigma/)
   in a desktop browser with Web Serial, such as Brave.
3. Connect the Cardputer-ADV with a USB-C data cable. Click **Install Enigma**,
   choose the **Espressif USB JTAG/serial debug unit** port, and confirm. Leave
   full flash erase unchecked if offered.
4. Keep USB connected until the installer reports success. Enigma should boot
   to the main menu. If it stays in download mode, reconnect USB with G0 released.

If the installer cannot connect, unplug USB, set the side switch to **OFF**,
and hold **G0** while reconnecting USB. Release **G0** once connected. The screen
may stay blank in download mode. [M5Stack download-mode instructions](https://docs.m5stack.com/en/core/Cardputer-Adv#download-mode)

If Ubuntu sees the Cardputer but the Brave Snap's port chooser is empty, see the
[Brave Snap USB permission instructions](web-installer.md#connection-troubleshooting).

## Flash a downloaded image manually

1. Download the merged `.bin` and its SHA-256 checksum from the installer
   footer. Verify the checksum before flashing.
2. Open an ESP32-S3 flasher such as the
   [Espressif browser flasher](https://espressif.github.io/esptool-js/) and
   connect to the Cardputer.
3. Select `enigma-cardputer-merged.bin` as the only file. Set the address to
   **`0x0`**, keep the image's flash settings, and leave full flash erase
   unchecked. Start programming and wait for verification. Reset the
   Cardputer if it stays in download mode.

| Component | Flash address |
|---|---|
| `bootloader.bin` | `0x0000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |
| `enigma-cardputer-merged.bin` | `0x0000`, instead of the four components |

## Developer builds and uploads

From the project directory, use `bash tools/build.sh`, source `tools/env.sh`,
then run `python tools/package.py` to regenerate the release. The packaging script
compares each binary at its exact offset in the merged image and records SHA-256
digests. `python tools/flash.py` is the developer entry point. It requires a
[locally selected device](testing.md#select-the-hardware-test-device) and checks
the Bruce backup, release, target MAC, flash ID, security state and write readback.

For a future M5Burner submission, supply the merged application image at 0x0 and
select the matching device entry. Verify the upload format and run an install
through M5Burner against the physical board before listing it in the catalog.
