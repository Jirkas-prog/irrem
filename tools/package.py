"""Package the already-built image; never accesses a serial port."""
import hashlib
import configparser
import json
import pathlib
import re
import shutil
import struct
import zipfile

root = pathlib.Path(__file__).resolve().parents[1]
build = root / '.pio/build/tdisplay'
version = re.search(r'constexpr char Version\[\]="([0-9.]+)"', (root/'src/main.cpp').read_text(encoding='utf-8')).group(1)
output = root.parent / ('releases/IRrem-' + version)
framework = pathlib.Path.home() / '.platformio/packages/framework-arduinoespressif32'
assert json.loads((framework / 'package.json').read_text())['version'].startswith('3.20017.'), 'Unexpected framework'

# Verify actual partition-table bytes before publishing offsets to the user.
partitions = {}
for offset in range(0, len((build/'partitions.bin').read_bytes()), 32):
    record = (build/'partitions.bin').read_bytes()[offset:offset+32]
    magic, kind, subtype, address, size, label, flags = struct.unpack('<HBBII16sI', record)
    if magic != 0x50AA:
        break
    partitions[label.rstrip(b'\0').decode()] = (address, size)
assert partitions['irrem_nvs'] == (0x10000, 0x20000)
assert partitions['factory'] == (0x30000, 0x300000)
config = configparser.ConfigParser()
config.read(root/'platformio.ini', encoding='utf-8')
# Arduino's uploader defaults to 0x10000 even with a different partition CSV.
assert int(config['env:tdisplay'].get('board_upload.offset_address', '0x10000'), 0) == partitions['factory'][0], 'Upload address does not match app partition'
assert partitions['otadata'] == (0xe000, 0x2000)
assert (build/'firmware.bin').stat().st_size < partitions['factory'][1]

output.mkdir(parents=True, exist_ok=True)
files = {
    'bootloader.bin': (build/'bootloader.bin', 0x1000),
    'partitions.bin': (build/'partitions.bin', 0x8000),
    'boot_app0.bin': (framework/'tools/partitions/boot_app0.bin', 0xe000),
    'firmware.bin': (build/'firmware.bin', 0x30000),
}
manifest = {'version':version, 'hardware_tested':False, 'chip':'esp32',
            'board':'original TTGO T-Display 135x240', 'flash_min_bytes':4*1024*1024,
            'files':{}, 'sources':{}}
for name, (source, offset) in files.items():
    shutil.copy2(source, output/name)
    data = (output/name).read_bytes()
    manifest['files'][name] = {'offset':hex(offset), 'bytes':len(data), 'sha256':hashlib.sha256(data).hexdigest()}

source_paths = [root/'platformio.ini', root/'partitions.csv', root/'README.md', root/'THIRD_PARTY.md', root/'home-assistant.yaml']
for directory in ['src','web','tests','tools','node-red']:
    source_paths.extend(p for p in (root/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
with zipfile.ZipFile(output/'source.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for source in source_paths:
        name = source.relative_to(root).as_posix()
        manifest['sources'][name] = hashlib.sha256(source.read_bytes()).hexdigest()
        archive.write(source,'ir-mqtt/'+name)
for name in ['README.md','THIRD_PARTY.md','home-assistant.yaml']:
    shutil.copy2(root/name,output/name)
(output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Verified partitions and packaged', output)
print('Application bytes:', (output/'firmware.bin').stat().st_size)
