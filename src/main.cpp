#include <Arduino.h>
#include <memory>
#include <atomic>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/x509_crt.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <esp_http_server.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <nvs_flash.h>
#include <sys/time.h>
#include "remote.h"
#include "profiles.h"
#include "config.h"
#include "display.h"
#include "gestures.h"

using namespace remote;
constexpr char Version[]="0.3.1";
constexpr int IrPin=25, IrGround=26, IrPower=27, ScreenButton=35, SetupButton=0;
constexpr uint32_t ConfigVersion=1, LearnTimeout=30000;
struct LegacyMapStore { uint32_t version=ConfigVersion; unsigned count=0; Mapping items[MaxKeys]; };
struct Packet {
    Event event; char timestamp[32]; uint32_t connection;
    uint32_t buttonId=0,commandId=0; char code[97]={},commandName[49]={},gesture[8]={};
    unsigned steps=1; bool virtualInput=false;
};
static Settings settings;
static control::Store model;
static SemaphoreHandle_t mutex;
static QueueHandle_t eventQueue;
struct Guard {
    Guard(){ xSemaphoreTake(mutex,portMAX_DELAY); }
    ~Guard(){ xSemaphoreGive(mutex); }
};
static Preferences prefs;
static String deviceId, baseTopic, apName;
static char bootId[9];
static bool learning=false, learnWait=false, suppress=false, candidateValid=false;
static Signal candidate;
static uint32_t learnStarted=0,candidateId=0,lastRx=0;
static bool receivedAny=false, mqttOnline=false;
static uint32_t mqttConnection=0, droppedEvents=0, invalidFrames=0, rebootAt=0, commandSequence=0;
static char lastKey[KeySize]={}, lastButton[NameSize]={}, lastTrigger[32]={}, lastLearn[96]={};
static int mqttError=-1;
static unsigned mqttBroker=0;
static uint32_t mqttAttempts=0, sentEvents=0, receivedFrames=0;
static char mqttDetail[160]="Čekám na síť", brokerIp[16]={};
static std::atomic<bool> reconnectRequested{false}, cleanupRequested{false};
static Packet recentEvents[12];
static unsigned recentCount=0, recentNext=0;
static std::atomic<bool> apActive{false};
static bool displayOn=true;
static TFT_eSPI lcd;
static StatusDisplay statusDisplay(lcd);
static IRrecv receiver(IrPin,1024,15,true);
static void isoTime(char *dest,size_t size) {
    timeval tv{}; gettimeofday(&tv,nullptr);
    if(tv.tv_sec<1700000000){ dest[0]=0; return; }
    tm utc{}; gmtime_r(&tv.tv_sec,&utc);
    char date[24]; strftime(date,sizeof(date),"%Y-%m-%dT%H:%M:%S",&utc);
    snprintf(dest,size,"%s.%03ldZ",date,long(tv.tv_usec/1000));
}
static bool activeAt(uint32_t now) { return receivedAny && now-lastRx<settings.releaseMs; }
static void onCommand(const control::Button &button,uint32_t commandId,control::Gesture gesture,
                      uint32_t press,uint32_t duration,unsigned steps,bool virtualInput,void*) {
    const auto *command=control::command(model.catalog,commandId); if(!command) return;
    Packet packet{}; packet.connection=mqttConnection; packet.buttonId=button.id;packet.commandId=commandId;
    packet.steps=steps;packet.virtualInput=virtualInput;
    snprintf(packet.code,sizeof(packet.code),"%s",command->code);
    snprintf(packet.commandName,sizeof(packet.commandName),"%s",command->name);
    snprintf(packet.gesture,sizeof(packet.gesture),"%s",control::gestureName(gesture));
    auto &event=packet.event;event.action=Action::Press;event.at=millis();event.pressId=press;
    event.sequence=++commandSequence;event.duration=duration;
    snprintf(event.name,sizeof(event.name),"%s",button.name);
    snprintf(event.key,sizeof(event.key),"%s",button.signal.key);
    isoTime(packet.timestamp,sizeof(packet.timestamp));
    recentEvents[recentNext]=packet; recentNext=(recentNext+1)%12; if(recentCount<12)++recentCount;
    snprintf(lastButton,sizeof(lastButton),"%s",button.name);snprintf(lastTrigger,sizeof(lastTrigger),"%s",packet.timestamp);
    if(!mqttOnline || xQueueSend(eventQueue,&packet,0)!=pdTRUE) ++droppedEvents;
}
static control::Gestures irGestures(onCommand), virtualGestures(onCommand,true);
static uint32_t virtualToken=0,virtualStarted=0,virtualSeen=0,virtualPress=0;
static bool virtualActive=false;
static bool repeatable(uint32_t id) {
    const auto *c=control::command(model.catalog,id);return c && !strncmp(c->code,"dim/",4);
}
static void cancelGestures() { irGestures.cancel();virtualGestures.cancel();virtualActive=false; }
static void onEvent(const Event &event,void*) {
    if(event.action==Action::Press) {
        for(unsigned i=0;i<model.catalog.count;i++) if(!strcmp(model.catalog.items[i].name,event.name)) {
            const auto &b=model.catalog.items[i];irGestures.start(b,event.pressId,event.at,repeatable(b.hold));break;
        }
    } else if(event.action==Action::Hold) irGestures.sample(event.duration);
    else irGestures.finish(event.duration,event.at);
}
static Engine engine(onEvent);
static bool makeSignal(const decode_results &result,Signal &s) {
    if(result.overflow) return false;
    String protocol=typeToString(result.decode_type);
    if(result.repeat && result.bits==0) {
        if(!engine.active) { s.repeat=true; return true; }
        String prefix=protocol+":";
        if(strncmp(engine.signal.key,prefix.c_str(),prefix.length())) { s.repeat=true; return true; }
        s=engine.signal; s.repeat=true; return true;
    }
    if(result.decode_type==UNKNOWN) {
        if(result.rawlen<13 || result.rawlen-1>MaxRaw) return false;
        s.count=result.rawlen-1;
        for(unsigned i=0;i<s.count;i++) s.raw[i]=result.rawbuf[i+1]*kRawTick;
        snprintf(s.key,sizeof(s.key),"RAW:%u:%08lX",s.count,(unsigned long)result.value);
    } else if(hasACState(result.decode_type)) {
        unsigned bytes=(result.bits+7)/8;
        if(bytes>kStateSizeMax || protocol.length()+12+bytes*2>=sizeof(s.key)) return false;
        snprintf(s.key,sizeof(s.key),"%s:%u:",protocol.c_str(),result.bits);
        size_t offset=strlen(s.key);
        for(unsigned i=0;i<bytes;i++) snprintf(s.key+offset+2*i,3,"%02X",result.state[i]);
    } else {
        uint64_t value=result.value;
        if(result.decode_type==RC5 || result.decode_type==RC5X) {
            s.toggleValid=true; s.toggle=(value & (1ULL<<11)); value &= ~(1ULL<<11);
        } else if(result.decode_type==RC6 && (result.bits==20 || result.bits==36)) {
            uint64_t mask=1ULL<<(result.bits==20?16:15);
            s.toggleValid=true; s.toggle=(value & mask); value &= ~mask;
        }
        snprintf(s.key,sizeof(s.key),"%s:%u:%016llX",protocol.c_str(),result.bits,(unsigned long long)value);
    }
    // Full frames marked repeat by a protocol still carry their own identity.
    return true;
}
static void irTask(void*) {
    receiver.setUnknownThreshold(12); receiver.enableIRIn(true);
    decode_results result;
    for(;;) {
        if(receiver.decode(&result)) {
            Signal signal;
            {
                Guard guard;
                uint32_t now=millis();
                bool valid=makeSignal(result,signal);
                // save_buffer=true already re-arms capture before decoding. Calling
                // resume() again here would discard a next frame already in progress.
                if(!valid) ++invalidFrames;
                else {
                    ++receivedFrames;
                    bool wasActive=activeAt(now);
                    receivedAny=true; lastRx=now;
                    if(!signal.repeat) snprintf(lastKey,sizeof(lastKey),"%s",signal.key);
                    if(learning && now-learnStarted>=LearnTimeout) { learning=false; snprintf(lastLearn,sizeof(lastLearn),"Čas pro učení vypršel."); }
                    if(learning) {
                        if(learnWait && !wasActive) learnWait=false;
                        if(!learnWait && !signal.repeat) {
                            candidate=signal; candidateValid=true; ++candidateId; learning=false;
                            snprintf(lastLearn,sizeof(lastLearn),"Signál zachycen. Zadejte jméno a uložte.");
                        }
                    } else if(!suppress) {
                        const char *name="";
                        if(!signal.repeat) for(unsigned i=0;i<model.catalog.count;i++) {
                            if(model.catalog.items[i].signal.key[0] && matches(model.catalog.items[i].signal,signal)) { name=model.catalog.items[i].name; break; }
                        }
                        engine.frame(signal,name,now);
                    }
                }
            }
        }
        {
            Guard guard; uint32_t now=millis(); engine.tick(now);
            irGestures.tick(now);virtualGestures.tick(now);
            if(virtualActive && (now-virtualSeen>500 || now-virtualStarted>30000)) {
                virtualGestures.finish(virtualSeen-virtualStarted,now);virtualActive=false;
            }
            if(learning && now-learnStarted>=LearnTimeout) {
                learning=false; snprintf(lastLearn,sizeof(lastLearn),"Čas pro učení vypršel.");
            }
            if(learnWait && !activeAt(now)) learnWait=false;
            if(!learning && !activeAt(now)) suppress=false;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
static bool storedMapsValid(const LegacyMapStore &store) {
    if(store.version!=ConfigVersion || store.count>MaxKeys) return false;
    for(unsigned i=0;i<store.count;i++) {
        const auto &m=store.items[i];
        if(!memchr(m.name,0,sizeof(m.name)) || !validName(m.name) || !m.signal.key[0] ||
           !memchr(m.signal.key,0,sizeof(m.signal.key)) || m.signal.count>MaxRaw) return false;
    }
    return true;
}
// The HTTP task is the only writer. NVS is the rollback copy, so editing does
// not need a second 55 kB allocation (ESP32 heap is fragmented after Wi-Fi).
class ModelEdit {
    Guard guard;bool committed=false;
public:
    ModelEdit(){cancelGestures();}
    ~ModelEdit(){if(!committed)configASSERT(prefs.getBytes("system",&model,sizeof(model))==sizeof(model) && control::valid(model));}
    bool save(){
        if(!control::valid(model))return false;
        ++model.catalog.revision;
        committed=prefs.putBytes("system",&model,sizeof(model))==sizeof(model);
        return committed;
    }
};
static void fillState(JsonDocument &doc,bool withEvents=false) {
    char now[32]; isoTime(now,sizeof(now));
    doc["version"]=Version; doc["device_id"]=deviceId; doc["base_topic"]=baseTopic;
    doc["boot_id"]=bootId; doc["wifi"]=WiFi.status()==WL_CONNECTED;
    doc["ip"]=WiFi.localIP().toString(); doc["ap"]=bool(WiFi.getMode() & WIFI_AP); doc["ap_ssid"]=apName;
    doc["rssi"]=WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : 0;
    doc["time_synced"]=bool(now[0]); doc["time"]=now;
    doc["uptime_ms"]=millis(); doc["free_heap"]=ESP.getFreeHeap();doc["largest_heap_block"]=ESP.getMaxAllocHeap();
    doc["hostname"]=settings.hostname;
    doc["web_url"]=String("http://")+settings.hostname+".local";
    doc["mqtt_enabled"]=settings.mqttEnabled;
    doc["client_id"]=settings.mqttClientId[0] ? settings.mqttClientId : deviceId.c_str();
    Guard guard;
    doc["broker_role"]=mqttBroker ? "backup" : "primary";
    doc["broker_host"]=mqttBroker ? settings.backupHost : settings.mqttHost;
    doc["broker_port"]=mqttBroker ? settings.backupPort : settings.mqttPort;
    doc["broker_tls"]=mqttBroker ? settings.backupTls : settings.mqttTls;
    doc["broker_ip"]=brokerIp; doc["mqtt_detail"]=mqttDetail;
    doc["mqtt_attempts"]=mqttAttempts; doc["sent_events"]=sentEvents; doc["received_frames"]=receivedFrames;
    doc["mqtt"]=mqttOnline; doc["mqtt_error"]=mqttError; doc["active"]=activeAt(millis());
    doc["button"]=String(engine.active ? engine.name : ""); doc["last_button"]=lastButton;
    if(lastTrigger[0]) doc["last_trigger"]=lastTrigger; else doc["last_trigger"]=nullptr;
    doc["duration_ms"]=engine.duration; doc["repeats"]=engine.repeats;
    doc["last_key"]=lastKey; doc["learning"]=learning; doc["learn_message"]=lastLearn;
    doc["candidate_id"]=candidateValid ? candidateId : 0;
    doc["candidate_key"]=String(candidateValid ? candidate.key : "");
    doc["candidate_raw"]=candidateValid && candidate.count;
    doc["dropped_events"]=droppedEvents; doc["invalid_frames"]=invalidFrames;
    doc["key_count"]=model.catalog.count;doc["command_count"]=model.catalog.commandCount;
    doc["revision"]=model.catalog.revision;doc["screen"]=displayOn;
    doc["ota_available"]=esp_ota_get_next_update_partition(nullptr)!=nullptr;
    doc["ota_slot"]=esp_ota_get_running_partition()->label;
    if(withEvents) {
        auto events=doc.createNestedArray("events");
        for(unsigned i=0;i<recentCount;i++) {
            const auto &p=recentEvents[(recentNext+12-1-i)%12]; auto e=events.createNestedObject();
            e["sequence"]=p.event.sequence; e["event_type"]="command";e["gesture"]=p.gesture;
            e["command"]=p.code;e["command_name"]=p.commandName;e["source"]=p.virtualInput?"web":"ir";
            e["button"]=String(p.event.name); e["key"]=String(p.event.key); e["duration_ms"]=p.event.duration;
            e["uptime_ms"]=p.event.at; e["timestamp"]=String(p.timestamp);
        }
    }
}
static bool publishJson(PubSubClient &mqtt,const String &topic,const JsonDocument &doc,bool retain) {
    if(doc.overflowed()) return false;
    String payload; serializeJson(doc,payload); return mqtt.publish(topic.c_str(),payload.c_str(),retain);
}
static void publishState(PubSubClient &mqtt) {
    DynamicJsonDocument doc(4096); fillState(doc); publishJson(mqtt,baseTopic+"/state",doc,true);
}
static void discovery(PubSubClient &mqtt,bool remove=false) {
    if(!settings.discoveryEnabled) return;
    struct Entity { const char *kind,*id,*name,*field; };
    const Entity entities[]={
        {"event","trigger","Trigger",nullptr},
        {"binary_sensor","active","IR aktivní","active"},
        {"sensor","last_trigger","Poslední stisk","last_trigger"},
        {"sensor","last_button","Poslední tlačítko","last_button"},
        {"sensor","duration","Délka držení","duration_ms"}
    };
    for(const auto &e:entities) {
        String topic=String(settings.discoveryPrefix)+"/"+e.kind+"/"+deviceId+"/"+e.id+"/config";
        if(remove) { mqtt.publish(topic.c_str(),"",true); continue; }
        DynamicJsonDocument doc(1536);
        doc["name"]=e.name; doc["unique_id"]=deviceId+"_"+e.id;
        doc["availability_topic"]=baseTopic+"/availability";
        doc["state_topic"]=baseTopic+(e.field ? "/state" : "/trigger");
        auto dev=doc.createNestedObject("device"); dev.createNestedArray("identifiers").add(deviceId);
        dev["name"]=settings.name; dev["manufacturer"]="IRrem"; dev["model"]="TTGO T-Display"; dev["sw_version"]=Version;
        dev["configuration_url"]=String("http://")+settings.hostname+".local";
        if(!e.field) {
            auto types=doc.createNestedArray("event_types"); types.add("command");
            doc["device_class"]="button";
        } else if(!strcmp(e.id,"active")) {
            doc["value_template"]="{{ 'ON' if value_json.active else 'OFF' }}";
        } else {
            doc["value_template"]=String("{{ value_json.")+e.field+" }}";
            if(!strcmp(e.id,"last_trigger")) doc["device_class"]="timestamp";
            if(!strcmp(e.id,"duration")) { doc["unit_of_measurement"]="ms"; doc["icon"]="mdi:timer-outline"; }
        }
        publishJson(mqtt,topic,doc,true);
    }
}
static bool discoverPending=false;
static void mqttMessage(char *topic,uint8_t *payload,unsigned size) {
    if(!strcmp(topic,settings.birthTopic) && size==6 && !memcmp(payload,"online",6)) discoverPending=true;
}
static void connectionStatus(int code,const char *message) {
    Guard guard; mqttError=code; snprintf(mqttDetail,sizeof(mqttDetail),"%s",message);
}
static const char *mqttErrorText(int code) {
    switch(code) {
        case -4:return "Broker neodpověděl v časovém limitu";
        case -3:return "Spojení s brokerem se přerušilo";
        case -2:return "Nelze otevřít síťové spojení";
        case -1:return "Odpojeno";
        case 0:return "Připojeno";
        case 1:return "Broker odmítl verzi MQTT";
        case 2:return "Broker odmítl klientské ID";
        case 3:return "Broker není dostupný";
        case 4:return "Nesprávný uživatel nebo heslo";
        case 5:return "Broker nepovolil přístup";
        default:return "Chyba spojení";
    }
}
static bool resolveHost(const char *host,IPAddress &ip) {
    if(ip.fromString(host)) return true;
    String name(host); name.toLowerCase();
    if(name.endsWith(".local")) {
        name.remove(name.length()-6); ip=MDNS.queryHost(name,1500); return bool(ip);
    }
    return WiFi.hostByName(host,ip)==1;
}
static void mqttTask(void*) {
    WiFiClient socket; WiFiClientSecure secure;
    socket.setTimeout(settings.socketTimeout); secure.setTimeout(settings.socketTimeout);
    secure.setHandshakeTimeout(settings.socketTimeout);
    PubSubClient mqtt(socket);
    mqtt.setBufferSize(4096); mqtt.setKeepAlive(settings.keepAlive);
    mqtt.setSocketTimeout(settings.socketTimeout); mqtt.setCallback(mqttMessage);
    const uint32_t retryMs=settings.reconnectSeconds*1000;
    uint32_t attempted=millis()-retryMs,lastState=0;
    unsigned nextBroker=0;
    bool previousActivity=false;
    String availability=baseTopic+"/availability";
    for(;;) {
        if(cleanupRequested.exchange(false) && mqtt.connected()) {
            discovery(mqtt,true); mqtt.publish(availability.c_str(),"offline",true); mqtt.disconnect();
            attempted=millis();
        }
        if(reconnectRequested.exchange(false)) {
            if(mqtt.connected()) { mqtt.publish(availability.c_str(),"offline",true); mqtt.disconnect(); }
            socket.stop(); secure.stop(); nextBroker=0; attempted=millis()-retryMs;
        }
        bool online=mqtt.connected() && WiFi.status()==WL_CONNECTED;
        if(!online) {
            { Guard guard; mqttOnline=false; }
            Packet discarded; while(xQueueReceive(eventQueue,&discarded,0)==pdTRUE) { Guard guard; ++droppedEvents; }
            if(!settings.mqttEnabled) connectionStatus(-1,"MQTT je vypnuté v nastavení");
            else if(!settings.mqttHost[0]) connectionStatus(-1,"Vyplňte adresu hlavního brokeru");
            else if(WiFi.status()!=WL_CONNECTED) connectionStatus(-1,"Čekám na připojení Wi-Fi");
            else if(millis()-attempted>=retryMs) {
                // Reset MQTT state too: a Wi-Fi outage may leave the old TCP socket
                // reporting connected until the next write. Never skip a fresh CONNECT.
                mqtt.disconnect(); socket.stop(); secure.stop(); discoverPending=false;
                unsigned selected=nextBroker;
                const char *host=selected ? settings.backupHost : settings.mqttHost;
                uint16_t port=selected ? settings.backupPort : settings.mqttPort;
                const char *user=selected ? settings.backupUser : settings.mqttUser;
                const char *password=selected ? settings.backupPassword : settings.mqttPassword;
                const char *ca=selected ? settings.backupCa : settings.mqttCa;
                bool tls=selected ? settings.backupTls : settings.mqttTls;
                { Guard guard; mqttBroker=selected; ++mqttAttempts; brokerIp[0]=0; }
                connectionStatus(-1,"Navazuji spojení…");
                char clock[32]; isoTime(clock,sizeof(clock)); IPAddress ip;
                bool connected=false;
                if(tls && !clock[0]) connectionStatus(-101,"TLS čeká na synchronizaci času NTP");
                else if(!resolveHost(host,ip)) connectionStatus(-100,"DNS / mDNS: adresu brokeru nelze přeložit");
                else {
                    { Guard guard; snprintf(brokerIp,sizeof(brokerIp),"%s",ip.toString().c_str()); }
                    if(tls) {
                        // Preserve the hostname for certificate verification/SNI, including mDNS.
                        mqtt.setClient(secure); secure.setCACert(ca);
                        connected=secure.connect(ip,port,host,ca,nullptr,nullptr)==1;
                        if(!connected) {
                            char error[160]; secure.lastError(error,sizeof(error));
                            connectionStatus(-102,error[0]?error:"TLS spojení selhalo");
                        }
                    } else {
                        mqtt.setClient(socket); connected=socket.connect(ip,port,settings.socketTimeout*1000)==1;
                        if(!connected) connectionStatus(-2,mqttErrorText(-2));
                    }
                    mqtt.setServer(host,port);
                    if(connected) {
                        online=mqtt.connect(settings.mqttClientId[0]?settings.mqttClientId:deviceId.c_str(),
                            user[0]?user:nullptr,user[0]?password:nullptr,availability.c_str(),1,true,"offline",true);
                        connectionStatus(mqtt.state(),mqttErrorText(mqtt.state()));
                    }
                }
                attempted=millis(); // The retry interval starts after DNS/TLS has finished.
                nextBroker=settings.backupHost[0] ? 1-selected : 0;
                { Guard guard; ++mqttConnection; }
                if(online) {
                    // Stay on the working broker. Manual reconnect starts with the primary.
                    nextBroker=selected;
                    if(settings.discoveryEnabled && settings.birthTopic[0]) mqtt.subscribe(settings.birthTopic,1);
                    { Guard guard; mqttOnline=true; }
                    discovery(mqtt);
                    publishState(mqtt); mqtt.publish(availability.c_str(),"online",true);
                }
            }
        } else {
            mqtt.loop();
            if(discoverPending) { discoverPending=false; discovery(mqtt); publishState(mqtt); }
            Packet packet;
            while(xQueueReceive(eventQueue,&packet,0)==pdTRUE) {
                uint32_t connection; { Guard guard; connection=mqttConnection; }
                // Never replay a press after a reconnect or a blocked/slow broker.
                if(packet.connection!=connection || millis()-packet.event.at>500 || !mqtt.connected()) {
                    Guard guard; ++droppedEvents; continue;
                }
                DynamicJsonDocument doc(1536); const auto &e=packet.event;
                doc["schema"]=1;doc["event_type"]="command";doc["command"]=packet.code;
                doc["command_name"]=packet.commandName;doc["command_id"]=packet.commandId;doc["button_id"]=packet.buttonId;
                doc["gesture"]=packet.gesture;doc["steps"]=packet.steps;doc["source"]=packet.virtualInput?"web":"ir";
                doc["button"]=e.name; doc["key"]=e.key;
                doc["boot_id"]=bootId; doc["press_id"]=e.pressId; doc["sequence"]=e.sequence;
                doc["duration_ms"]=e.duration; doc["repeats"]=e.repeats; doc["uptime_ms"]=e.at;
                if(packet.timestamp[0]) doc["timestamp"]=packet.timestamp; else doc["timestamp"]=nullptr;
                bool sent=publishJson(mqtt,baseTopic+"/trigger",doc,false);
                { Guard guard; if(sent) ++sentEvents; else ++droppedEvents; }
                publishState(mqtt);
            }
            bool activity; { Guard guard; activity=activeAt(millis()); }
            if(activity!=previousActivity || millis()-lastState>=1000) {
                lastState=millis(); previousActivity=activity; publishState(mqtt);
            }
        }
        if(online && !mqtt.connected()) {
            Guard guard; mqttOnline=false; mqttError=mqtt.state();
            snprintf(mqttDetail,sizeof(mqttDetail),"%s",mqttErrorText(mqttError));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern const char indexStart[] asm("_binary_web_index_html_start");
extern const char indexEnd[] asm("_binary_web_index_html_end");
extern const char scriptStart[] asm("_binary_web_app_js_start");
extern const char scriptEnd[] asm("_binary_web_app_js_end");
extern const char styleStart[] asm("_binary_web_style_css_start");
extern const char styleEnd[] asm("_binary_web_style_css_end");
static esp_err_t sendJson(httpd_req_t *request,const JsonDocument &doc,const char *status="200 OK") {
    if(doc.overflowed()) return httpd_resp_send_err(request,HTTPD_500_INTERNAL_SERVER_ERROR,"JSON allocation failed");
    // Stream the catalog instead of allocating another potentially 40 kB String.
    struct Writer {
        httpd_req_t *r;char buffer[512];size_t used=0;bool good=true;
        explicit Writer(httpd_req_t *request):r(request){}
        bool flush(){if(used && good)good=httpd_resp_send_chunk(r,buffer,used)==ESP_OK;used=0;return good;}
        size_t write(uint8_t b){if(!good)return 0;buffer[used++]=b;if(used==sizeof(buffer))flush();return good?1:0;}
        size_t write(const uint8_t *p,size_t n){size_t i=0;while(i<n && write(p[i]))++i;return i;}
    } writer(request);
    httpd_resp_set_status(request,status);
    httpd_resp_set_type(request,"application/json; charset=utf-8");
    httpd_resp_set_hdr(request,"Cache-Control","no-store");
    serializeJson(doc,writer);return writer.flush()?httpd_resp_send_chunk(request,nullptr,0):ESP_FAIL;
}
static esp_err_t error(httpd_req_t *request,const char *message,const char *status="400 Bad Request") {
    StaticJsonDocument<256> doc; doc["error"]=message; return sendJson(request,doc,status);
}
static esp_err_t okay(httpd_req_t *request) { StaticJsonDocument<64> doc; doc["ok"]=true; return sendJson(request,doc); }
static bool certificateValid(const char *pem) {
    mbedtls_x509_crt certificate; mbedtls_x509_crt_init(&certificate);
    int result=mbedtls_x509_crt_parse(&certificate,(const unsigned char*)pem,strlen(pem)+1);
    mbedtls_x509_crt_free(&certificate); return result==0;
}
#include "webapi.h"
static esp_err_t staticFile(httpd_req_t *request) {
    const char *start=indexStart,*end=indexEnd,*type="text/html; charset=utf-8";
    if(!strcmp(request->uri,"/app.js")) { start=scriptStart; end=scriptEnd; type="text/javascript; charset=utf-8"; }
    else if(!strcmp(request->uri,"/style.css")) { start=styleStart; end=styleEnd; type="text/css; charset=utf-8"; }
    httpd_resp_set_type(request,type); httpd_resp_set_hdr(request,"X-Content-Type-Options","nosniff");
    httpd_resp_set_hdr(request,"Content-Security-Policy","default-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'self'");
    return httpd_resp_send(request,start,end-start-1);
}
static void startWeb() {
    httpd_config_t config=HTTPD_DEFAULT_CONFIG(); config.max_uri_handlers=32; config.stack_size=16384;
    config.lru_purge_enable=true; config.recv_wait_timeout=2; config.send_wait_timeout=2;
    httpd_handle_t server; ESP_ERROR_CHECK(httpd_start(&server,&config));
    for(const char *path:{"/","/app.js","/style.css"}) {
        httpd_uri_t route{}; route.uri=path; route.method=HTTP_GET; route.handler=staticFile;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server,&route));
    }
    for(const char *path:{"/api/state","/api/settings","/api/catalog","/api/profiles"}) {
        httpd_uri_t route{}; route.uri=path; route.method=HTTP_GET; route.handler=getApi;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server,&route));
    }
    for(const char *path:{"/api/settings","/api/mqtt/reconnect","/api/learn","/api/learn/cancel","/api/buttons","/api/buttons/delete","/api/commands","/api/commands/delete","/api/profile","/api/virtual","/api/backup","/api/restore","/api/update","/api/restart"}) {
        httpd_uri_t route{}; route.uri=path; route.method=HTTP_POST; route.handler=postApi;
        ESP_ERROR_CHECK(httpd_register_uri_handler(server,&route));
    }
}
#include "fallback_wifi.h"
static void drawDisplay() {
    if(!displayOn) return;
    char now[32]; isoTime(now,sizeof(now));
    bool mqtt,active,learn; char button[NameSize];
    { Guard guard; mqtt=mqttOnline; active=activeAt(millis()); learn=learning; snprintf(button,sizeof(button),"%s",lastButton); }
    // Built-in font is ASCII; web/MQTT retain full UTF-8 names.
    for(char *p=button;*p;p++) if((unsigned char)*p>=128) *p='?';
    StatusDisplay::Lines lines{};
    snprintf(lines[0],sizeof(lines[0]),"IRrem %s",Version);
    const bool connected=WiFi.status()==WL_CONNECTED;
    snprintf(lines[1],sizeof(lines[1]),"WiFi: %.32s",connected ? settings.ssid : settings.ssid[0] ? "Pripojuji..." : "Nastavte ve webu");
    snprintf(lines[2],sizeof(lines[2]),"IP: %s",connected ? WiFi.localIP().toString().c_str() : "--");
    snprintf(lines[3],sizeof(lines[3]),"MQTT: %s   NTP: %s",mqtt?"OK":"OFFLINE",now[0]?"OK":"CEKAM");
    snprintf(lines[4],sizeof(lines[4]),"IR: %s",learn?"UCENI":active?"PRIJEM":"KLID");
    snprintf(lines[5],sizeof(lines[5]),"Btn: %.34s",button);
    snprintf(lines[6],sizeof(lines[6]),"%.31s.local",settings.hostname);
    if(apActive) {
        snprintf(lines[7],sizeof(lines[7]),"AP: %s",apName.c_str());
        snprintf(lines[8],sizeof(lines[8]),"Otevrena sit - bez hesla");
        snprintf(lines[9],sizeof(lines[9]),"Web: 192.168.4.1");
    }
    statusDisplay.draw(lines);
}
void setup() {
    Serial.begin(115200);
    mutex=xSemaphoreCreateMutex(); eventQueue=xQueueCreate(32,sizeof(Packet)); configASSERT(mutex && eventQueue);
    pinMode(IrGround,OUTPUT); digitalWrite(IrGround,LOW); pinMode(IrPower,OUTPUT); digitalWrite(IrPower,HIGH);
    pinMode(ScreenButton,INPUT); pinMode(SetupButton,INPUT_PULLUP);
    lcd.init(); lcd.setRotation(1); pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,HIGH);
    lcd.fillScreen(TFT_BLACK); lcd.setCursor(5,10,2); lcd.setTextColor(TFT_WHITE); lcd.print("IRrem start...");
    if(!statusDisplay.begin()) {
        Serial.println("LCD ERROR: row buffer allocation failed");
        lcd.println("LCD BUFFER ERROR");
    } else Serial.println("LCD: buffered rows, 9600 bytes, redraw on change");
    // Keep configuration outside PlatformIO's fixed boot_app0 upload at 0xe000.
    if(nvs_flash_init_partition("irrem_nvs")!=ESP_OK || !prefs.begin("irrem",false,"irrem_nvs")) {
        lcd.println("NVS ERROR"); while(true) delay(1000);
    }
    if(prefs.getBytesLength("settings")==sizeof(settings)) {
        auto saved=std::unique_ptr<Settings>(new Settings);
        prefs.getBytes("settings",saved.get(),sizeof(Settings)); if(!settingsError(*saved)) settings=*saved;
    } else if(prefs.getBytesLength("settings")==sizeof(LegacySettings)) {
        LegacySettings saved; prefs.getBytes("settings",&saved,sizeof(saved));
        if(legacyValid(saved)) migrateSettings(saved,settings);
    }
    bool migrated=false;
    if(prefs.getBytesLength("system")==sizeof(model)) {
        prefs.getBytes("system",&model,sizeof(model));
        if(!control::valid(model)) { lcd.println("CONFIG ERROR");while(true)delay(1000); }
        settings=model.network;
    } else {
        if(prefs.getBytesLength("system")) { lcd.println("CONFIG VERSION ERROR");while(true)delay(1000); }
        control::defaults(model.catalog);model.network=settings;
        if(prefs.getBytesLength("maps")==sizeof(LegacyMapStore)) {
            auto old=std::unique_ptr<LegacyMapStore>(new(std::nothrow) LegacyMapStore);
            if(!old || prefs.getBytes("maps",old.get(),sizeof(*old))!=sizeof(*old) || !storedMapsValid(*old)) {
                lcd.println("MAP MIGRATION ERROR");while(true)delay(1000);
            }
            for(unsigned i=0;i<old->count;i++) {
                auto &button=model.catalog.items[model.catalog.count++];
                static_cast<Mapping&>(button)=old->items[i];button.id=model.catalog.nextId++;control::assignLegacy(button);
            }
        }
        migrated=true;
    }
    uint64_t mac=ESP.getEfuseMac(); char id[24]; snprintf(id,sizeof(id),"irrem-%012llx",(unsigned long long)mac);
    deviceId=id; baseTopic=settings.mqttTopic[0] ? String(settings.mqttTopic) : "irrem/"+deviceId; apName="IRrem-"+deviceId.substring(deviceId.length()-6);
    if(migrated) {
        if(!control::valid(model) || prefs.putBytes("system",&model,sizeof(model))!=sizeof(model)) {
            lcd.println("MIGRATION SAVE ERROR");while(true)delay(1000);
        }
        prefs.remove("maps");prefs.remove("settings");prefs.remove("ap_password");
        Serial.printf("Migrated %u buttons, %u commands\n",model.catalog.count,model.catalog.commandCount);
    }
    displayOn=model.screen;digitalWrite(TFT_BL,displayOn?HIGH:LOW);
    snprintf(bootId,sizeof(bootId),"%08lx",(unsigned long)esp_random());
    engine.releaseMs=settings.releaseMs; engine.holdMs=settings.holdMs;
    WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.setHostname(settings.hostname); WiFi.setAutoReconnect(true); WiFi.setSleep(false);
    if(settings.ssid[0]) WiFi.begin(settings.ssid,settings.wifiPassword); else startAp();
    configTime(0,0,settings.ntp,"time.cloudflare.com");
    startWeb();
    configASSERT(xTaskCreatePinnedToCore(irTask,"ir",6144,nullptr,3,nullptr,1)==pdPASS);
    configASSERT(xTaskCreatePinnedToCore(mqttTask,"mqtt",12288,nullptr,1,nullptr,0)==pdPASS);
    Serial.printf("IRrem %s, %s, IR GPIO25 / GND26 / VCC27\n",Version,deviceId.c_str());
}
void loop() {
    static uint32_t drawn=0,setupPressed=0,screenChanged=0;
    static bool setupWas=false,screenWas=false,setupDone=false;
    uint32_t now=millis();
    updateWifi(now);
    bool setupDown=digitalRead(SetupButton)==LOW;
    if(setupDown && !setupWas) { setupPressed=now; setupDone=false; }
    if(setupDown && !setupDone && now-setupPressed>=3000) { startAp(); setupDone=true; }
    setupWas=setupDown;
    bool screenDown=digitalRead(ScreenButton)==LOW;
    if(screenDown!=screenWas && now-screenChanged>=50) {
        screenChanged=now; screenWas=screenDown;
        if(screenDown) { displayOn=!displayOn; digitalWrite(TFT_BL,displayOn?HIGH:LOW); }
    }
    if(model.screenTimeout && !apActive && now>uint32_t(model.screenTimeout)*1000 && now-lastRx>uint32_t(model.screenTimeout)*1000 && now-screenChanged>uint32_t(model.screenTimeout)*1000) { displayOn=false;digitalWrite(TFT_BL,LOW); }
    if(now-drawn>=300) { drawn=now; drawDisplay(); }
    bool restart; { Guard guard; restart=rebootAt && int32_t(now-rebootAt)>=0; }
    if(restart) ESP.restart();
    delay(10);
}
