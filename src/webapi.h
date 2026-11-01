#pragma once
// Included by main.cpp after the HTTP/IR helpers. The HTTP task serializes all writes.
static bool receiveBytes(httpd_req_t *r,void *dest,size_t size) {
    size_t done=0; auto *p=static_cast<char*>(dest);
    while(done<size) { int n=httpd_req_recv(r,p+done,size-done);if(n<=0)return false;done+=n; }
    return true;
}
static bool revisionMatches(JsonObjectConst doc) {
    return doc["revision"].is<uint32_t>() && doc["revision"].as<uint32_t>()==model.catalog.revision;
}
static esp_err_t backup(httpd_req_t *r,bool full) {
    // All configuration mutations run on this same HTTP task; IR only reads it.
    const void *body=full?static_cast<void*>(&model):static_cast<void*>(&model.catalog);
    control::BackupHeader header;header.scope=full?1:0;header.size=full?sizeof(model):sizeof(model.catalog);
    header.crc=control::crc32(body,header.size);
    httpd_resp_set_type(r,"application/octet-stream");httpd_resp_set_hdr(r,"Cache-Control","no-store");
    httpd_resp_set_hdr(r,"Content-Disposition",full?"attachment; filename=irrem-system.irrem":"attachment; filename=irrem-buttons.irrem");
    if(httpd_resp_send_chunk(r,reinterpret_cast<const char*>(&header),sizeof(header))!=ESP_OK)return ESP_FAIL;
    auto *bytes=static_cast<const char*>(body);
    for(size_t offset=0;offset<header.size;offset+=2048)
        if(httpd_resp_send_chunk(r,bytes+offset,std::min(size_t(2048),header.size-offset))!=ESP_OK)return ESP_FAIL;
    return httpd_resp_send_chunk(r,nullptr,0);
}
static esp_err_t restore(httpd_req_t *r) {
    control::BackupHeader header;
    if(r->content_len<sizeof(header) || !receiveBytes(r,&header,sizeof(header)) || memcmp(header.magic,"IRREM03",8) || header.scope>1 ||
       header.size!=(header.scope?sizeof(control::Store):sizeof(control::Catalog)) || r->content_len!=sizeof(header)+header.size)
        return error(r,"Neplatný nebo nekompatibilní soubor zálohy.");
    uint32_t currentRevision=model.catalog.revision;
    ModelEdit edit;auto *next=&model;
    void *body=header.scope?static_cast<void*>(next):static_cast<void*>(&next->catalog);
    if(!receiveBytes(r,body,header.size) || control::crc32(body,header.size)!=header.crc || !control::valid(*next))
        return error(r,"Záloha je poškozená nebo obsahuje neplatné přiřazení. Nic se nezměnilo.");
    char preview[8]={};httpd_req_get_hdr_value_str(r,"X-IRrem-Preview",preview,sizeof(preview));
    if(!strcmp(preview,"1")) {
        StaticJsonDocument<384> doc;doc["scope"]=header.scope?"system":"buttons";
        doc["buttons"]=next->catalog.count;doc["commands"]=next->catalog.commandCount;
        doc["name"]=next->network.name;doc["revision"]=currentRevision;return sendJson(r,doc);
    }
    char revision[16]={};httpd_req_get_hdr_value_str(r,"X-IRrem-Revision",revision,sizeof(revision));
    char *end=nullptr;unsigned long supplied=strtoul(revision,&end,10);
    if(!revision[0] || *end || supplied!=currentRevision)return error(r,"Nastavení se změnilo; načtěte zálohu znovu.","409 Conflict");
    next->catalog.revision=currentRevision;
    if(!edit.save())return error(r,"Zálohu se nepodařilo uložit.","500 Internal Server Error");
    learning=false;candidateValid=false;if(header.scope)rebootAt=millis()+1500;
    return okay(r);
}
static esp_err_t updateFirmware(httpd_req_t *r) {
    const esp_partition_t *partition=esp_ota_get_next_update_partition(nullptr);
    if(!partition)return error(r,"Toto rozdělení flash nepodporuje OTA. Je nutná první instalace přes USB.");
    if(r->content_len<1024 || r->content_len>partition->size)return error(r,"Firmware má nesprávnou velikost.");
    esp_ota_handle_t handle=0;
    if(esp_ota_begin(partition,r->content_len,&handle)!=ESP_OK)return error(r,"Nelze připravit OTA oddíl.","500 Internal Server Error");
    char buffer[2048];size_t remaining=r->content_len;bool first=true;
    while(remaining) {
        size_t length=std::min(remaining,sizeof(buffer));
        if(!receiveBytes(r,buffer,length) || (first && uint8_t(buffer[0])!=ESP_IMAGE_HEADER_MAGIC) || esp_ota_write(handle,buffer,length)!=ESP_OK) {
            esp_ota_abort(handle);return error(r,"Aktualizace nebyla dokončena. Původní firmware zůstává aktivní.");
        }
        first=false;remaining-=length;
    }
    if(esp_ota_end(handle)!=ESP_OK || esp_ota_set_boot_partition(partition)!=ESP_OK)
        return error(r,"Kontrola firmwaru selhala. Původní firmware zůstává aktivní.");
    { Guard guard;cancelGestures();rebootAt=millis()+1500; }return okay(r);
}
static esp_err_t getApi(httpd_req_t *r) {
    String path=r->uri;
    if(path=="/api/state") { DynamicJsonDocument doc(8192);fillState(doc,true);return sendJson(r,doc); }
    if(path=="/api/settings") {
        DynamicJsonDocument doc(7168);publicSettings(doc,model.network);
        doc["screen"]=model.screen;doc["screen_timeout"]=model.screenTimeout;doc["revision"]=model.catalog.revision;
        return sendJson(r,doc);
    }
    if(path=="/api/catalog") {
        DynamicJsonDocument doc(4096+JSON_OBJECT_SIZE(3)+JSON_ARRAY_SIZE(model.catalog.count)+JSON_ARRAY_SIZE(model.catalog.commandCount)+
            model.catalog.count*JSON_OBJECT_SIZE(10)+model.catalog.commandCount*JSON_OBJECT_SIZE(3));
        doc["revision"]=model.catalog.revision;
        auto buttons=doc.createNestedArray("buttons");auto commands=doc.createNestedArray("commands");
        { Guard guard;
          for(unsigned i=0;i<model.catalog.count;i++) {
            const auto &b=model.catalog.items[i];auto o=buttons.createNestedObject();
            o["id"]=b.id;o["name"]=b.name;o["key"]=b.signal.key;o["raw"]=bool(b.signal.count);
            o["single"]=b.single;o["double"]=b.twice;o["long"]=b.hold;
            o["long_ms"]=b.longMs;o["double_ms"]=b.doubleMs;o["repeat_ms"]=b.repeatMs;
          }
          for(unsigned i=0;i<model.catalog.commandCount;i++) {
            const auto &c=model.catalog.commands[i];auto o=commands.createNestedObject();
            o["id"]=c.id;o["name"]=c.name;o["code"]=c.code;
          }
        }return sendJson(r,doc);
    }
    if(path=="/api/profiles") {
        DynamicJsonDocument doc(2048);auto list=doc.createNestedArray("profiles");
        for(unsigned i=0;i<ProfileCount;i++){auto p=list.createNestedObject();p["id"]=i;p["name"]=profiles[i].name;p["count"]=profiles[i].count;}
        return sendJson(r,doc);
    }
    return error(r,"Nenalezeno","404 Not Found");
}
static esp_err_t postApi(httpd_req_t *r) {
    char marker[8],type[64];
    if(httpd_req_get_hdr_value_str(r,"X-IRRem",marker,sizeof(marker))!=ESP_OK || strcmp(marker,"1") ||
       httpd_req_get_hdr_value_str(r,"Content-Type",type,sizeof(type))!=ESP_OK)
        return error(r,"Požadavek musí přijít z webu zařízení.","403 Forbidden");
    String path=r->uri;
    if(path=="/api/restore" || path=="/api/update") {
        if(strcmp(type,"application/octet-stream"))return error(r,"Očekávám binární soubor.");
        return path=="/api/restore"?restore(r):updateFirmware(r);
    }
    if(strncmp(type,"application/json",16) || r->content_len<2 || r->content_len>10000)return error(r,"Neplatný požadavek.");
    auto body=std::unique_ptr<char[]>(new(std::nothrow) char[r->content_len+1]);
    if(!body)return error(r,"Nedostatek paměti.","503 Service Unavailable");
    if(!receiveBytes(r,body.get(),r->content_len))return error(r,"Neúplný požadavek.","408 Request Timeout");
    body[r->content_len]=0;DynamicJsonDocument doc(r->content_len+3072);
    if(deserializeJson(doc,body.get(),r->content_len,DeserializationOption::NestingLimit(4)) || !doc.is<JsonObject>())return error(r,"Neplatný JSON.");
    body.reset();
    if(path=="/api/backup")return backup(r,doc["scope"]=="system");
    if(path=="/api/mqtt/reconnect"){reconnectRequested=true;return okay(r);}
    if(path=="/api/restart"){Guard guard;cancelGestures();rebootAt=millis()+1000;return okay(r);}
    if(path=="/api/learn") {
        Guard guard;cancelGestures();engine.release(millis());cancelGestures();learning=true;
        learnWait=activeAt(millis());suppress=true;candidateValid=false;learnStarted=millis();
        snprintf(lastLearn,sizeof(lastLearn),"Uvolněte ovladač a stiskněte vybrané tlačítko.");return okay(r);
    }
    if(path=="/api/learn/cancel") { Guard guard;learning=false;candidateValid=false;snprintf(lastLearn,sizeof(lastLearn),"Učení zrušeno.");return okay(r); }
    if(path=="/api/virtual") {
        if(!doc["button_id"].is<uint32_t>() || !doc["token"].is<uint32_t>() || !doc["token"].as<uint32_t>() || !doc["phase"].is<const char*>())return error(r,"Neplatný virtuální stisk.");
        Guard guard;uint32_t now=millis(),token=doc["token"];const char *phase=doc["phase"];
        // A late release after the heartbeat watchdog is already completed.
        if(!strcmp(phase,"release") && token==virtualToken && !virtualActive)return okay(r);
        int i=control::index(model.catalog,doc["button_id"]);if(i<0)return error(r,"Tlačítko už neexistuje.","409 Conflict");
        if(learning || !mqttOnline)return error(r,learning?"Probíhá učení IR; ovladač je pozastaven.":"MQTT není připojeno.","409 Conflict");
        if(!strcmp(phase,"press")) {
            if(virtualActive)return error(r,"Jiné virtuální tlačítko je stále stisknuté.","409 Conflict");
            virtualToken=token;virtualStarted=virtualSeen=now;virtualActive=true;++virtualPress;
            const auto &b=model.catalog.items[i];virtualGestures.start(b,virtualPress,now,repeatable(b.hold));
        } else if((!strcmp(phase,"hold") || !strcmp(phase,"release")) && token==virtualToken && virtualActive) {
            virtualSeen=now;
            if(!strcmp(phase,"release")){virtualGestures.finish(now-virtualStarted,now);virtualActive=false;}
            else virtualGestures.sample(now-virtualStarted);
        } else return error(r,"Virtuální stisk vypršel nebo nemá platný začátek.","409 Conflict");
        return okay(r);
    }
    if(!revisionMatches(doc.as<JsonObjectConst>()))return error(r,"Nastavení se mezitím změnilo. Obnovte seznam a zkuste to znovu.","409 Conflict");
    ModelEdit edit;auto *next=&model;
    auto &c=next->catalog;
    if(path=="/api/settings") {
        if(const char *message=parseSettings(doc.as<JsonObjectConst>(),next->network))return error(r,message);
        if((next->network.mqttTls && next->network.mqttHost[0] && !certificateValid(next->network.mqttCa)) ||
           (next->network.backupTls && next->network.backupHost[0] && !certificateValid(next->network.backupCa)))return error(r,"Neplatný CA certifikát.");
        if(doc.containsKey("screen")){if(!doc["screen"].is<bool>())return error(r,"Neplatný stav displeje.");next->screen=doc["screen"];}
        if(doc.containsKey("screen_timeout")){if(!doc["screen_timeout"].is<unsigned>() || doc["screen_timeout"].as<unsigned>()>3600)return error(r,"Čas displeje musí být 0–3600 s.");next->screenTimeout=doc["screen_timeout"];}
    } else if(path=="/api/buttons" || path=="/api/buttons/delete") {
        if(!doc["id"].is<uint32_t>())return error(r,"Chybí ID tlačítka.");
        uint32_t id=doc["id"];int i=control::index(c,id);
        if(id && i<0)return error(r,"Tlačítko neexistuje.");
        if(path=="/api/buttons/delete") {
            if(i<0)return error(r,"Tlačítko neexistuje.");
            for(unsigned j=i+1;j<c.count;j++)c.items[j-1]=c.items[j];c.items[--c.count]=control::Button{};
        } else {
            if(!id){if(c.count>=MaxKeys)return error(r,"Lze uložit nejvýše 64 tlačítek.");i=c.count++;c.items[i].id=c.nextId++;}
            auto &b=c.items[i];
            if(!copyString(doc["name"],b.name,sizeof(b.name)) || !validName(b.name))return error(r,"Název musí mít 1–48 bajtů.");
            struct Binding{const char *key;uint32_t *value;};
            for(auto binding:{Binding{"single",&b.single},Binding{"double",&b.twice},Binding{"long",&b.hold}})
                if(doc.containsKey(binding.key)){if(!doc[binding.key].is<uint32_t>())return error(r,"Neplatný příkaz.");*binding.value=doc[binding.key];}
            struct Timing{const char *key;uint16_t *value;unsigned min,max;};
            for(auto timing:{Timing{"long_ms",&b.longMs,250,10000},Timing{"double_ms",&b.doubleMs,150,1500},Timing{"repeat_ms",&b.repeatMs,250,2000}})
                if(doc.containsKey(timing.key)){if(!doc[timing.key].is<unsigned>() || doc[timing.key].as<unsigned>()<timing.min || doc[timing.key].as<unsigned>()>timing.max)return error(r,"Časování je mimo povolený rozsah.");*timing.value=doc[timing.key];}
            if(doc["clear_signal"]==true)b.signal=Signal{};
            if(doc.containsKey("candidate_id")) {
                if(!doc["candidate_id"].is<uint32_t>() || !candidateValid || doc["candidate_id"].as<uint32_t>()!=candidateId)return error(r,"Signál vypršel; spusťte párování znovu.");
                b.signal=candidate;
            }
        }
    } else if(path=="/api/commands" || path=="/api/commands/delete") {
        if(!doc["id"].is<uint32_t>())return error(r,"Chybí ID příkazu.");
        uint32_t id=doc["id"];int i=-1;for(unsigned j=0;j<c.commandCount;j++)if(c.commands[j].id==id)i=j;
        if(id && i<0)return error(r,"Příkaz neexistuje.");
        if(path=="/api/commands/delete") {
            if(i<0)return error(r,"Příkaz neexistuje.");
            for(unsigned j=0;j<c.count;j++)if(c.items[j].single==id || c.items[j].twice==id || c.items[j].hold==id)return error(r,"Příkaz používá tlačítko. Nejprve změňte jeho přiřazení.","409 Conflict");
            for(unsigned j=i+1;j<c.commandCount;j++)c.commands[j-1]=c.commands[j];c.commands[--c.commandCount]=control::Command{};
        } else {
            if(!id){if(c.commandCount>=control::MaxCommands)return error(r,"Lze uložit nejvýše 64 příkazů.");i=c.commandCount++;c.commands[i].id=c.nextId++;}
            auto &a=c.commands[i];
            if(!copyString(doc["name"],a.name,sizeof(a.name)) || !validName(a.name) || !copyString(doc["code"],a.code,sizeof(a.code)) || !control::commandValid(a.code))return error(r,"Neplatný název nebo parametry příkazu.");
        }
    } else if(path=="/api/profile") {
        if(!doc["id"].is<unsigned>() || doc["id"].as<unsigned>()>=ProfileCount)return error(r,"Neplatná předvolba.");
        const auto &p=profiles[doc["id"].as<unsigned>()];
        for(unsigned i=0;i<p.count;i++) {
            control::Button b;snprintf(b.name,sizeof(b.name),"%s_%s",p.prefix,p.keys[i].name);
            snprintf(b.signal.key,sizeof(b.signal.key),"NEC:32:%016llX",(unsigned long long)p.keys[i].code);
            bool duplicate=false;for(unsigned j=0;j<c.count;j++)if(matches(b.signal,c.items[j].signal) || !strcmp(b.name,c.items[j].name))duplicate=true;
            if(duplicate)continue;if(c.count>=MaxKeys)return error(r,"Předvolba se nevejde do 64 tlačítek. Nic se nezměnilo.");
            b.id=c.nextId++;c.items[c.count++]=b;
        }
    } else return error(r,"Nenalezeno","404 Not Found");
    if(!control::valid(*next))return error(r,"Duplicitní název / signál nebo neplatné přiřazení příkazu. Nic se nezměnilo.");
    if(!edit.save())return error(r,"Nastavení se nepodařilo uložit.","500 Internal Server Error");
    if(path=="/api/settings") {
        if(settings.discoveryEnabled && (!next->network.discoveryEnabled || strcmp(settings.discoveryPrefix,next->network.discoveryPrefix) ||
            strcmp(settings.mqttHost,next->network.mqttHost) || settings.mqttPort!=next->network.mqttPort))cleanupRequested=true;
        rebootAt=millis()+1500;
    }
    if(path=="/api/buttons" && doc.containsKey("candidate_id")){candidateValid=false;learning=false;}
    StaticJsonDocument<128> reply;reply["ok"]=true;reply["revision"]=model.catalog.revision;return sendJson(r,reply);
}
