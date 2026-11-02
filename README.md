# IRrem

An ESP32 infrared remote bridge with a local web interface, virtual buttons and MQTT integration for Home Assistant and Node-RED. Hardware: the original TTGO T-Display.

## English and Czech

The web interface supports **EN/CZ**. Select a language in the top bar; the choice is saved in the browser and restored on the next visit. A first visit uses Czech for a Czech browser and English otherwise. Switching also works when browser storage is unavailable.

Button names, command names, entered settings and passwords remain unchanged when switching languages. The language choice is local to the browser. Menus, status messages, diagnostics, configuration errors and firmware-update messages use the selected language.

## Build and configuration

Build firmware 0.3.2 with PlatformIO:

```sh
pio run -e tdisplay
```

Configure Wi-Fi and MQTT at runtime in the web interface. The fallback access point is open; use the device on a trusted local network.

## Tests

On Windows with Visual Studio C++ tools, run `tools/test-core.cmd` for the five host test suites. The browser checks use a local simulated device and require no ESP32 connection:

```sh
npm ci
npx playwright install chromium
npm run test:web
```

For manual interface testing, `node tests/web.cjs` starts the same local fixture. See [VALIDATION.md](VALIDATION.md) for completed checks and their limits.

## Repository contents

Source code, build configuration and tests. Keep credentials, installation records, device backups and compiled firmware outside Git. Fill in your own settings locally. Dependencies and attribution are listed in [THIRD_PARTY.md](THIRD_PARTY.md).

[Jirkas-prog project directory](https://github.com/Jirkas-prog) · [Home Assistant add-ons](https://github.com/Jirkas-prog/home-assistant-addons)
