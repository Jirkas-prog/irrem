#pragma once
// Included by main.cpp after the shared Wi-Fi/display state.
static void startAp() {
    if(apActive || WiFi.status()==WL_CONNECTED) return;
    apActive=WiFi.softAP(apName.c_str());
    if(apActive) { displayOn=true; digitalWrite(TFT_BL,HIGH); }
    else WiFi.mode(WIFI_STA);
    Serial.printf("Fallback Wi-Fi AP (open): %s\n",apActive ? "ready" : "failed");
}
static void updateWifi(uint32_t now) {
    static uint32_t disconnectedAt=0;
    static bool disconnected=false,mdns=false;
    if(WiFi.status()==WL_CONNECTED) {
        disconnected=false;
        // Disable the AP interface itself, including any restored/default SSID.
        if((WiFi.getMode() & WIFI_AP) && WiFi.mode(WIFI_STA)) {
            apActive=false;
            Serial.println("Fallback Wi-Fi AP: off (station connected)");
        }
        if(!mdns) { mdns=MDNS.begin(settings.hostname); if(mdns) MDNS.addService("http","tcp",80); }
    } else {
        if(mdns) { MDNS.end(); mdns=false; }
        if(!disconnected) { disconnectedAt=now; disconnected=true; }
        if(!settings.ssid[0] || now-disconnectedAt>=30000) startAp();
    }
}
