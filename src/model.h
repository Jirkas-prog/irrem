#pragma once
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <initializer_list>

namespace control {
constexpr unsigned MaxCommands=64;
struct Command { uint32_t id=0; char name[49]={}, code[97]={}; };
struct Button : remote::Mapping {
    uint32_t id=0, single=0, twice=0, hold=0;
    uint16_t longMs=1000, doubleMs=350, repeatMs=350;
};
struct Catalog {
    uint32_t revision=1, nextId=1;
    unsigned count=0, commandCount=0;
    Button items[remote::MaxKeys];
    Command commands[MaxCommands];
};
struct Store {
    uint32_t version=3;
    Catalog catalog;
    Settings network;
    char apPassword[9]={}; // Legacy field retained for NVS/backup compatibility; AP is always open.
    bool screen=true;
    uint16_t screenTimeout=0;
};
inline bool commandValid(const char *s) {
    if(!strcmp(s,"on") || !strcmp(s,"off") || !strcmp(s,"white") ||
       !strcmp(s,"dim/up") || !strcmp(s,"dim/down")) return true;
    const char *tail=strchr(s,'/');if(!tail)return false;
    for(++tail;*tail;tail++)if(!isdigit(static_cast<unsigned char>(*tail)) && *tail!='.' && *tail!='/')return false;
    bool white=!strncmp(s,"white/",6);
    if(!white && strncmp(s,"rgb/",4))return false;
    const char *p=s+(white?6:4);char *end=nullptr;
    double pct=strtod(p,&end);
    if(end==p || *end!='/' || end[-1]=='.' || !isfinite(pct) || pct<0.1 || pct>100)return false;
    p=end+1;
    for(unsigned i=0;i<(white?1u:3u);i++) {
        if(!isdigit(static_cast<unsigned char>(*p)))return false;
        unsigned long value=strtoul(p,&end,10);
        if(value>(white?6500u:255u) || (white && value<2000))return false;
        if(i==(white?0u:2u))return *end==0;
        if(*end!='/')return false;p=end+1;
    }
    return false;
}
inline const Command *command(const Catalog &c,uint32_t id) {
    for(unsigned i=0;i<c.commandCount;i++) if(c.commands[i].id==id) return &c.commands[i];
    return nullptr;
}
inline int index(const Catalog &c,uint32_t id) {
    for(unsigned i=0;i<c.count;i++) if(c.items[i].id==id) return int(i);
    return -1;
}
inline bool valid(const Catalog &c) {
    if(c.count>remote::MaxKeys || c.commandCount>MaxCommands || !c.nextId || !c.revision) return false;
    for(unsigned i=0;i<c.commandCount;i++) {
        const auto &a=c.commands[i];
        if(!a.id || a.id>=c.nextId || !terminated(a.name) || !remote::validName(a.name) ||
           !terminated(a.code) || !commandValid(a.code)) return false;
        for(unsigned j=0;j<i;j++) if(a.id==c.commands[j].id || !strcmp(a.name,c.commands[j].name)) return false;
    }
    for(unsigned i=0;i<c.count;i++) {
        const auto &a=c.items[i];
        if(!a.id || a.id>=c.nextId || !terminated(a.name) || !remote::validName(a.name) ||
           !terminated(a.signal.key) || a.signal.count>remote::MaxRaw ||
           (a.signal.count && !a.signal.key[0]) || a.longMs<250 || a.longMs>10000 ||
           a.doubleMs<150 || a.doubleMs>1500 || a.repeatMs<250 || a.repeatMs>2000) return false;
        for(unsigned k=0;k<a.signal.count;k++) if(!a.signal.raw[k]) return false;
        for(auto id:{a.single,a.twice,a.hold}) if(id && !command(c,id)) return false;
        for(unsigned j=0;j<i;j++) if(a.id==c.items[j].id || !strcmp(a.name,c.items[j].name) ||
           (a.signal.key[0] && remote::matches(a.signal,c.items[j].signal))) return false;
        if(command(c,a.id)) return false;
    }
    return true;
}
inline bool valid(const Store &s) {
    return s.version==3 && valid(s.catalog) && !settingsError(s.network) &&
        terminated(s.apPassword) && (!s.apPassword[0] || strlen(s.apPassword)==8) && s.screenTimeout<=3600;
}
inline uint32_t add(Catalog &c,const char *name,const char *code) {
    if(c.commandCount>=MaxCommands) return 0;
    auto &a=c.commands[c.commandCount++]; a.id=c.nextId++;
    snprintf(a.name,sizeof(a.name),"%s",name); snprintf(a.code,sizeof(a.code),"%s",code); return a.id;
}
inline void defaults(Catalog &c) {
    add(c,"Zapnout · bílá 36 %","white/36/3000"); // 1
    add(c,"Vypnout relé","off"); // 2
    add(c,"Jas +","dim/up"); add(c,"Jas −","dim/down"); // 3,4
    add(c,"Bílá · zachovat jas","white"); // 5
    for(unsigned pct:{1u,2u,5u,10u,36u,60u,80u,100u}) {
        char name[49],code[97]; snprintf(name,sizeof(name),"Bílá %u %%",pct);
        snprintf(code,sizeof(code),"white/%u/3000",pct); add(c,name,code); // 6..13
    }
    for(unsigned pct:{5u,20u,30u}) {
        char name[49],code[97]; snprintf(name,sizeof(name),"Teplá RGB %u %%",pct);
        snprintf(code,sizeof(code),"rgb/%u/255/180/100",pct); add(c,name,code); // 14..16
    }
}
inline void assignLegacy(Button &b) {
    const char *n=b.name;
    if(!strcmp(n,"retez_on") || !strcmp(n,"TV_L_ON")) b.single=1;
    else if(!strcmp(n,"retez_off") || !strcmp(n,"TV_L_OFF")) b.single=2;
    else if(!strcmp(n,"retez_dim_plus")) b.single=b.hold=3;
    else if(!strcmp(n,"retez_dim_minus")) b.single=b.hold=4;
    else if(!strcmp(n,"retez_timer_off")) b.single=5;
    else if(!strcmp(n,"TV_L_001")) b.single=6;
    else if(!strcmp(n,"TV_L_100")) b.single=13;
    else if(!strncmp(n,"retez_mode_",11) && strlen(n)==12 && n[11]>='1' && n[11]<='8') b.single=6+n[11]-'1';
    else if(!strcmp(n,"retez_timer_4h")) b.single=14;
    else if(!strcmp(n,"retez_timer_6h")) b.single=15;
    else if(!strcmp(n,"retez_timer_8h")) b.single=16;
}
inline uint32_t crc32(const void *data,size_t size) {
    uint32_t crc=~0u; const auto *p=static_cast<const uint8_t*>(data);
    while(size--) { crc^=*p++; for(unsigned bit=0;bit<8;bit++) crc=(crc>>1)^((crc&1)?0xedb88320u:0); }
    return ~crc;
}
// Fixed version/size and CRC prevent partial or incompatible restores. No pointers in storage.
struct BackupHeader { char magic[8]={'I','R','R','E','M','0','3',0}; uint32_t scope=0,size=0,crc=0; };
}
