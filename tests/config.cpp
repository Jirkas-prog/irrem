#define _CRT_SECURE_NO_WARNINGS
#include "../src/config.h"
#include <cassert>
#include <iostream>
#include <string>

static const char *apply(Settings &settings,const char *json) {
    DynamicJsonDocument doc(12000);
    assert(!deserializeJson(doc,json));
    return parseSettings(doc.as<JsonObjectConst>(),settings);
}
int main() {
    Settings s;
    assert(!settingsError(s) && !strcmp(s.hostname,"irrem"));
    assert(s.mqttEnabled && s.discoveryEnabled && !s.mqttTls);
    // Existing settings blobs retain their exact legacy ABI and values.
    struct OriginalSettings {
        uint32_t version;
        char name[49],ssid[33],wifiPassword[65],mqttHost[97],mqttUser[65],mqttPassword[97],ntp[97];
        uint16_t mqttPort,releaseMs,holdMs;
    };
    static_assert(sizeof(OriginalSettings)==sizeof(LegacySettings),"Legacy storage ABI changed");
    LegacySettings old;
    strcpy(old.name,"Původní IR"); strcpy(old.ssid,"Doma"); strcpy(old.wifiPassword,"old-wifi-pass");
    strcpy(old.mqttHost,"mqtt.home.arpa"); strcpy(old.mqttPassword,"mqtt-secret"); old.releaseMs=220;
    migrateSettings(old,s);
    assert(!settingsError(s) && s.version==2 && s.releaseMs==220);
    assert(!strcmp(s.ssid,"Doma") && !strcmp(s.hostname,"irrem") && !strcmp(s.mqttPassword,"mqtt-secret"));
    assert(!apply(s,R"({"name":"Nové IR","hostname":"irrem-patro","mqtt_host":"homeassistant.local","backup_host":"mqtt.example.com","backup_port":8883,"mqtt_client_id":"irrem-patro-01","mqtt_topic":"dum/patro/ir","keep_alive":60})"));
    assert(s.keepAlive==60 && s.backupPort==8883 && s.mqttPort==1883 && s.releaseMs==220);
    assert(!apply(s,R"({"wifi_password":"","mqtt_password":"","backup_password":"backup-secret"})"));
    assert(!strcmp(s.wifiPassword,"old-wifi-pass") && !strcmp(s.mqttPassword,"mqtt-secret"));
    assert(!apply(s,R"({"clear_mqtt_password":true,"clear_backup_password":true})"));
    assert(!s.mqttPassword[0] && !s.backupPassword[0] && s.wifiPassword[0]);
    assert(!apply(s,R"({"mqtt_enabled":false,"discovery_enabled":false,"birth_topic":""})"));
    assert(!s.mqttEnabled && !s.discoveryEnabled && !s.birthTopic[0]);
    assert(!apply(s,R"({"mqtt_topic":"","mqtt_client_id":""})"));
    for(const char *json:{
        R"({"hostname":"irrem.local"})",R"({"hostname":"IRrem"})",R"({"hostname":"-irrem"})",
        R"({"hostname":"irrem-"})",R"({"hostname":""})",R"({"hostname":123})",
        R"({"mqtt_host":"mqtt://homeassistant.local"})",R"({"mqtt_host":"mqtt.local:1883"})",
        R"({"mqtt_host":"bad..domain"})",R"({"mqtt_host":"x.-bad.com"})",
        R"({"backup_host":"homeassistant.local","backup_port":1883})",
        R"({"mqtt_host":""})",R"({"mqtt_topic":"test/#"})",R"({"mqtt_topic":"/irrem"})",
        R"({"mqtt_topic":"irrem/"})",R"({"discovery_prefix":"ha//test"})",
        R"({"discovery_prefix":""})",R"({"birth_topic":"test/+"})",
        R"({"mqtt_client_id":"two words"})",R"({"mqtt_port":0})",R"({"mqtt_port":65536})",
        R"({"mqtt_port":-1})",R"({"mqtt_port":"1883"})",R"({"release_ms":99})",
        R"({"keep_alive":4})",R"({"socket_timeout":16})",R"({"reconnect_seconds":1})",
        R"({"mqtt_enabled":"true"})",R"({"wifi_password":"short"})",
        R"({"mqtt_password":"bad\nsecret"})",R"({"clear_wifi_password":1})",
        R"({"name":"embedded\u0000tail"})",R"({"mqtt_tls":true})",R"({"backup_tls":true})"
    }) {
        Settings next=s;
        assert(apply(next,json)!=nullptr);
    }
    // PEM parsing/verification itself is supplied by mbedTLS in the firmware.
    assert(!apply(s,R"({"mqtt_tls":true,"mqtt_ca":"-----BEGIN CERTIFICATE-----\nTEST\n-----END CERTIFICATE-----"})"));
    assert(s.mqttTls);
    strcpy(s.mqttPassword,"never-return-this"); strcpy(s.backupPassword,"nor-this");
    DynamicJsonDocument doc(7168); publicSettings(doc,s);
    assert(!doc.overflowed() && doc["wifi_password_set"]==true && doc["mqtt_password_set"]==true);
    assert(!doc.containsKey("wifi_password") && !doc.containsKey("mqtt_password") && !doc.containsKey("backup_password"));
    std::string json; serializeJson(doc,json);
    assert(json.find("never-return-this")==std::string::npos && json.find("nor-this")==std::string::npos);
    assert(json.find("old-wifi-pass")==std::string::npos);
    std::cout<<"Settings: defaults, migration ABI, DNS/mDNS, topics, bounds, partial updates, secrets, TLS requirements OK\n";
}
