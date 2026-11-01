"""Exercise IRrem 0.3 persistence/validation without issuing lighting commands."""
import argparse
import json
import struct
import urllib.error
import urllib.request
import zlib
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--host', default='http://irrem')
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--old-mappings', type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)

def request(path, body=None, headers=None, expected=200):
    hdr = {'X-IRrem': '1'}
    if isinstance(body, dict):
        body = json.dumps(body).encode()
        hdr['Content-Type'] = 'application/json'
    elif body is not None:
        hdr['Content-Type'] = 'application/octet-stream'
    hdr.update(headers or {})
    req = urllib.request.Request(args.host + '/api/' + path, body, hdr)
    try:
        response = urllib.request.urlopen(req, timeout=20)
    except urllib.error.HTTPError as error:
        response = error
    data = response.read()
    assert response.status == expected, (path, response.status, data[:180])
    return json.loads(data) if 'json' in response.headers.get('Content-Type', '') else data

state = request('state')
assert state['version'] == '0.3.1' and state['mqtt'] and state['ota_available']
catalog = request('catalog')
settings = request('settings')
assert not any(k in settings for k in ('wifi_password', 'mqtt_password', 'backup_password'))
if args.old_mappings:
    old = json.loads(args.old_mappings.read_text(encoding='utf-8'))
    old = old['mappings'] if isinstance(old, dict) else old
    assert {(b['name'], b['key']) for b in old} == {(b['name'], b['key']) for b in catalog['buttons']}
    assert all(b['single'] for b in catalog['buttons']), 'Expected migrated actions on all existing named buttons'
print('Firmware, MQTT, OTA, catalog and all migrated IR keys OK')

backups = {}
for scope in ('buttons', 'system'):
    data = request('backup', {'scope': scope})
    magic, kind, length, crc = struct.unpack('<8sIII', data[:20])
    assert magic == b'IRREM03\0' and kind == (scope == 'system')
    assert length == len(data) - 20 and zlib.crc32(data[20:]) == crc
    args.output.joinpath('irrem-' + scope + '.irrem').write_bytes(data)
    preview = request('restore', data, {'X-IRrem-Preview': '1'})
    assert preview['scope'] == scope and preview['buttons'] == len(catalog['buttons'])
    assert preview['commands'] == len(catalog['commands'])
    damaged = bytearray(data)
    damaged[-1] ^= 1
    request('restore', bytes(damaged), {'X-IRrem-Preview': '1'}, 400)
    assert request('catalog') == catalog
    backups[scope] = data
    print(scope, 'backup CRC, preview and corrupt-file rejection OK:', len(data), 'bytes')

request('restore', backups['buttons'], {'X-IRrem-Revision': str(catalog['revision'] + 1)}, 409)
request('restore', backups['buttons'], {'X-IRrem-Revision': str(catalog['revision'])})
after = request('catalog')
assert after['revision'] == catalog['revision'] + 1
assert after['buttons'] == catalog['buttons'] and after['commands'] == catalog['commands']
settings_after = request('settings')
assert {k:v for k,v in settings.items() if k != 'revision'} == {k:v for k,v in settings_after.items() if k != 'revision'}
print('Actual catalog restore preserved every binding, IR key and network setting')

# Create a name-only button, then independently assign actions and clear them.
request('buttons', {'revision': after['revision'], 'id': 0, 'name': '__IRrem_verification__'})
temporary = request('catalog')
button = next(b for b in temporary['buttons'] if b['name'] == '__IRrem_verification__')
assert not button['key'] and not any(button[k] for k in ('single', 'double', 'long'))
request('buttons', {'revision': temporary['revision'], 'id': button['id'], 'name': button['name'], 'single': 1, 'double': 13, 'long': 14, 'long_ms': 1250})
temporary = request('catalog')
button = next(b for b in temporary['buttons'] if b['id'] == button['id'])
assert (button['single'], button['double'], button['long'], button['long_ms']) == (1, 13, 14, 1250)
request('buttons', {'revision': temporary['revision'], 'id': button['id'], 'name': button['name'], 'double': 0, 'long': 0})
temporary = request('catalog')
button = next(b for b in temporary['buttons'] if b['id'] == button['id'])
assert button['single'] == 1 and button['double'] == button['long'] == 0
request('restore', backups['buttons'], {'X-IRrem-Revision': str(temporary['revision'])})
after = request('catalog')
assert after['buttons'] == catalog['buttons'] and after['commands'] == catalog['commands']
print('Name-only creation, independent bindings, disabled double/long and cleanup OK')
args.output.joinpath('catalog.json').write_text(json.dumps(after, ensure_ascii=False, indent=2), encoding='utf-8')
print('Device persistence tests passed; no lighting commands were sent.')
