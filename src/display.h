#pragma once
#include <TFT_eSPI.h>
#include <cstring>

// One small row buffer (9.6 kB), reused for every change. The LCD never sees
// the erase/text drawing in progress, and unchanged rows generate no SPI traffic.
class StatusDisplay {
public:
    static constexpr unsigned Rows=10, Columns=40;
    using Lines=char[Rows][Columns];
    explicit StatusDisplay(TFT_eSPI &lcd): row(&lcd) {}
    bool begin() {
        row.setColorDepth(16);
        ready=row.createSprite(240,20)!=nullptr;
        row.setTextSize(1); row.setTextWrap(false);
        return ready;
    }
    void draw(const Lines &lines) {
        if(!ready) return;
        for(unsigned i=0;i<Rows;++i) {
            if(drawn && !strcmp(previous[i],lines[i])) continue;
            row.fillSprite(0x0863);
            row.setTextColor(i==0 ? 0x97F2 : TFT_WHITE,0x0863);
            row.setCursor(4,i==0 ? 2 : 1,i==0 ? 2 : 1);
            row.print(lines[i]);
            row.pushSprite(0,i==0 ? 0 : 20+(i-1)*12,0,0,240,i==0 ? 20 : 12);
            memcpy(previous[i],lines[i],Columns);
        }
        drawn=true;
    }
private:
    TFT_eSprite row;
    Lines previous{};
    bool ready=false,drawn=false;
};
