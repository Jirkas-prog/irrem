# Zdroje a závislosti

- Číselné předvolby v `src/profiles.h` se generují z
  [WLED v0.14.0, ir_codes.h](https://github.com/wled/WLED/blob/v0.14.0/wled00/ir_codes.h).
  Referenční soubor je v `tools/wled-ir-codes.reference.h`, jeho MIT licence
  v `tools/WLED-LICENSE.txt`. Copyright (c) 2016 Christian Schwinne.
  Generátor pouze vybírá tabulky a upravuje názvy na malá písmena.
- [IRremoteESP8266 2.8.6](https://github.com/crankyoldgit/IRremoteESP8266/tree/v2.8.6):
  dekódování IR a záznam průběhu. Licence a copyright jednotlivých částí jsou
  součástí knihovny stažené PlatformIO.
- [ArduinoJson 6.21.5](https://github.com/bblanchon/ArduinoJson/tree/v6.21.5), MIT.
- [TFT_eSPI 2.5.43](https://github.com/Bodmer/TFT_eSPI/tree/v2.5.43), FreeBSD licence.
- [PubSubClient 2.8](https://github.com/knolleary/pubsubclient/tree/v2.8), MIT.
- PlatformIO Espressif32 6.12.0, Arduino ESP32 2.0.17 / ESP-IDF 4.4.7.

Zapojení a typ displeje byly převzaty z místního referenčního projektu
`gateway/main/board.h`; nový firmware používá vlastní konfiguraci a úložiště.

Protokol Home Assistantu vychází z oficiální dokumentace
[MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery),
[MQTT Event](https://www.home-assistant.io/integrations/event.mqtt/),
[MQTT Binary Sensor](https://www.home-assistant.io/integrations/binary_sensor.mqtt/)
a [MQTT triggerů](https://www.home-assistant.io/docs/automation/trigger/#mqtt-trigger).
