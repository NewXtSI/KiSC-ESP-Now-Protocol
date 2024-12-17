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

#define PROTO_VERSION   0x20

#define MSGTYPE_INFO    0x01

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
    espnowmsg_t msg;
    memcpy(msg.srcAddress, address, sizeof(msg.srcAddress));
    memcpy(msg.payload, data, len);
    msg.payload_len = len;

    KiSCProtoV2Message *kiscmsg = buildProtoMessage(msg);
    if (kiscmsg != nullptr) {
        DBGLOG(Debug, "KiSCProtoV2.dataReceived: Message built");
        messageReceived(kiscmsg, rssi, broadcast);
    }
}

KiSCProtoV2Message* 
KiSCProtoV2::buildProtoMessage(espnowmsg_t msg) {
    if (msg.payload_len < 2) {     // Gibt es nicht mehr
        DBGLOG(Error, "Invalid message length");
        return nullptr;
    }
    if (msg.payload[0] > PROTO_VERSION) {  // Proto 2.0
        DBGLOG(Error, "Unsupported protocol version");
        return nullptr;
    }
    uint8_t msgType = msg.payload[1];
    if (msgType == MSGTYPE_INFO) {
        KiSCProtoV2Message_Info *kiscmsg = new KiSCProtoV2Message_Info(msg.srcAddress);
        kiscmsg->setBufferedMessage(&msg);
        if (kiscmsg->buildFromBuffer()) {
            return kiscmsg;
        } else {
            DBGLOG(Error, "Failed to build message from buffer");            
            delete kiscmsg;
            return nullptr;
        }
    } else {
        DBGLOG(Error, "Unsupported message type %d", msgType);
    }
    return nullptr;
}

void         
KiSCProtoV2::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast) {
    DBGLOG(Debug, "KiSCProtoV2.messageReceived");
    msg->buildFromBuffer();

    if (msg->isA<KiSCProtoV2Message_Info>()) {
        DBGLOG(Debug, "KiSCProtoV2Message_Info");
        KiSCProtoV2Message_Info* infoMsg = dynamic_cast<KiSCProtoV2Message_Info*>(msg);
        infoMsg->dump();
    }

    delete msg;
}

bool 
KiSCProtoV2::init() {
    if (sendQueue == nullptr) {
        sendQueue = xQueueCreate(queueSize, sizeof(espnowmsg_t));
    }
    DBGLOG(Info, "Initializing ESPNow");
    if (state == KiSCPeer::Unknown) {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.disconnect(false, true);
        quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20);

        quickEspNow.begin(2, 0, false);
        quickEspNow.onDataSent(KiSCProtoV2::dataSent);
        quickEspNow.onDataRcvd(KiSCProtoV2::dataReceived);
        // Address auf eigene WiFi MAC setzen
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        address = KiSCAddress(mac);
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
KiSCProtoV2::send(KiSCProtoV2Message *msg) {
    DBGLOG(Debug, "KiSCProtoV2.send()");
    msg->setSource(address);
    
    if (xQueueSend(sendQueue, msg->getBufferedMessage(), 0) == pdTRUE) {
        DBGLOG(Debug, "Message queued (Payloadlength: %d)", msg->getBufferedMessage()->payload_len);
    } else {
        DBGLOG(Warning, "Failed to queue message");
    }

}

void
KiSCProtoV2::_sendViaESPNow(espnowmsg_t msg) {
    DBGLOG(Debug, "KiSCProtoV2._sendViaESPNow, Payloadlength: %d", msg.payload_len);
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
    memcpy(msg.dstAddress, target.getAddress(), sizeof(msg.dstAddress));
    memcpy(msg.srcAddress, source.getAddress(), sizeof(msg.srcAddress));
    memset(msg.payload, 0, sizeof(msg.payload));
    msg.payload[0] = PROTO_VERSION;
    msg.payload_len = 2;
}

bool
KiSCProtoV2Message::buildFromBuffer() {
    DBGLOG(Debug, "KiSCProtoV2Message.buildFromBuffer()");
    memcpy(target.getAddress(), msg.dstAddress, sizeof(msg.dstAddress));
    memcpy(source.getAddress(), msg.srcAddress, sizeof(msg.srcAddress));
    return true;
}

void    
KiSCProtoV2Message::dump() {
    DBGLOG(Debug, "KiSCProtoV2Message.dump()");
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

void
KiSCProtoV2Slave::taskTick500ms() {
    DBGLOG(Debug, "KiSCProtoV2Slave.taskTick500ms()");
    if (!masterFound) {
        KiSCProtoV2Message_Info msg(BroadcastAddress.getAddress());
        send(&msg);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Master
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Master::KiSCProtoV2Master(String name) : KiSCProtoV2(name, KiSCPeer::Master) {
    DBGLOG(Debug, "KiSCProtoV2Master( %s )", name.c_str());
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
    KiSCProtoV2Message_Info msg(BroadcastAddress.getAddress());
    msg.setName(name);
    msg.setRole(role);
    msg.setState(state);
    DBGLOG(Debug, "KiSCProtoV2Master.sendBroadcastOffer()");
    send(&msg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message_Info
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message_Info::KiSCProtoV2Message_Info(uint8_t address[]) : KiSCProtoV2Message(address) {
    DBGLOG(Debug, "KiSCProtoV2Message_Info()");
}
void KiSCProtoV2Message_Info::buildBufferedMessage() {
    DBGLOG(Debug, "KiSCProtoV2Message_Info.buildBufferedMessage()");
//    KiSCProtoV2Message::buildBufferedMessage();
    memcpy(msg.dstAddress, target.getAddress(), sizeof(msg.dstAddress));
    msg.payload[0] = PROTO_VERSION;
    msg.payload[1] = MSGTYPE_INFO;
    msg.payload_len = msg.payload_len;

    // Add payload
    // 0: Version
    // 1: Message Type
    // 2: Rollen ID
    // 3: State
    // 4-13: Name
    
    msg.payload[2] = role;
    msg.payload[3] = state;
    for (int i = 0; i < name.length(); i++) {
        msg.payload[4+i] = name[i];
    }
    msg.payload_len = 4 + name.length();
}

bool
KiSCProtoV2Message_Info::buildFromBuffer() {
    DBGLOG(Debug, "KiSCProtoV2Message_Info.buildFromBuffer()");
    KiSCProtoV2Message::buildFromBuffer();
    // Dump payload in lines to 16 Bytes

    char line[255];
    sprintf(line, "Payload: %d", msg.payload_len);
    for (int i=0; i<msg.payload_len; i++) {
        sprintf(line, "%s %02X", line, msg.payload[i]);
    }
    DBGLOG(Debug, line);
    
    memcpy(target.getAddress(), msg.dstAddress, sizeof(msg.dstAddress));
    if (msg.payload_len < 2) {
        return false;
    }
    if (msg.payload[1] != MSGTYPE_INFO) {
        return false;
    }
    role = (KiSCPeer::Role)msg.payload[2];
    state = (KiSCPeer::State)msg.payload[3];
    name = "";
    for (int i = 4; i < msg.payload_len; i++) {
        name += (char)msg.payload[i];
    }
    return true;
}

void
KiSCProtoV2Message_Info::dump() {
    DBGLOG(Debug, "From %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
    DBGLOG(Debug, "To %02X %02X %02X %02X %02X %02X", target.getAddress()[0], target.getAddress()[1], target.getAddress()[2], target.getAddress()[3], target.getAddress()[4], target.getAddress()[5]);
    DBGLOG(Debug, "  Name: %s", name.c_str());
    DBGLOG(Debug, "  Role: %d", role);
    DBGLOG(Debug, "  State: %d", state);

}