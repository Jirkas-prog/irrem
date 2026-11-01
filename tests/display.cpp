#include "../src/display.h"
#include <iostream>
int main() {
    TFT_eSPI lcd;
    StatusDisplay display(lcd);
    StatusDisplay::Lines lines{};
    strcpy(lines[0],"IRrem 0.2.2"); strcpy(lines[4],"IR: KLID");
    strcpy(lines[8],"Heslo: example-password");
    assert(display.begin()); display.draw(lines);
    assert(allocations==1 && transfers.size()==10);
    assert(transfers.front().height==20 && transfers.back().y==116);
    transfers.clear();
    for(int i=0;i<100;++i) display.draw(lines);
    assert(transfers.empty() && allocations==1); // no periodic redraw or allocation
    strcpy(lines[4],"IR: PRIJEM"); display.draw(lines);
    assert(transfers.size()==1 && transfers[0].y==56 && transfers[0].text=="IR: PRIJEM");
    transfers.clear();
    strcpy(lines[4],"IR: KLID"); lines[8][0]=0; display.draw(lines);
    assert(transfers.size()==2 && transfers[0].text=="IR: KLID" && transfers[1].text.empty());
    transfers.clear();
    allocationFails=true; StatusDisplay failed(lcd);
    assert(!failed.begin()); failed.draw(lines); assert(transfers.empty());
    std::cout << "LCD: buffered rows, no unchanged redraw, shorter/cleared text, bounds and allocation failure OK\n";
}
