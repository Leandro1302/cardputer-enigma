# Enigma web installer

[Open the Enigma web installer](https://leandro1302.github.io/cardputer-enigma/)
in a desktop browser with Web Serial. It uses ESP Web Tools to install the
standalone firmware directly over USB. See [Install Enigma on Cardputer-ADV](flashing.md)
for the step-by-step installation and manual flash instructions.

## Connection troubleshooting

If Brave blocks serial access, check **Serial ports** in the site's permission
settings. Close other programs using the device before opening the installer.

If Ubuntu detects the Cardputer but the Brave Snap shows an empty port chooser,
check `snap connections brave`. If `brave:raw-usb` is disconnected, an
administrator can grant the Brave Snap USB access with
`sudo snap connect brave:raw-usb`. Fully quit and reopen Brave before retrying.
To revoke that access afterward, run `sudo snap disconnect brave:raw-usb`.
[Snap USB permission documentation](https://snapcraft.io/docs/reference/interfaces/raw-usb-interface/)

## Build and preview locally

From the repository root on the documented Linux development environment, run:

```bash
bash tools/setup.sh
bash tools/build.sh
source tools/env.sh
python tools/build_installer.py
python3 -m http.server 8765 --bind 127.0.0.1 --directory dist
```

Open [the local preview](http://127.0.0.1:8765/installer/) in Brave. Stop the
loopback server with Ctrl+C when finished. The preview serves the manifest and
firmware locally and loads ESP Web Tools 10.5.2 over HTTPS.

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

Use the [installation steps](flashing.md#install-from-your-browser) to check that
the deployed installer reports success and the device boots into Enigma. With the
default configuration, encrypt `AAAAA` and check for `BDZGO` and rotor windows `AAF`.

The Cardputer owner confirmed that the local installer reported success in Brave
on Ubuntu and the Cardputer-ADV rebooted into Enigma after enabling the Brave
Snap's USB access. Automated checks verified the page, the unsupported-browser
message, and the firmware download checksum. A separate esptool test verified
the exact merged image on flash and confirmed that `AAAAA` produced `BDZGO` with
rotor windows `AAF`.

## Deploy an installer update

1. Push the source changes to `main`.
2. Open **Actions → Publish Enigma installer → Run workflow**, choose `main`,
   and start the run. Deployment starts when the workflow is run manually.
3. After the deployment job succeeds, open the URL for the `github-pages`
   environment and repeat the hardware check above.

For a new GitHub repository or fork, first set **Settings → Pages → Source** to
**GitHub Actions**. The workflow builds the selected commit and uploads only
`dist/installer/`. Keep generated copies of the site and firmware out of Git.

[GitHub Pages publishing source guide](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
