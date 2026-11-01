#include "../src/remote.h"
#include <assert.h>
#include <vector>
#include <iostream>
using namespace remote;
static std::vector<Event> events;
static void emit(const Event &e,void*) { events.push_back(e); }
static Signal code(const char *key,bool repeat=false) {
    Signal s; snprintf(s.key,sizeof(s.key),"%s",key); s.repeat=repeat; return s;
}
int main() {
    Engine e(emit); auto a=code("NEC:32:A"); auto r=code("NEC:32:A",true);
    e.frame(r,"up",0); assert(events.empty()); // orphan repeat
    e.frame(a,"up",1000);
    for (unsigned t=1100;t<=3000;t+=100) e.frame(r,"up",t);
    assert(e.active && e.duration==2000 && events[0].action==Action::Press);
    e.tick(3179); assert(e.active); e.tick(3180); assert(!e.active);
    assert(events.back().action==Action::Release && events.back().duration==2000);
    assert(events.back().repeats==20 && events.back().pressId==events[0].pressId);
    e.frame(r,"up",3200); assert(!e.active);
    e.frame(a,"up",3300); assert(events.back().action==Action::Press && e.pressId==2);
    e.frame(code("SONY:12:B"),"down",3340);
    assert(events[events.size()-2].action==Action::Release && events.back().action==Action::Press);
    e.frame(r,"up",3380); assert(e.repeats==0); // unrelated repeat cannot extend Sony
    e.tick(3520); assert(!e.active);
    // Full-frame repeats (NEC2/Sony), including a changed mapping during a press.
    e.frame(a,"original",3600); e.frame(a,"renamed",3700);
    assert(e.repeats==1 && !strcmp(events.back().name,"original"));
    e.release(3710);
    auto rc5=code("RC5:12:10"); rc5.toggleValid=true;
    e.frame(rc5,"tv",3800); auto id=e.pressId; rc5.toggle=true; e.frame(rc5,"tv",3840);
    assert(e.pressId==id+1); e.release(3850);
    e.frame(a,"up",UINT32_MAX-50); e.frame(r,"up",49); assert(e.duration==100);
    e.tick(228); assert(e.active); e.tick(229); assert(!e.active);
    Signal raw1=code("UNKNOWN:A"),raw2=code("UNKNOWN:B");
    raw1.count=raw2.count=4;
    for(unsigned i=0;i<4;i++){ raw1.raw[i]=560; raw2.raw[i]=630; }
    assert(matches(raw1,raw2)); raw2.raw[2]=1690; assert(!matches(raw1,raw2));
    assert(validName("Čtení +")); assert(!validName("")); assert(!validName(" bad"));
    assert(!validName("bad\nname"));
    std::cout << "IR engine: repeats, release, switching, toggle, rollover, raw matching OK\n";
}
