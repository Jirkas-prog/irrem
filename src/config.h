#pragma once
#include <ArduinoJson.h>
#include <ctype.h>
#include "remote.h"

// Keep this exact layout for migration from 0.1.x. Mapping storage is independent.
struct LegacySettings {
    uint32_t version=1;
    char name[49]="IRrem", ssid[33]={}, wifiPassword[65]={};
    char mqttHost[97]={}, mqttUser[65]={}, mqttPassword[97]={}, ntp[97]="pool.ntp.org";
    uint16_t mqttPort=1883, releaseMs=180, holdMs=100;
};
struct Settings : LegacySettings {
    char hostname[33]="irrem";
    char backupHost[97]={}, backupUser[65]={}, backupPassword[97]={};
    char mqttClientId[65]={}, mqttTopic[193]={};
    char discoveryPrefix[97]="homeassistant", birthTopic[129]="homeassistant/status";
    char mqttCa[2049]={}, backupCa[2049]={};
    uint16_t backupPort=1883, keepAlive=15, reconnectSeconds=5, socketTimeout=2;
    bool mqttEnabled=true, discoveryEnabled=true, mqttTls=false, backupTls=false;
    Settings() { version=2; }
};
template<size_t N> inline bool terminated(const char (&s)[N]) { return memchr(s,0,N)!=nullptr; }
inline bool copyString(JsonVariantConst value,char *dest,size_t size,bool multiline=false) {
    if(!value.is<const char*>()) return false;
    const char *s=value.as<const char*>(); size_t length=value.as<JsonString>().size();
    if(length!=strlen(s) || length>=size) return false;
    for(size_t i=0;i<length;i++)
        if(((unsigned char)s[i]<32 && !(multiline && (s[i]=='\r' || s[i]=='\n'))) || s[i]==127) return false;
    memcpy(dest,s,length+1); return true;
}
inline bool hostnameValid(const char *name) {
    size_t n=strlen(name);
    if(!n || n>32 || name[0]=='-' || name[n-1]=='-') return false;
    for(const char *p=name;*p;p++) if(!(*p>='a' && *p<='z') && !(*p>='0' && *p<='9') && *p!='-') return false;
    return true;
}
inline bool hostValid(const char *host) {
    // A DNS name or IPv4 literal; URLs, paths and embedded ports are not hosts.
    if(!*host) return true;
    unsigned label=0;
    for(const unsigned char *p=(const unsigned char*)host;*p;p++) {
        if(*p=='.') { if(!label || p[-1]=='-') return false; label=0; continue; }
        if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='-')) return false;
        if(!label && *p=='-') return false;
        if(++label>63) return false;
    }
    return label && host[strlen(host)-1]!='-';
}
inline bool topicValid(const char *topic,bool empty=false) {
    size_t n=strlen(topic); if(!n) return empty;
    if(topic[0]=='/' || topic[n-1]=='/') return false;
    for(size_t i=0;i<n;i++) if((unsigned char)topic[i]<33 || topic[i]=='#' || topic[i]=='+' ||
        (topic[i]=='/' && topic[i+1]=='/') || topic[i]==127) return false;
    return true;
}
inline bool legacyValid(const LegacySettings &s) {
    return (s.version==1 || s.version==2) && terminated(s.name) && remote::validName(s.name) &&
        terminated(s.ssid) && terminated(s.wifiPassword) && terminated(s.mqttHost) && terminated(s.mqttUser) &&
        terminated(s.mqttPassword) && terminated(s.ntp) && s.mqttPort &&
        s.releaseMs>=100 && s.releaseMs<=1000 && s.holdMs>=50 && s.holdMs<=1000;
}
inline const char *settingsError(const Settings &s) {
    if(!legacyValid(s) || s.version!=2 || !terminated(s.hostname) || !terminated(s.backupHost) ||
       !terminated(s.backupUser) || !terminated(s.backupPassword) || !terminated(s.mqttClientId) ||
       !terminated(s.mqttTopic) || !terminated(s.discoveryPrefix) || !terminated(s.birthTopic) ||
       !terminated(s.mqttCa) || !terminated(s.backupCa)) return "Neplatné nebo neúplné nastavení.";
    if(!hostnameValid(s.hostname)) return "Síťové jméno: 1–32 malých písmen, číslic nebo pomlček, bez .local.";
    if(!hostValid(s.mqttHost) || !hostValid(s.backupHost) || !hostValid(s.ntp) || !s.ntp[0])
        return "Zadejte DNS jméno nebo IPv4 adresu bez http:// a bez portu.";
    if(s.backupHost[0] && !s.mqttHost[0]) return "Záložní broker vyžaduje vyplněný hlavní broker.";
    if(s.backupHost[0] && !strcmp(s.mqttHost,s.backupHost) && s.mqttPort==s.backupPort)
        return "Hlavní a záložní broker mají být různé adresy nebo porty.";
    if(!topicValid(s.mqttTopic,true) || !topicValid(s.discoveryPrefix) || !topicValid(s.birthTopic,true))
        return "Témata nesmí obsahovat mezery, +, # ani prázdné úrovně /; prefix má být bez koncového /.";
    for(const unsigned char *p=(const unsigned char*)s.mqttClientId;*p;p++) if(*p<33 || *p==127)
        return "Klientské ID nesmí obsahovat mezery ani řídicí znaky.";
    if(!s.backupPort || s.keepAlive<5 || s.keepAlive>300 || s.reconnectSeconds<2 || s.reconnectSeconds>120 ||
       s.socketTimeout<1 || s.socketTimeout>15) return "Zkontrolujte port a intervaly MQTT.";
    size_t n=strlen(s.wifiPassword);
    if(n && (n<8 || n>63)) return "Heslo Wi-Fi musí mít 8–63 bajtů.";
    if((s.mqttTls && s.mqttHost[0] && (!strstr(s.mqttCa,"-----BEGIN CERTIFICATE-----") || !strstr(s.mqttCa,"-----END CERTIFICATE-----"))) ||
       (s.backupTls && s.backupHost[0] && (!strstr(s.backupCa,"-----BEGIN CERTIFICATE-----") || !strstr(s.backupCa,"-----END CERTIFICATE-----"))))
        return "Pro TLS vložte CA certifikát příslušného brokeru ve formátu PEM.";
    return nullptr;
}
inline void migrateSettings(const LegacySettings &old,Settings &next) {
    // Caller supplies a default Settings instance, avoiding a large stack temporary.
    static_cast<LegacySettings&>(next)=old; next.version=2;
}
inline const char *parseSettings(JsonObjectConst doc,Settings &next) {
    // Partial updates keep omitted fields; blank password inputs preserve stored secrets.
    struct Text { const char *key; char *dest; size_t size; bool lines; };
    Text fields[]={
        {"name",next.name,sizeof(next.name),false},{"ssid",next.ssid,sizeof(next.ssid),false},
        {"hostname",next.hostname,sizeof(next.hostname),false},{"ntp",next.ntp,sizeof(next.ntp),false},
        {"mqtt_host",next.mqttHost,sizeof(next.mqttHost),false},{"mqtt_user",next.mqttUser,sizeof(next.mqttUser),false},
        {"backup_host",next.backupHost,sizeof(next.backupHost),false},{"backup_user",next.backupUser,sizeof(next.backupUser),false},
        {"mqtt_client_id",next.mqttClientId,sizeof(next.mqttClientId),false},{"mqtt_topic",next.mqttTopic,sizeof(next.mqttTopic),false},
        {"discovery_prefix",next.discoveryPrefix,sizeof(next.discoveryPrefix),false},{"birth_topic",next.birthTopic,sizeof(next.birthTopic),false},
        {"mqtt_ca",next.mqttCa,sizeof(next.mqttCa),true},{"backup_ca",next.backupCa,sizeof(next.backupCa),true}
    };
    for(auto &f:fields) if(doc.containsKey(f.key) && !copyString(doc[f.key],f.dest,f.size,f.lines))
        return "Neplatný typ nebo příliš dlouhý text v nastavení.";
    struct Number { const char *key; uint16_t *dest; unsigned min,max; };
    Number numbers[]={
        {"mqtt_port",&next.mqttPort,1,65535},{"backup_port",&next.backupPort,1,65535},
        {"release_ms",&next.releaseMs,100,1000},{"hold_ms",&next.holdMs,50,1000},
        {"keep_alive",&next.keepAlive,5,300},{"reconnect_seconds",&next.reconnectSeconds,2,120},
        {"socket_timeout",&next.socketTimeout,1,15}
    };
    for(auto &f:numbers) if(doc.containsKey(f.key)) {
        if(!doc[f.key].is<unsigned>() || doc[f.key].as<unsigned>()<f.min || doc[f.key].as<unsigned>()>f.max)
            return "Číselná hodnota je mimo povolený rozsah.";
        *f.dest=static_cast<uint16_t>(doc[f.key].as<unsigned>());
    }
    struct Flag { const char *key; bool *dest; };
    Flag flags[]={{"mqtt_enabled",&next.mqttEnabled},{"discovery_enabled",&next.discoveryEnabled},
        {"mqtt_tls",&next.mqttTls},{"backup_tls",&next.backupTls}};
    for(auto &f:flags) if(doc.containsKey(f.key)) {
        if(!doc[f.key].is<bool>()) return "Neplatná hodnota přepínače.";
        *f.dest=doc[f.key].as<bool>();
    }
    struct Secret { const char *key,*clear; char *dest; size_t size; };
    Secret secrets[]={
        {"wifi_password","clear_wifi_password",next.wifiPassword,sizeof(next.wifiPassword)},
        {"mqtt_password","clear_mqtt_password",next.mqttPassword,sizeof(next.mqttPassword)},
        {"backup_password","clear_backup_password",next.backupPassword,sizeof(next.backupPassword)}
    };
    for(auto &f:secrets) {
        char value[97];
        if(doc.containsKey(f.key)) {
            if(!copyString(doc[f.key],value,f.size)) return "Neplatné heslo.";
            if(value[0]) snprintf(f.dest,f.size,"%s",value);
        }
        if(doc.containsKey(f.clear)) {
            if(!doc[f.clear].is<bool>()) return "Neplatný přepínač smazání hesla.";
            if(doc[f.clear].as<bool>()) f.dest[0]=0;
        }
    }
    return settingsError(next);
}
inline void publicSettings(JsonDocument &doc,const Settings &s) {
    doc["name"]=s.name; doc["hostname"]=s.hostname; doc["ssid"]=s.ssid; doc["ntp"]=s.ntp;
    doc["mqtt_host"]=s.mqttHost; doc["mqtt_port"]=s.mqttPort; doc["mqtt_user"]=s.mqttUser;
    doc["backup_host"]=s.backupHost; doc["backup_port"]=s.backupPort; doc["backup_user"]=s.backupUser;
    doc["mqtt_tls"]=s.mqttTls; doc["backup_tls"]=s.backupTls; doc["mqtt_ca"]=s.mqttCa; doc["backup_ca"]=s.backupCa;
    doc["mqtt_enabled"]=s.mqttEnabled; doc["discovery_enabled"]=s.discoveryEnabled;
    doc["mqtt_client_id"]=s.mqttClientId; doc["mqtt_topic"]=s.mqttTopic;
    doc["discovery_prefix"]=s.discoveryPrefix; doc["birth_topic"]=s.birthTopic;
    doc["keep_alive"]=s.keepAlive; doc["reconnect_seconds"]=s.reconnectSeconds; doc["socket_timeout"]=s.socketTimeout;
    doc["release_ms"]=s.releaseMs; doc["hold_ms"]=s.holdMs;
    doc["wifi_password_set"]=bool(s.wifiPassword[0]); doc["mqtt_password_set"]=bool(s.mqttPassword[0]);
    doc["backup_password_set"]=bool(s.backupPassword[0]);
}
