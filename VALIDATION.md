# Validation

## English/Czech web interface — November 2, 2026

- Firmware 0.3.2: `pio run -e tdisplay` passed, including the embedded translation asset and HTTP route.
- `tools/test-core.cmd`: all five host suites passed (IR engine, configuration, gestures, display and Wi-Fi fallback).
- `npm run test:web`: six Chromium tests passed against the local simulated device. They cover switching both ways, preserving draft settings and user names, avoiding configuration writes, restoring the language after reload, Czech browser preference, unavailable storage, backend errors, mobile access and correct offline status after switching.
- Node.js syntax checks for the web scripts passed.

No physical device was flashed or tested during this change. A successful build and simulated browser tests do not establish hardware or live MQTT behavior. Compiled binaries are not included in this source update.

## Initial source import — November 1, 2026

All five Windows / MSVC 2022 host suites passed. Gitleaks 8.30.1 found no secrets in the exported source; examples used placeholders. A complete firmware build, board installation and tests connected to real hardware were not performed during that initial import.
