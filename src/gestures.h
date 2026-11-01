#pragma once
#include "model.h"

namespace control {
enum class Gesture { Single, Double, Long, Repeat };
inline const char *gestureName(Gesture g) {
    return g==Gesture::Single?"single":g==Gesture::Double?"double":g==Gesture::Long?"long":"repeat";
}
using Dispatch=void(*)(const Button&,uint32_t,Gesture,uint32_t,uint32_t,unsigned,bool,void*);
class Gestures {
    Dispatch emit_; void *context_; bool virtual_;
    Button current_{}, waiting_{};
    bool active_=false, fired_=false, long_=false, pending_=false, repeat_=false;
    uint32_t press_=0, waitPress_=0, released_=0, duration_=0;
    unsigned repeats_=0;
    void send(const Button &b,uint32_t command,Gesture g,uint32_t press,uint32_t duration,unsigned steps=1) {
        if(command) emit_(b,command,g,press,duration,steps,virtual_,context_);
    }
    void flush() { if(pending_)send(waiting_,waiting_.single,Gesture::Single,waitPress_,duration_); pending_=false; }
public:
    Gestures(Dispatch fn,bool virt=false,void *context=nullptr):emit_(fn),context_(context),virtual_(virt){}
    void cancel() { active_=pending_=false; }
    void tick(uint32_t now) { if(pending_ && now-released_>=waiting_.doubleMs) flush(); }
    void start(const Button &button,uint32_t press,uint32_t now,bool repeatable) {
        tick(now);
        bool twice=pending_ && waiting_.id==button.id && now-released_<waiting_.doubleMs;
        if(pending_ && !twice) flush();
        current_=button; press_=press; active_=true; fired_=long_=false; repeats_=0; repeat_=repeatable;
        if(twice) { pending_=false; fired_=true; send(current_,current_.twice,Gesture::Double,press_,0); }
        else if(!button.twice && (!button.hold || button.hold==button.single)) {
            send(current_,current_.single,Gesture::Single,press_,0); fired_=true;
        }
    }
    void sample(uint32_t duration) {
        if(!active_ || !current_.hold || duration<current_.longMs || (fired_ && !long_ && current_.twice)) return;
        unsigned count=1+(duration-current_.longMs)/current_.repeatMs;
        if(!long_) { long_=fired_=true; repeats_=count; send(current_,current_.hold,Gesture::Long,press_,duration,repeat_?count:1); }
        else if(repeat_ && count>repeats_) { unsigned delta=count-repeats_; repeats_=count; send(current_,current_.hold,Gesture::Repeat,press_,duration,delta); }
    }
    void finish(uint32_t duration,uint32_t now) {
        if(!active_) return;
        sample(duration); active_=false;
        if(fired_) return;
        if(current_.twice) { waiting_=current_;waitPress_=press_;released_=now;duration_=duration;pending_=true; }
        else send(current_,current_.single,Gesture::Single,press_,duration);
    }
};
}
