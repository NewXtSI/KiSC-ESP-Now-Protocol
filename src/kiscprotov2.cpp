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

bool KiSCProtoV2::ESPNowSent = true;
QueueHandle_t KiSCProtoV2::sendQueue = nullptr	;

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
    if (sendQueue == nullptr) {
        sendQueue = xQueueCreate(queueSize, sizeof(espnowmsg_t));
    }
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
    KiSCProtoV2* proto = (KiSCProtoV2*)param;
    uint32_t last100ms = millis();
    uint32_t last500ms = millis();
    uint32_t last1s = millis();
    DBGLOG(Info, "Starting KiSCCommTask");
    for (;;) {
        if (ESPNowSent) {
            espnowmsg_t msg;
            if (xQueueReceive(sendQueue, &msg, 0)) {
                _sendViaESPNow(msg);
            }
        }
        // run 100ms tick every 100ms
        if (millis() - last100ms > 100) {
            proto->taskTick100ms();
            last100ms = millis();
        }
        // run 500ms tick every 500ms
        if (millis() - last500ms > 500) {
            proto->taskTick500ms();
            last500ms = millis();
        }
        // run 1s tick every 1s
        if (millis() - last1s > 1000) {
            proto->taskTick1s();
            last1s = millis();
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
        xTaskCreate(task, "KiSCCommTast", 1024*6, this, 0, NULL);
    } else {
        DBGLOG(Error, "Failed to initialize ESPNow");
    }
}

void
KiSCProtoV2::send(KiSCProtoV2Message msg) {
    DBGLOG(Debug, "KiSCProtoV2.send()");
    if (xQueueSend(sendQueue, msg.getBufferedMessage(), 0) == pdTRUE) {
        DBGLOG(Debug, "Message queued");
    } else {
        DBGLOG(Warning, "Failed to queue message");
    }

}

void
KiSCProtoV2::_sendViaESPNow(espnowmsg_t msg) {
    DBGLOG(Debug, "KiSCProtoV2._sendViaESPNow");
    ESPNowSent = false;
    quickEspNow.send(msg.dstAddress, msg.payload, msg.payload_len);
}

void                
KiSCProtoV2::taskTick100ms() {
//    DBGLOG(Debug, "KiSCProtoV2.taskTick100ms()");
}

void
KiSCProtoV2::taskTick500ms() {
    DBGLOG(Debug, "KiSCProtoV2.taskTick500ms()");
}

void
KiSCProtoV2::taskTick1s() {
    DBGLOG(Debug, "KiSCProtoV2.taskTick1s()");
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
    broadcastActive = true;
}

void                    
KiSCProtoV2Master::taskTick500ms() {
    DBGLOG(Debug, "KiSCProtoV2Master.taskTick500ms()");
    if (broadcastActive) {
        sendBroadcastOffer();
    }
}

void
KiSCProtoV2Master::sendBroadcastOffer() {
    KiSCProtoV2Message msg(BroadcastAddress.getAddress());
    DBGLOG(Debug, "KiSCProtoV2Master.sendBroadcastOffer()");
    send(msg);
}