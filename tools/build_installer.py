#!/usr/bin/env python3
"""Stage the static installer and the exact, verified firmware it serves."""
import json
from pathlib import Path
import shutil
from tempfile import TemporaryDirectory

from common import ROOT, configured_serial, digest
from package import MERGED_NAME, package_firmware

FIRMWARE_PATH = f'firmware/{MERGED_NAME}'
ASSETS = {
    'index.html': 'installer/index.html',
    'installer.css': 'installer/installer.css',
    'installer.js': 'installer/installer.js',
    'images/enigma-menu.png': 'docs/images/menu.png',
    'LICENSE.txt': 'LICENSE',
    'THIRD_PARTY_NOTICES.txt': 'THIRD_PARTY_NOTICES.md',
}


def installer_manifest(version):
    return {
        'name': 'Enigma M3',
        'version': version,
        # Let the visitor leave chip-wide erase unchecked.
        'new_install_prompt_erase': True,
        'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': 'ESP32-S3', 'parts': [{'path': FIRMWARE_PATH, 'offset': 0}]}],
    }


def check_payload(site, flash_manifest):
    expected = set(ASSETS) | {'manifest.json', 'firmware-layout.json', 'SHA256SUMS', FIRMWARE_PATH}
    files = {path.relative_to(site).as_posix() for path in site.rglob('*') if path.is_file()}
    if files != expected:
        raise RuntimeError(f'Unexpected installer files: {sorted(files ^ expected)}')
    if digest(site / FIRMWARE_PATH) != flash_manifest['merged_sha256']:
        raise RuntimeError('The website firmware differs from the verified build')
    forbidden = [str(Path.home()).encode().lower()]
    if serial := configured_serial(required=False):
        forbidden.extend(serial.replace(':', separator).encode() for separator in (':', '-', ''))
    for name in files:
        content = (site / name).read_bytes().lower()
        if any(token in content for token in forbidden):
            raise RuntimeError(f'Local device identifier or user path in installer file: {name}')


def build_installer():
    output = ROOT / 'dist' / 'installer'
    output.parent.mkdir(exist_ok=True)
    with TemporaryDirectory(prefix='.installer-', dir=output.parent) as temp:
        work = Path(temp)
        bundle = work / 'bundle'
        flash_manifest = package_firmware(bundle)
        site = work / 'site'
        for name, source in ASSETS.items():
            destination = site / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / source, destination)
        merged = site / FIRMWARE_PATH
        merged.parent.mkdir()
        shutil.copyfile(bundle / MERGED_NAME, merged)

        manifest = installer_manifest(flash_manifest['version'])
        (site / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        (site / 'firmware-layout.json').write_text(
            json.dumps({**flash_manifest, 'merged_file': FIRMWARE_PATH}, indent=2) + '\n', encoding='utf-8'
        )
        (site / 'SHA256SUMS').write_text(
            f"{flash_manifest['merged_sha256']}  {FIRMWARE_PATH}\n", encoding='ascii'
        )
        check_payload(site, flash_manifest)
        if output.exists():
            # Replace the generated artifact only once the new payload is verified.
            output.rename(work / 'previous')
        site.rename(output)

    print(f'Installer is ready at {output}')
    print(f"Firmware SHA-256: {flash_manifest['merged_sha256']}")


if __name__ == '__main__':
    build_installer()
