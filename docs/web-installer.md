# Enigma web installer

The installer uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/).
Visitors open the HTTPS page in a desktop browser with Web Serial, connect a Cardputer-ADV
with a USB-C data cable, click **Install Enigma**, choose their serial port, and
confirm. They do not need a local development environment or M5Burner.

## Build and preview locally

From the repository root on the documented Linux development environment, run:

```bash
bash tools/setup.sh
bash tools/build.sh
source tools/env.sh
python tools/build_installer.py
python3 -m http.server 8765 --bind 127.0.0.1 --directory dist
```

Open [the local installer](http://127.0.0.1:8765/installer/) in Brave. If Brave
blocks serial access, check **Serial ports** in the site's permission settings.
The server is loopback-only. Stop it with Ctrl+C when finished. ESP Web Tools
is pinned to 10.5.2 and loaded over HTTPS. The preview needs internet access
for that module. Its manifest, screenshot, and firmware are served locally.

If Ubuntu detects the Cardputer but the Brave Snap shows an empty port chooser,
check `snap connections brave`. If `brave:raw-usb` is disconnected, an
administrator can grant the Brave Snap USB access with
`sudo snap connect brave:raw-usb`. Fully quit and reopen Brave before retrying.
To revoke that access afterward, run `sudo snap disconnect brave:raw-usb`.
[Snap USB permission documentation](https://snapcraft.io/docs/reference/interfaces/raw-usb-interface/)

The installer uses the same packager as the release build. It creates a fresh
`dist/installer/` with relative URLs that also work under a GitHub Pages
project path. The build verifies the flash header, partition-table checksum,
non-overlapping offsets, merged padding and component bytes. The web manifest
selects the `ESP32-S3` family and writes a single merged image at `0x0`.

| Image component | Byte offset |
| --- | ---: |
| ESP32-S3 bootloader | `0x0000` |
| Partition table | `0x8000` |
| Boot selection data | `0xE000` |
| Enigma application | `0x10000` |

The boot header specifies 8 MiB flash and DIO at 80 MHz, matching the existing
Cardputer-ADV build. ESP Web Tools identifies the chip family. Check that the
physical board is a Cardputer-ADV before confirming installation.

The website artifact includes the merged build, an image checksum and layout
report, and the allowlisted page assets and notices. The merge reads four build
files, never a flash dump. The partition gap for user data is filled with erased
bytes. Installing can replace existing firmware and settings, even when full
flash erase is unchecked.

## Hardware check before sharing

Back up anything you want to keep. In the local preview, select the Cardputer-ADV
USB port, leave chip-wide erase unchecked if prompted, and confirm. Verify that
the browser reports success and the device boots into Enigma. With the default
configuration, encrypt `AAAAA` and check for `BDZGO` and rotor windows `AAF`.
If the board does not boot after a successful flash, reset it with G0 released.

The Cardputer owner confirmed that the local installer reported success in Brave
on Ubuntu and the Cardputer-ADV rebooted into Enigma after enabling the Brave
Snap's USB access. Automated checks verified the page, the unsupported-browser
message, and the firmware download checksum. A separate esptool test verified
the exact merged image on flash and confirmed that `AAAAA` produced `BDZGO` with
rotor windows `AAF`.

## Enable GitHub Pages when ready

1. Commit the installer source files and workflow and push them to the default
   branch. Git ignores `dist/`, so the generated image and site can stay local.
2. In the GitHub repository, open **Settings → Pages**. Under **Build and
   deployment**, set **Source** to **GitHub Actions**.
3. Go to **Actions → Publish Enigma installer → Run workflow**. Choose the
   default branch and start the run. Nothing deploys on a push by itself.
4. After the deployment job succeeds, use the URL shown for the `github-pages`
   environment. Check the page over HTTPS, then repeat the hardware check above
   before sharing the link.

The workflow builds the firmware from the selected commit and uploads only
`dist/installer/`. Check the rest of the source repository separately before
making it public, since GitHub repository visibility includes source files beyond
the Pages artifact.

[GitHub Pages publishing source guide](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
