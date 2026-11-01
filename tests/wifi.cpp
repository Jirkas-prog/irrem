#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>

constexpr int WIFI_STA=1,WIFI_AP=2,WIFI_AP_STA=3,WL_CONNECTED=3,TFT_BL=4,HIGH=1;
static std::atomic<bool> apActive{false};
static bool displayOn=false;
static std::string apName="IRrem-test";
static struct { const char *hostname="irrem"; const char *ssid="home"; } settings;
static void digitalWrite(int,int) {}
static struct {
    int state=0,currentMode=WIFI_STA,starts=0,stops=0,clients=0;
    bool startOk=true,modeOk=true,open=false;
    int status() { return state; }
    int getMode() { return currentMode; }
    bool mode(int value) { ++stops;if(modeOk)currentMode=value;return modeOk; }
    bool softAP(const char *ssid,const char *password=nullptr) {
        assert(ssid==apName);++starts;currentMode=WIFI_AP_STA;
        open=!password || !password[0];return startOk;
    }
} WiFi;
static struct {
    bool active=false;
    bool begin(const char*) { active=true;return true; }
    void end() { active=false; }
    void addService(const char*,const char*,int) {}
} MDNS;
static struct {
    void printf(const char*,const char*) {}
    void println(const char*) {}
} Serial;
#include "../src/fallback_wifi.h"

int main() {
    updateWifi(0);updateWifi(29999);
    assert(!apActive && WiFi.currentMode==WIFI_STA && !WiFi.starts);
    updateWifi(30000);
    assert(apActive && WiFi.open && displayOn && WiFi.currentMode==WIFI_AP_STA);
    updateWifi(30001);assert(WiFi.starts==1);
    // Reconnection immediately turns off the AP, even with an AP client.
    WiFi.clients=1;WiFi.state=WL_CONNECTED;updateWifi(30002);
    assert(!apActive && WiFi.currentMode==WIFI_STA && MDNS.active);
    startAp();assert(!apActive && WiFi.starts==1); // Manual setup cannot override a working STA.
    WiFi.state=0;updateWifi(40000);updateWifi(69999);
    assert(!apActive && !MDNS.active);
    WiFi.state=WL_CONNECTED;updateWifi(69999); // Brief dropout resets the delay.
    WiFi.state=0;updateWifi(70000);updateWifi(99999);assert(!apActive);
    updateWifi(100000);assert(apActive && WiFi.starts==2 && WiFi.open);
    // A failed mode change is retried; an untracked/default AP is also disabled.
    WiFi.state=WL_CONNECTED;WiFi.modeOk=false;updateWifi(100001);assert(apActive);
    WiFi.modeOk=true;updateWifi(100002);assert(!apActive && WiFi.currentMode==WIFI_STA);
    WiFi.currentMode=WIFI_AP_STA;updateWifi(100003);assert(WiFi.currentMode==WIFI_STA);
    // Manual fallback is available offline, without waiting 30 seconds.
    WiFi.state=0;startAp();assert(apActive && WiFi.open);
    WiFi.state=WL_CONNECTED;updateWifi(100004);assert(!apActive);
    // No configured SSID: open AP immediately; failed start never leaves a default AP.
    settings.ssid="";WiFi.state=0;WiFi.startOk=false;updateWifi(100005);
    assert(!apActive && WiFi.currentMode==WIFI_STA);
    WiFi.startOk=true;updateWifi(100006);assert(apActive && WiFi.open);
    WiFi.state=WL_CONNECTED;updateWifi(100007);settings.ssid="home";
    // The 30-second timeout also works across millis() rollover.
    WiFi.state=0;uint32_t lost=0xfffffff0u;updateWifi(lost);
    updateWifi(lost+29999u);assert(!apActive);
    updateWifi(lost+30000u);assert(apActive && WiFi.open);
    puts("Wi-Fi fallback: delay, reconnect, manual guard, open AP, failures and rollover OK");
}
