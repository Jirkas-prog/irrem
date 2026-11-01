#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>

namespace remote {
constexpr unsigned MaxKeys = 64, MaxRaw = 200, KeySize = 144, NameSize = 49;
struct Signal {
    char key[KeySize] = {};
    uint16_t raw[MaxRaw] = {};
    uint16_t count = 0;
    bool repeat = false, toggleValid = false, toggle = false;
};
struct Mapping { Signal signal; char name[NameSize] = {}; };
inline bool matches(const Signal &a, const Signal &b) {
    if (!a.count || !b.count) return !a.count && !b.count && !strcmp(a.key, b.key);
    if (a.count != b.count) return false;
    // Unknown protocols: compare the complete envelope, not just a 32-bit hash.
    for (unsigned i = 0; i < a.count; ++i) {
        unsigned x = a.raw[i], y = b.raw[i], d = x > y ? x-y : y-x;
        if (d > 100 + x / 4) return false;
    }
    return true;
}
inline bool validName(const char *s) {
    size_t n = strlen(s);
    if (!n || n >= NameSize || s[0] == ' ' || s[n-1] == ' ') return false;
    for (size_t i=0; i<n; ++i) if ((unsigned char)s[i]<32 || s[i]==127) return false;
    return true;
}
enum class Action { Press, Hold, Release };
inline const char *actionName(Action a) {
    return a==Action::Press ? "press" : a==Action::Hold ? "hold" : "release";
}
struct Event {
    Action action;
    char key[KeySize], name[NameSize];
    uint32_t pressId, sequence, at, duration, repeats;
};
using Emit = void (*)(const Event &, void *);
class Engine {
public:
    uint32_t releaseMs=180, holdMs=100;
    bool active=false;
    uint32_t started=0, last=0, duration=0, repeats=0, pressId=0, sequence=0;
    Signal signal;
    char name[NameSize] = {};
    Engine(Emit emit, void *context=nullptr): emit_(emit), context_(context) {}
    void frame(const Signal &s, const char *mappedName, uint32_t now) {
        tick(now);
        if (s.repeat) {
            // A short repeat has no address: only attach it to a live same-protocol press.
            if (!active || strcmp(s.key, signal.key)) return;
        } else if (active && (!matches(s,signal) ||
                   (s.toggleValid && signal.toggleValid && s.toggle!=signal.toggle))) {
            release(now);
        }
        if (!active) {
            if (s.repeat) return;
            active=true; signal=s; started=last=lastHold_=now; duration=repeats=0; ++pressId;
            snprintf(name,sizeof(name),"%s",mappedName ? mappedName : "");
            send(Action::Press, now);
        } else {
            last=now; duration=now-started; ++repeats;
            if (now-lastHold_>=holdMs) { lastHold_=now; send(Action::Hold,now); }
        }
    }
    void tick(uint32_t now) { if (active && now-last>=releaseMs) release(now); }
    void release(uint32_t now) {
        if (!active) return;
        // Exclude the silence timeout: this is observed IR duration, not exact finger time.
        duration=last-started; active=false; send(Action::Release, now);
    }
private:
    Emit emit_; void *context_; uint32_t lastHold_=0;
    void send(Action action,uint32_t now) {
        Event event{}; event.action=action; event.pressId=pressId; event.sequence=++sequence;
        event.at=now; event.duration=duration; event.repeats=repeats;
        snprintf(event.key,sizeof(event.key),"%s",signal.key);
        snprintf(event.name,sizeof(event.name),"%s",name);
        emit_(event,context_);
    }
};
}
