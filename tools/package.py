#!/usr/bin/env python3
"""Package and inspect the exact ESP32-S3 build; no device access."""
import json
import hashlib
import shutil
import struct
import subprocess
import sys
from esptool.bin_image import LoadFirmwareImage
from common import ROOT, FLASH_BYTES, digest, local_environment

VERSION = "0.1.0"
MERGED_NAME = "enigma-cardputer-merged.bin"

def package_firmware(out):
    """Merge build products and verify the ESP32-S3 image, without accessing a device."""
    build=ROOT/".pio/build/cardputer"
    sources=[(0,build/"bootloader.bin"),(0x8000,build/"partitions.bin"),
             (0xe000,ROOT/".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"),
             (0x10000,build/"firmware.bin")]
    partitions=[]
    data=sources[1][1].read_bytes()
    for n in range(0,len(data),32):
        entry=data[n:n+32]
        if entry[:2]!=b"\xaa\x50":break
        _,kind,subtype,offset,size,label,flags=struct.unpack("<HBBII16sI",entry)
        partitions.append(dict(type=kind,subtype=subtype,offset=offset,size=size,label=label.split(b"\0")[0].decode(),flags=flags))
    expected_layout = [(1, 2, 0x9000, 0x5000), (1, 0, 0xe000, 0x2000), (0, 0, 0x10000, 0x7f0000)]
    actual_layout = [(p['type'], p['subtype'], p['offset'], p['size']) for p in partitions]
    if actual_layout != expected_layout:
        raise RuntimeError('Unexpected partition table layout')
    if entry[:16] != b'\xeb\xeb' + b'\xff' * 14 or entry[16:] != hashlib.md5(data[:n]).digest():
        raise RuntimeError('Invalid partition table checksum')
    if any(partition['flags'] for partition in partitions):
        raise RuntimeError('Unexpected factory partition flags')
    for (offset, source), (limit, _) in zip(sources, sources[1:] + [(FLASH_BYTES, None)]):
        if not 0 < source.stat().st_size <= limit - offset:
            raise RuntimeError(f'{source.name} is empty or exceeds its flash region')
    image_details={}
    for name in ["bootloader.bin","firmware.bin"]:
        image=LoadFirmwareImage("esp32s3",str(build/name))
        if image.chip_id != 9 or image.flash_size_freq >> 4 != 3:
            raise RuntimeError(f'{name}: expected ESP32-S3 with 8 MB flash')
        if image.flash_mode != 2 or image.flash_size_freq & 0xf != 0xf:
            raise RuntimeError(f'{name}: expected DIO boot mode at 80 MHz')
        image_details[name]={"chip_id":image.chip_id,"flash_mode_code":image.flash_mode,"flash_size_freq":image.flash_size_freq}
    out.mkdir(parents=True,exist_ok=True)
    components=[]
    for offset,source in sources:
        target=out/source.name
        shutil.copyfile(source,target)
        components.append(dict(file=target.name,offset=offset,size=target.stat().st_size,sha256=digest(target)))
    merged=out/MERGED_NAME
    args=[sys.executable,"-m","esptool","--chip","esp32s3","merge-bin","-o",str(merged)]
    for item in components:args += [hex(item['offset']),str(out/item['file'])]
    subprocess.run(args,check=True,cwd=ROOT,env=local_environment(),timeout=30)
    merged_data=merged.read_bytes()
    previous_end=0
    for item in components:
        gap=merged_data[previous_end:item['offset']]
        if gap != b'\xff' * len(gap):
            raise RuntimeError(f"Merged image has unexpected data before {item['file']}")
        if merged_data[item['offset']:item['offset']+item['size']] != (out/item['file']).read_bytes():
            raise RuntimeError(f"Merged image has incorrect data at {item['offset']:#x}")
        previous_end=item['offset']+item['size']
    if len(merged_data)!=previous_end or len(merged_data)>FLASH_BYTES:
        raise RuntimeError('Unexpected merged image length')
    manifest={"firmware":"enigma-cardputer","version":VERSION,"chip":"ESP32-S3","board":"M5Stack Cardputer",
              "flash_bytes":FLASH_BYTES,"merged_offset":0,"merged_file":merged.name,"merged_sha256":digest(merged),
              "components":components,"partitions":partitions,"images":image_details}
    (out/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    return manifest

def main():
    out=ROOT/"releases"/VERSION
    manifest=package_firmware(out)
    (out/"INSTALL.txt").write_text("Enigma M3 0.1.0 — M5Stack Cardputer / ESP32-S3 / 8 MB\n"
        "Standalone merged image: enigma-cardputer-merged.bin, flash at 0x0.\n"
        "Application-only firmware.bin belongs at 0x10000 and requires the matching bootloader and partition table.\n"
        "Verified on Cardputer-ADV. Back up existing firmware and verify SHA256SUMS first.\n"
        "Browser flasher: https://espressif.github.io/esptool-js/ (Chrome or Edge).\n"
        "Select the Cardputer port and program only the merged image at 0x0.\n"
        "No development tools or MicroHydra are required on the Cardputer.\n")
    for name in ('CHANGELOG.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md'):
        shutil.copyfile(ROOT/name,out/name)
    files=[out/i['file'] for i in manifest['components']]+[out/MERGED_NAME,out/"manifest.json",out/"INSTALL.txt",
                                              out/"CHANGELOG.md",out/"LICENSE",out/"THIRD_PARTY_NOTICES.md"]
    (out/"SHA256SUMS").write_text(''.join(f'{digest(p)}  {p.name}\n' for p in files))
    print(json.dumps(manifest,indent=2))

if __name__=="__main__":main()
