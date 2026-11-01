#define _CRT_SECURE_NO_WARNINGS
#include "../src/gestures.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace control;
struct Sent{uint32_t id,command,press,duration;Gesture gesture;unsigned steps;};
static std::vector<Sent> sent;
static void emit(const Button &b,uint32_t c,Gesture g,uint32_t p,uint32_t d,unsigned n,bool,void*){sent.push_back({b.id,c,p,d,g,n});}
int main(){
    auto store=new Store;defaults(store->catalog);strcpy(store->apPassword,"12345678");
    assert(valid(*store));store->apPassword[0]=0;assert(valid(*store));
    strcpy(store->apPassword,"short");assert(!valid(*store));strcpy(store->apPassword,"12345678");
    Button b;b.id=store->catalog.nextId++;strcpy(b.name,"retez_dim_plus");assignLegacy(b);
    store->catalog.items[store->catalog.count++]=b;assert(valid(*store));assert(b.single==3&&b.hold==3);
    assert(commandValid("white/0.1/2000")&&commandValid("rgb/20/255/180/100"));
    for(const char *s:{"white/101/3000","rgb/1/999/0/0","white/nan/3000","white/1/3000/extra","white/ 1/3000","shell/reboot"})assert(!commandValid(s));
    {Gestures g(emit);sent.clear();g.start(b,1,0,true);assert(sent.size()==1);g.sample(999);assert(sent.size()==1);
     g.sample(1000);assert(sent.size()==2&&sent.back().gesture==Gesture::Long);g.sample(1349);assert(sent.size()==2);
     g.sample(1350);assert(sent.size()==3&&sent.back().steps==1);g.finish(1350,1530);g.tick(10000);assert(sent.size()==3);}
    b.single=1;b.twice=13;b.hold=14;
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.finish(0,180);g.tick(529);assert(sent.empty());g.tick(530);assert(sent.size()==1&&sent[0].command==1);}
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.finish(0,180);g.start(b,2,529,false);g.finish(10,719);g.tick(2000);
     assert(sent.size()==1&&sent[0].command==13&&sent[0].gesture==Gesture::Double);}
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.finish(0,180);g.start(b,2,530,false);g.finish(0,710);g.tick(1060);assert(sent.size()==2&&sent[0].command==1&&sent[1].command==1);}
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.sample(999);assert(sent.empty());g.sample(1000);g.sample(3000);g.finish(3000,3180);g.tick(4000);assert(sent.size()==1&&sent[0].command==14);}
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.finish(0,180);g.cancel();g.tick(1000);assert(sent.empty());}
    {Gestures g(emit);sent.clear();g.start(b,1,0xffffff00u,false);g.finish(0,0xffffffb4u);g.tick(0x111u);assert(sent.empty());g.tick(0x112u);assert(sent.size()==1);}
    b.twice=0;b.hold=0;
    {Gestures g(emit);sent.clear();
     for(unsigned p=1;p<=5;++p){g.start(b,p,p*200,false);assert(sent.size()==p);g.finish(0,p*200+180);}
     g.start(b,6,1200,false);g.sample(5000);g.finish(5000,6380);g.tick(7000);
     assert(sent.size()==6);for(const auto &e:sent)assert(e.gesture==Gesture::Single&&e.command==1);}
    b.hold=14;
    {Gestures g(emit);sent.clear();
     for(unsigned p=1;p<=3;++p){g.start(b,p,p*200,false);g.finish(0,p*200+180);assert(sent.size()==p);}
     for(const auto &e:sent)assert(e.gesture==Gesture::Single);
     g.start(b,4,800,false);g.sample(999);assert(sent.size()==3);g.sample(1000);g.finish(1000,1980);
     assert(sent.size()==4&&sent.back().gesture==Gesture::Long);}
    b.hold=0;b.twice=13;
    {Gestures g(emit);sent.clear();g.start(b,1,0,false);g.sample(5000);assert(sent.empty());
     g.finish(5000,5180);g.tick(5530);assert(sent.size()==1&&sent.back().gesture==Gesture::Single);
     g.start(b,2,6000,false);g.finish(0,6180);g.start(b,3,6200,false);g.sample(5000);g.finish(5000,11380);
     assert(sent.size()==2&&sent.back().gesture==Gesture::Double);}
    const auto checksum=crc32(&store->catalog,sizeof(store->catalog));store->catalog.items[0].twice=999;
    assert(!valid(*store)&&checksum!=crc32(&store->catalog,sizeof(store->catalog)));store->catalog.items[0].twice=0;
    assert(valid(*store));store->catalog.items[0].signal.count=201;assert(!valid(*store));
    std::cout<<"Gestures: exclusive single/double/long, 999/1000 ms, repeat, cancellation, wraparound, migration, command validation, CRC OK; store "<<sizeof(Store)<<" bytes\n";
    delete store;
}
