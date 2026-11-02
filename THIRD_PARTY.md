# Sources and dependencies

- Numeric presets in `src/profiles.h` are generated from
  [WLED v0.14.0, ir_codes.h](https://github.com/wled/WLED/blob/v0.14.0/wled00/ir_codes.h).
  The reference file is `tools/wled-ir-codes.reference.h`; its MIT license is
  in `tools/WLED-LICENSE.txt`. Copyright (c) 2016 Christian Schwinne.
  The generator selects tables and converts names to lowercase.
- [IRremoteESP8266 2.8.6](https://github.com/crankyoldgit/IRremoteESP8266/tree/v2.8.6):
  IR decoding and signal capture. Licenses and copyright notices for individual
  components are included in the library downloaded by PlatformIO.
- [ArduinoJson 6.21.5](https://github.com/bblanchon/ArduinoJson/tree/v6.21.5), MIT.
- [TFT_eSPI 2.5.43](https://github.com/Bodmer/TFT_eSPI/tree/v2.5.43), FreeBSD license.
- [PubSubClient 2.8](https://github.com/knolleary/pubsubclient/tree/v2.8), MIT.
- PlatformIO Espressif32 6.12.0, Arduino ESP32 2.0.17 / ESP-IDF 4.4.7.
- [Playwright](https://github.com/microsoft/playwright), Apache-2.0, used only for development browser tests. The exact version is pinned in `package-lock.json`.

The wiring and display type originated in the local reference project's
`gateway/main/board.h`; this firmware uses its own configuration and storage.

The Home Assistant protocol follows the official documentation for
[MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery),
[MQTT Event](https://www.home-assistant.io/integrations/event.mqtt/),
[MQTT Binary Sensor](https://www.home-assistant.io/integrations/binary_sensor.mqtt/)
and [MQTT triggers](https://www.home-assistant.io/docs/automation/trigger/#mqtt-trigger).
