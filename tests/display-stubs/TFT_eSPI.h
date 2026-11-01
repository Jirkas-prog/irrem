#pragma once
#include <cassert>
#include <string>
#include <vector>
constexpr unsigned TFT_WHITE=0xffff;
struct TFT_eSPI {};
struct DrawnRow { int y,height; std::string text; };
static std::vector<DrawnRow> transfers;
static bool allocationFails=false;
static unsigned allocations=0;
// No direct LCD drawing methods: accidental live clears fail to compile.
struct TFT_eSprite {
    explicit TFT_eSprite(TFT_eSPI*) {}
    std::string text;
    bool cleared=false;
    void setColorDepth(int depth) { assert(depth==16); }
    void *createSprite(int w,int h) {
        assert(w==240 && h==20); ++allocations;
        return allocationFails ? nullptr : this;
    }
    void setTextSize(int size) { assert(size==1); }
    void setTextWrap(bool wrap) { assert(!wrap); }
    void fillSprite(unsigned) { cleared=true; text.clear(); }
    void setTextColor(unsigned,unsigned) {}
    void setCursor(int,int,int) {}
    void print(const char *value) { assert(cleared); text=value; }
    bool pushSprite(int x,int y,int sx,int sy,int w,int h) {
        assert(cleared && x==0 && sx==0 && sy==0 && w==240 && y+h<=135);
        transfers.push_back({y,h,text}); cleared=false; return true;
    }
};
