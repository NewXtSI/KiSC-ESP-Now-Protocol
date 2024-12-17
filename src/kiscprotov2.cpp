#include <Arduino.h>
#include "../include/kiscprotov2.h"

#define ESP32DEBUGGING
#include <ESP32Logger.h>

#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#else
#error "Unsupported platform"
#endif  // ESP32

#include <QuickEspNow.h>

KiSCProtoV2::KiSCProtoV2(String name, KiSCPeer::Role role) : name(name), role(role) {

}

void
KiSCProtoV2::dataSent(uint8_t* address, uint8_t status) {
    ESPNowSent = true;
}

void
KiSCProtoV2::dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {

}

bool KiSCProtoV2::init() {
    if (state == KiSCPeer::Unknown) {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.disconnect(false, true);
        quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20);

        quickEspNow.begin(1, 0, false);
        quickEspNow.onDataSent(KiSCProtoV2::dataSent);
        quickEspNow.onDataRcvd(KiSCProtoV2::dataReceived);
    }
    return true;
}

void
KiSCProtoV2::task(void* param) {
    for (;;) {
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void
KiSCProtoV2::start() {
    if (init()) {
        state = KiSCPeer::Idle;
        ESPNowSent = true;
        xTaskCreate(task, "KiSCCommTast", 4096, NULL, 0, NULL);
    }
}

void
KiSCProtoV2::send(KiSCProtoV2Message msg) {
}

void
KiSCProtoV2::_sendViaESPNow(KiSCProtoV2Message msg) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message::KiSCProtoV2Message(uint8_t* address[], uint8_t* data, uint8_t len) {

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Slave
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
KiSCProtoV2Slave::dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    KiSCProtoV2::dataReceived(address, data, len, rssi, broadcast);
}

