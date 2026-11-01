"""Regenerate numeric preset data from the checked-in WLED v0.14.0 reference (MIT)."""
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]
source = (root / "tools/wled-ir-codes.reference.h").read_text(encoding="utf-8")
groups = [
    ("IR24", "RGB 24 tlačítek", "rgb24"),
    ("IR24_OLD", "RGB 24 tlačítek – starší kódy", "rgb24old"),
    ("IR24_CT", "RGB / teplá a studená bílá 24 tlačítek", "rgb24ct"),
    ("IR44", "RGB 44 tlačítek / DIY", "rgb44"),
    ("IR40", "RGBW 40 tlačítek", "rgb40"),
    ("IR21", "RGB 21 tlačítek", "rgb21"),
    ("IR9", "Mini 9 tlačítek", "mini9"),
    ("IR6", "Učící ovladač 6 tlačítek", "mini6"),
]
output = ['#pragma once', '#include <stdint.h>',
          '// Numeric IR facts from WLED ir_codes.h; see THIRD_PARTY.md.',
          'struct PresetKey { const char *name; uint32_t code; };',
          'struct Profile { const char *name,*prefix; const PresetKey *keys; unsigned count; };']
for prefix, title, slug in groups:
    entries = re.findall(r'^#define '+prefix+r'_([A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+)', source, re.M)
    if prefix == 'IR24':
        entries = [(name, code) for name, code in entries if not name.startswith(('OLD_', 'CT_'))]
    expected = {'IR24':24, 'IR24_OLD':24, 'IR24_CT':24, 'IR44':44, 'IR40':40, 'IR21':21, 'IR9':9, 'IR6':6}
    assert len(entries) == expected[prefix], (prefix, len(entries))
    assert len({code for name, code in entries}) == len(entries), prefix
    output.append(f'static const PresetKey {slug}[]={{')
    output.extend(f'    {{"{name.lower()}",{code}}},' for name, code in entries)
    output.append('};')
output.append('static const Profile profiles[]={')
for prefix, title, slug in groups:
    output.append(f'    {{"{title}","{slug}",{slug},sizeof({slug})/sizeof({slug}[0])}},')
output.extend(['};', 'constexpr unsigned ProfileCount=sizeof(profiles)/sizeof(profiles[0]);', ''])
(root / 'src/profiles.h').write_text('\n'.join(output), encoding='utf-8')
print('Generated', len(groups), 'profiles')
