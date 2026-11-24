# Bruce recovery

Restore a Bruce backup only to the same physical Cardputer it was read from.
The backup may include credentials. Keep `backup/bruce/` private. The tools
also recognize an existing `../backup/bruce/` when the checkout has no local
backup directory.

`flash_backup.bin` is a complete 8,388,608-byte SPI flash image from offset zero.
The backup tool requires two matching reads and saves the image with mode 0400.
It records the digest in `flash_backup.sha256` and the device identity, flash,
security settings, and original partition map in `backup_info.json`.

Secure boot, flash encryption, and secure download mode were off when the dump
was taken. That makes a full plaintext image restore to this same chip feasible.
The restoration command checks the current security state and both hardware IDs
again. Restoration was deliberately not executed, so a restored Bruce boot has
not been exercised. Removable storage and eFuses are outside this SPI flash dump.

Before restoring, set the [authorized USB serial](testing.md#select-the-hardware-test-device).
From the repository root, substitute the path to your private backup directory:

```bash
backup_dir=backup/bruce
(cd "$backup_dir" && sha256sum -c flash_backup.sha256)
source tools/env.sh
python tools/flash.py --backup-dir "$backup_dir" --restore-bruce --confirm-restore
```

The script writes the entire 8 MiB image at `0x0`, preserves its flash header,
verifies the data against the saved file, and reboots. If automatic entry to
download mode fails, follow the official Cardputer-ADV G0 and USB instructions,
then run the same guarded procedure. If verification fails, preserve the backup
and log and investigate device access before retrying. [M5Stack download mode](https://docs.m5stack.com/en/core/Cardputer-Adv#download-mode)
