#!/usr/bin/env python3
"""Guarded firmware upload or explicit restoration to the authorized Cardputer."""
import argparse
import json
from pathlib import Path
from esptool import cmds
from esptool.bin_image import LoadFirmwareImage
from common import ROOT, FLASH_BYTES, digest, verified_backup, connect_verified

def release_components(directory):
    directory=directory.resolve()
    if not directory.is_relative_to(ROOT/"releases"):
        raise RuntimeError("Release must be inside this project's releases directory")
    m=json.loads((directory/"manifest.json").read_text())
    if m['chip']!='ESP32-S3' or m['flash_bytes']!=FLASH_BYTES or m['board']!='M5Stack Cardputer':
        raise RuntimeError("Release target mismatch")
    expected=[("bootloader.bin",0),("partitions.bin",0x8000),("boot_app0.bin",0xe000),("firmware.bin",0x10000)]
    if [(x['file'],x['offset']) for x in m['components']]!=expected:
        raise RuntimeError("Unexpected flash image layout")
    parts=[]
    previous_end=0
    for c in m['components']:
        p=directory/c['file']
        if p.stat().st_size!=c['size'] or digest(p)!=c['sha256'] or c['offset']<previous_end or c['offset']+c['size']>FLASH_BYTES:
            raise RuntimeError("Image checksum/size/offset mismatch")
        previous_end=c['offset']+c['size']
        if p.name in ('bootloader.bin','firmware.bin'):
            image=LoadFirmwareImage('esp32s3',str(p))
            if image.chip_id!=9 or image.flash_size_freq>>4!=3:
                raise RuntimeError("Image chip/flash header mismatch")
        parts.append((c['offset'],str(p)))
    return parts

def development_build():
    """Permit an app-only upload after checking the board's bootloader and layout."""
    build = ROOT / '.pio/build/cardputer'
    firmware = build / 'firmware.bin'
    app_offset = 0x10000
    image = LoadFirmwareImage('esp32s3', str(firmware))
    if image.chip_id != 9 or image.flash_size_freq >> 4 != 3 or firmware.stat().st_size > FLASH_BYTES - app_offset:
        raise RuntimeError('Development application does not fit the ESP32-S3 factory layout')
    boot = [(0, str(build / 'bootloader.bin')), (0x8000, str(build / 'partitions.bin'))]
    return [(app_offset, str(firmware))], boot

def main():
    parser=argparse.ArgumentParser()
    operation = parser.add_mutually_exclusive_group()
    operation.add_argument('--release',type=Path,default=ROOT/'releases/0.1.0')
    operation.add_argument('--build',action='store_true',help='Upload only the current development application')
    operation.add_argument('--restore-bruce',action='store_true')
    parser.add_argument('--backup-dir',type=Path,help='Directory containing the already-verified Bruce backup')
    parser.add_argument('--confirm-restore',action='store_true')
    args=parser.parse_args()
    backup,backup_info=verified_backup(args.backup_dir)
    preflight = []
    if args.restore_bruce:
        if not args.confirm_restore:
            raise RuntimeError("Restoration requires --confirm-restore; Enigma will be replaced with saved Bruce")
        parts=[(0,str(backup))]
    elif args.build:
        parts, preflight = development_build()
    else:parts=release_components(args.release)
    esp,info=connect_verified()
    try:
        if info['mac']!=backup_info['mac'] or info['flash_id']!=backup_info['flash_id']:
            raise RuntimeError("Physical chip/flash differs from Bruce backup")
        print(json.dumps(info,indent=2),flush=True)
        esp=esp.run_stub()
        esp.change_baud(460800)
        if preflight:
            cmds.verify_flash(esp,preflight,flash_size='keep',flash_mode='keep',flash_freq='keep')
        cmds.write_flash(esp,parts,flash_size='keep',flash_mode='keep',flash_freq='keep')
        cmds.verify_flash(esp,parts,flash_size='keep',flash_mode='keep',flash_freq='keep')
        print("FLASH CONTENTS VERIFIED",flush=True)
    finally:
        try:esp.hard_reset()
        finally:esp._port.close()

if __name__=='__main__':main()
