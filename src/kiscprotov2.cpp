#include <Arduino.h>
#include "../include/kiscprotov2.h"

#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#else
#error "Unsupported platform"
#endif  // ESP32

#include <QuickEspNow.h>

KiSCProtoV2::KiSCProtoV2(String name, KiSCPeer::Role role) : name(name), role(role) {
    
}

bool KiSCProtoV2::init() {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(false, true);
    quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20);

    quickEspNow.begin(1, 0, false);
    quickEspNow.onDataSent(KiSCProtoV2::dataSent);
    quickEspNow.onDataRcvd(KiSCProtoV2::dataReceived);
    return true;
}

void KiSCProtoV2::start() {
    if (init()) {
        state = KiSCPeer::Idle;
    }
}

