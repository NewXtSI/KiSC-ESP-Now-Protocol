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

bool KiSCProtoV2::ESPNowSent = false;
QueueHandle_t KiSCProtoV2::sendQueue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnowmsg_t));

KiSCAddress BroadcastAddress(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

KiSCProtoV2::KiSCProtoV2(String name, KiSCPeer::Role role) : name(name), role(role) {
    DBGLOG(Debug, "KiSCProtoV2()");
}

void
KiSCProtoV2::dataSent(uint8_t* address, uint8_t status) {
    DBGLOG(Debug, "KiSCProtoV2.dataSent %d", status);
    ESPNowSent = true;
}

void
KiSCProtoV2::dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    DBGLOG(Debug, "KiSCProtoV2.dataReceived %d, Broadcast: %d", len, broadcast);
}

bool KiSCProtoV2::init() {
    DBGLOG(Info, "Initializing ESPNow");
    if (state == KiSCPeer::Unknown) {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.disconnect(false, true);
        quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20);

        quickEspNow.begin(1, 0, false);
        quickEspNow.onDataSent(KiSCProtoV2::dataSent);
        quickEspNow.onDataRcvd(KiSCProtoV2::dataReceived);
    } else {
        DBGLOG(Warning, "ESPNow already initialized");
    }
    return true;
}

void
KiSCProtoV2::task(void* param) {
    DBGLOG(Info, "Starting KiSCCommTask");
    for (;;) {
        if (ESPNowSent) {
            espnowmsg_t msg;
            if (xQueueReceive(sendQueue, &msg, 0)) {
                _sendViaESPNow(msg);
            }
        }
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void
KiSCProtoV2::start() {
    DBGLOG(Info, "Starting KiSCProtoV2");

    if (init()) {
        state = KiSCPeer::Idle;
        ESPNowSent = true;
        xTaskCreate(task, "KiSCCommTast", 4096, NULL, 0, NULL);
    } else {
        DBGLOG(Error, "Failed to initialize ESPNow");
    }
}

void
KiSCProtoV2::send(KiSCProtoV2Message msg) {
    DBGLOG(Debug, "KiSCProtoV2.send()");
    if (queueSize > uxQueueSpacesAvailable(sendQueue)) {
        DBGLOG(Warning, "Send queue full");
    } else {
        DBGLOG(Debug, "Pushing to queue...");
        xQueueSend(sendQueue, msg.getBufferedMessage(), 0);
    }
}

void
KiSCProtoV2::_sendViaESPNow(espnowmsg_t msg) {
    DBGLOG(Debug, "KiSCProtoV2._sendViaESPNow");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message::KiSCProtoV2Message(uint8_t address[]) {
    DBGLOG(Debug, "KiSCProtoV2Message()");
    this->target = KiSCAddress(address);
}

void            
KiSCProtoV2Message::buildBufferedMessage() {
    DBGLOG(Debug, "KiSCProtoV2Message.buildBufferedMessage()");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Slave
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Slave::KiSCProtoV2Slave(String name) : KiSCProtoV2(name, KiSCPeer::Slave) {
    DBGLOG(Debug, "KiSCProtoV2Slave()");
}

void
KiSCProtoV2Slave::dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    KiSCProtoV2::dataReceived(address, data, len, rssi, broadcast);
    DBGLOG(Debug, "KiSCProtoV2Slave::dataReceived");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Master
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Master::KiSCProtoV2Master(String name) : KiSCProtoV2(name, KiSCPeer::Master) {
    DBGLOG(Debug, "KiSCProtoV2Master()");
}
