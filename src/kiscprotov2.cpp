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

#define MSGTYPE_INFO            0x01
#define MSGTYPE_NETWORK         0x10
#define MSGTYPE_NETWORK_JOIN    0x11
#define MSGTYPE_NETWORK_LEAVE   0x12
#define MSGTYPE_NETWORK_ACCEPT  0x13
#define MSGTYPE_NETWORK_REJECT  0x14

KiSCProtoV2 *kiscprotoV2;

KiSCProtoV2::KiSCProtoV2(String name, KiSCPeer::Role role) : name(name), role(role) {
    DBGLOG(Debug, "KiSCProtoV2()");
    kiscprotoV2 = this;
    mutex = xSemaphoreCreateMutex();
}

void
KiSCProtoV2::dataSent(uint8_t* address, uint8_t status) {
    DBGLOG(Debug, "KiSCProtoV2.dataSent %d", status);
    ESPNowSent = true;
}

void hexdump(void *ptr, int buflen, bool in) {
  unsigned char *buf = (unsigned char*)ptr;
  int i, j;
  char line[255];
  for (i=0; i<buflen; i+=16) {
    sprintf(line, "%s %06x: ", in?"IN ":"OUT", i);
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        sprintf(line, "%s%02x ", line, buf[i+j]);
      else
        sprintf(line,"%s   ", line);
    sprintf(line,"%s ",line);
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        sprintf(line,"%s%c", line,isprint(buf[i+j]) ? buf[i+j] : '.');
     DBGLOG(Info, line);
  }
}

void dmp(uint8_t* data, uint8_t len, bool incoming) {
    hexdump(data, len, incoming);
        // Create lines of Hexdump with 8 bytes per line with following char (if printable)
#if 0        
    char line[255];
    sprintf(line, "%s Payload: %d |", incoming ? "IN " : "OUT", len);
    for (int i=0; i<len; i++) {
        sprintf(line, "%s %02X", line, data[i]);
    }
    DBGLOG(Info, line);
#endif    

}

void
KiSCProtoV2::dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    dmp(data, len, true);
    DBGLOG(Debug, "KiSCProtoV2.dataReceived %d, Broadcast: %d", len, broadcast);
    espnowmsg_t msg;
    memcpy(msg.srcAddress, address, sizeof(msg.srcAddress));
    if (broadcast) { 
        memcpy(msg.dstAddress, BroadcastAddress.getAddress(), sizeof(msg.dstAddress));
    } else {
        memcpy(msg.dstAddress, kiscprotoV2->getAddress().getAddress(), sizeof(msg.dstAddress));
    }
    memcpy(msg.payload, data, len);
    msg.payload_len = len;

    KiSCProtoV2Message *kiscmsg = buildProtoMessage(msg);
    if (kiscmsg != nullptr) {
        kiscprotoV2->messageReceived(kiscmsg, rssi, broadcast);
    } else {
        char line[255];
        sprintf(line, "Payload: %d", msg.payload_len);
        for (int i=0; i<msg.payload_len; i++) {
            sprintf(line, "%s %02X", line, msg.payload[i]);
        }
        DBGLOG(Warning, line);
    }
}

KiSCProtoV2Message* 
KiSCProtoV2::buildProtoMessage(espnowmsg_t msg) {
    if (msg.payload_len < 2) {     // Gibt es nicht mehr
        DBGLOG(Error, "Invalid message length");
        return nullptr;
    }
    if (msg.payload[0] > PROTO_VERSION) {  // Proto 2.0
        DBGLOG(Error, "Unsupported protocol version (%02X)", msg.payload[0]);
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
    } else if ((msgType == MSGTYPE_NETWORK) || (msgType == MSGTYPE_NETWORK_JOIN) || (msgType == MSGTYPE_NETWORK_LEAVE) || (msgType == MSGTYPE_NETWORK_ACCEPT) || (msgType == MSGTYPE_NETWORK_REJECT)) {
        KiSCProtoV2Message_network *kiscmsg = new KiSCProtoV2Message_network(msg.srcAddress);
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
KiSCProtoV2::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast, bool delBuffer) {
    DBGLOG(Debug, "KiSCProtoV2.messageReceived");
    msg->buildFromBuffer();

    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = dynamic_cast<KiSCProtoV2Message_Info*>(msg);
        infoMsg->dump();
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
        KiSCProtoV2Message_network* networkMsg = dynamic_cast<KiSCProtoV2Message_network*>(msg);
        networkMsg->dump();
    } else {
        DBGLOG(Error, "Unknown message type");
    }
    if (delBuffer)
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
    dmp(msg.payload, msg.payload_len, false);
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
bool KiSCAddress::operator==(const KiSCAddress& other) const {

    return memcmp(address, other.address, sizeof(address)) == 0;

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
    msg.payload[1] = 0xFF;
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
KiSCProtoV2Slave::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast, bool delBuffer) {
    KiSCProtoV2::messageReceived(msg, rssi, broadcast, false);
    if ((!broadcast) && (msg->getSource() == master.address)) {
        master.lastMsg = millis();
    }
    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = dynamic_cast<KiSCProtoV2Message_Info*>(msg);
        if (!(dynamic_cast<KiSCProtoV2Slave*>(kiscprotoV2))->isMasterFound()) {
            if (infoMsg->getRole() == KiSCPeer::Role::Master) {
                // Send join Request
                KiSCProtoV2Message_network joinMsg(infoMsg->getSource().getAddress());
                joinMsg.setJoinRequest();
                joinMsg.setName(name);
                joinMsg,setType(type);
                kiscprotoV2->send(&joinMsg);
                DBGLOG(Info, "Join request sent to %02X %02X %02X %02X %02X %02X", infoMsg->getSource().getAddress()[0], infoMsg->getSource().getAddress()[1], infoMsg->getSource().getAddress()[2], infoMsg->getSource().getAddress()[3], infoMsg->getSource().getAddress()[4], infoMsg->getSource().getAddress()[5]);
            } else {
                DBGLOG(Warning, "No master, but no master in message");
            }
//            masterFound = true;
//            master = KiSCPeer(infoMsg->source, infoMsg->name, infoMsg->role, infoMsg->state, infoMsg->type);
        } else {
            if (infoMsg->getRole() == KiSCPeer::Role::Master) {
                DBGLOG(Info, "Master name setting to: %s", infoMsg->getName().c_str());
                if (infoMsg->getName() != master.name) {
                    DBGLOG(Info, "Master name changed to: %s", infoMsg->getName().c_str());
                    master.name = infoMsg->getName();
                }
//                (dynamic_cast<KiSCProtoV2Slave*>(kiscprotoV2))->getMaster()->name = infoMsg->getName();
            }
        }
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
//        DBGLOG(Verbose, "Network message");
        KiSCProtoV2Message_network* networkMsg = dynamic_cast<KiSCProtoV2Message_network*>(msg);
        if (networkMsg->getSubCommand() == MSGTYPE_NETWORK_ACCEPT) {
            if (!(dynamic_cast<KiSCProtoV2Slave*>(kiscprotoV2))->isMasterFound()) {
                DBGLOG(Info, "KiSCProtoV2Slave.messageReceived: Accept Response from %02X %02X %02X %02X %02X %02X", msg->getSource().getAddress()[0], msg->getSource().getAddress()[1], msg->getSource().getAddress()[2], msg->getSource().getAddress()[3], msg->getSource().getAddress()[4], msg->getSource().getAddress()[5]);
                KiSCPeer _master;
                _master.address = networkMsg->getSource();
                _master.lastMsg = millis();
                _master.active = true;
                dynamic_cast<KiSCProtoV2Slave*>(kiscprotoV2)->setMaster(_master);
            }
        }
    } else {
        DBGLOG(Warning, "Unknown message type %02X", msg->getCommand());
    }
    if (delBuffer)
        delete msg;
}

void
KiSCProtoV2Slave::taskTick1s() {
    KiSCProtoV2::taskTick1s();
    DBGLOG(Debug, "KiSCProtoV2Slave.taskTick1s()");
    if (masterFound) {
        if (millis() - master.lastMsg > 10000) {
            DBGLOG(Warning, "Master not responding");
            masterFound = false;
        } else {
            KiSCProtoV2Message_Info msg(master.address.getAddress());
            msg.setName(name);
            msg.setRole(role);
            msg.setState(state);
            msg.setType(type);

            send(&msg);

        }

    }
}

void
KiSCProtoV2Slave::taskTick500ms() {
    DBGLOG(Debug, "KiSCProtoV2Slave.taskTick500ms()");
    if (!masterFound) {
        KiSCProtoV2Message_Info msg(BroadcastAddress.getAddress());
        msg.setName(name);
        msg.setRole(role);
        msg.setState(state);
        msg.setType(type);

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
    broadcastStart = millis();
}

void
KiSCProtoV2Master::taskTick1s() {
    KiSCProtoV2::taskTick1s();
    // Send heartbeat to all active slaves
    for (KiSCPeer slave : slaves) {
        if (millis() - slave.lastMsg > 5000) {
            slave.active = false;
            DBGLOG(Warning, "Slave %02X %02X %02X %02X %02X %02X not responding", slave.address.getAddress()[0], slave.address.getAddress()[1], slave.address.getAddress()[2], slave.address.getAddress()[3], slave.address.getAddress()[4], slave.address.getAddress()[5]);
        }
        if (slave.active) {
            DBGLOG(Verbose, "Pinging %02X %02X %02X %02X %02X %02X", slave.address.getAddress()[0], slave.address.getAddress()[1], slave.address.getAddress()[2], slave.address.getAddress()[3], slave.address.getAddress()[4], slave.address.getAddress()[5]);
            KiSCProtoV2Message_Info msg(slave.address.getAddress());
            msg.setName(name);
            msg.setRole(role);
            msg.setState(state);
            msg.setType(KiSCPeer::SlaveType::Unidentified);
            send(&msg);
        }
    }
    // Check for offline peers and remove them
    slaves.erase(std::remove_if(slaves.begin(), slaves.end(), [](KiSCPeer slave) { return !slave.active; }), slaves.end());

    // Turn off broadcast after 3 sek
    if (millis() - broadcastStart > 3000) {
        if (broadcastActive) {
            DBGLOG(Info, "Broadcast timeout, disabling broadcast announcements");
        }
        broadcastActive = false;
    }
}

void
KiSCProtoV2Master::taskTick500ms() {
    DBGLOG(Debug, "KiSCProtoV2Master.taskTick500ms()");
    if (broadcastActive) {
        sendBroadcastOffer();
    }
}

bool
KiSCProtoV2Master::canAdd(KiSCPeer slave) {
    // Check existing slaves, if a slave of the given type is already there. If so, reject the new slave
    for (KiSCPeer _slave : slaves) {
        if (_slave.type == slave.type) {
            return false;
        }
    }
    return true;
}

void
KiSCProtoV2Master::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast, bool delBuffer) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
        DBGLOG(Error, "Failed to take mutex");
        return;
    }
    DBGLOG(Debug, "KiSCProtoV2Master.messageReceived()");
    KiSCProtoV2::messageReceived(msg, rssi, broadcast, false);
    if (broadcast) {
        DBGLOG(Info, "Broadcast message received, enabling broadcast announcements");
        broadcastActive = true;
        broadcastStart = millis();
    } else {
        // Check, if peer exists, than update the last Msg time
        for (KiSCPeer slave : slaves) {
            if (slave.address == msg->getSource()) {
                slave.lastMsg = millis();
                slave.active = true;
            }
        }
    }
    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = dynamic_cast<KiSCProtoV2Message_Info*>(msg);
//        KiSCPeer slave(i nfoMsg->getSource(), infoMsg->getName(), infoMsg->getRole(), infoMsg->getState(), infoMsg->getType());
//        slaves.push_back(slave);
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
        KiSCProtoV2Message_network* networkMsg = dynamic_cast<KiSCProtoV2Message_network*>(msg);
        if (networkMsg->getSubCommand() == MSGTYPE_NETWORK_JOIN) {
            DBGLOG(Info, "Join Request from %02X %02X %02X %02X %02X %02X", msg->getSource().getAddress()[0], msg->getSource().getAddress()[1], msg->getSource().getAddress()[2], msg->getSource().getAddress()[3], msg->getSource().getAddress()[4], msg->getSource().getAddress()[5]);
            KiSCProtoV2Message_network acceptMsg(msg->getSource().getAddress());
            KiSCPeer _slave;
            _slave.name = networkMsg->getName();
            _slave.type = networkMsg->getType();
            _slave.address = networkMsg->getSource();
            _slave.lastMsg = millis();
            if (canAdd(_slave)) {
                DBGLOG(Info, "Slave added %02X %02X %02X %02X %02X %02X", _slave.address.getAddress()[0], _slave.address.getAddress()[1], _slave.address.getAddress()[2], _slave.address.getAddress()[3], _slave.address.getAddress()[4], _slave.address.getAddress()[5]);
                dynamic_cast<KiSCProtoV2Master*>(kiscprotoV2)->addSlave(KiSCPeer(_slave));
                acceptMsg.setAcceptResponse();
            } else {
                DBGLOG(Warning, "Slave not added %02X %02X %02X %02X %02X %02X", _slave.address.getAddress()[0], _slave.address.getAddress()[1], _slave.address.getAddress()[2], _slave.address.getAddress()[3], _slave.address.getAddress()[4], _slave.address.getAddress()[5]);
                acceptMsg.setRejectResponse();
            }
            acceptMsg.setName(name);
            
            kiscprotoV2->send(&acceptMsg);
        }
    } else {
        DBGLOG(Warning, "Unknown message type %02X", msg->getCommand());
    }
    if (delBuffer) {
        delete msg;
    }
    xSemaphoreGive(mutex);
}

void                    
KiSCProtoV2Master::addSlave(KiSCPeer slave) { 
    DBGLOG(Info, "adding slave %02X %02X %02X %02X %02X %02X Type %d Name %s", slave.address.getAddress()[0], slave.address.getAddress()[1], slave.address.getAddress()[2], slave.address.getAddress()[3], slave.address.getAddress()[4], slave.address.getAddress()[5], slave.type, slave.name.c_str());
    slaves.push_back(slave); 
}

void
KiSCProtoV2Master::sendBroadcastOffer() {
    KiSCProtoV2Message_Info msg(BroadcastAddress.getAddress());
    msg.setName(name);
    msg.setRole(role);
    msg.setState(state);
    msg.setType(KiSCPeer::SlaveType::Unidentified);
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
    // 4: Type
    // 5-13: Name
    
    msg.payload[2] = role;
    msg.payload[3] = state;
    msg.payload[4] = type;
    for (int i = 0; i < name.length(); i++) {
        msg.payload[5+i] = name[i];
    }
    msg.payload_len = 5 + name.length();
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
    type = (KiSCPeer::SlaveType)msg.payload[4];	
    name = "";
    for (int i = 5; i < msg.payload_len; i++) {
        name += (char)msg.payload[i];
    }
    return true;
}

void
KiSCProtoV2Message_Info::dump() {
    DBGLOG(Verbose, "Info from %s (%02X:%02X:%02X:%02X:%02X:%02X) to %02X:%02X:%02X:%02X:%02X:%02X", name.c_str(), source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5], target.getAddress()[0], target.getAddress()[1], target.getAddress()[2], target.getAddress()[3], target.getAddress()[4], target.getAddress()[5]);
/*    DBGLOG(Verbose, "From %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
    DBGLOG(Verbose, "To %02X %02X %02X %02X %02X %02X", target.getAddress()[0], target.getAddress()[1], target.getAddress()[2], target.getAddress()[3], target.getAddress()[4], target.getAddress()[5]);
    DBGLOG(Verbose, "  Name: %s", name.c_str());
    DBGLOG(Verbose, "  Role: %d", role);
    DBGLOG(Verbose, "  State: %d", state);
*/    
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message_network
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message_network::KiSCProtoV2Message_network(uint8_t address[]) : KiSCProtoV2Message(address) {
    DBGLOG(Debug, "KiSCProtoV2Message_network()");
}

bool
KiSCProtoV2Message_network::buildFromBuffer() {
    DBGLOG(Debug, "KiSCProtoV2Message_network.buildFromBuffer()");
    KiSCProtoV2Message::buildFromBuffer();
    if (msg.payload_len < 2) {
        return false;
    }
    if ((msg.payload[1] != MSGTYPE_NETWORK) && (msg.payload[1] != MSGTYPE_NETWORK_JOIN) && (msg.payload[1] != MSGTYPE_NETWORK_LEAVE) && (msg.payload[1] != MSGTYPE_NETWORK_ACCEPT) && (msg.payload[1] != MSGTYPE_NETWORK_REJECT)) {
        return false;
    }
    subCommand = msg.payload[2];
    type = KiSCPeer::SlaveType(msg.payload[3]);
    name = "";
    for (int i = 4; i < msg.payload_len; i++) {
        name += (char)msg.payload[i];
    }

    return true;
}
void
KiSCProtoV2Message_network::buildBufferedMessage() {
    DBGLOG(Debug, "KiSCProtoV2Message_network.buildBufferedMessage()");
    KiSCProtoV2Message::buildBufferedMessage();
    memcpy(msg.dstAddress, target.getAddress(), sizeof(msg.dstAddress));
    msg.payload[0] = PROTO_VERSION;
    msg.payload[1] = subCommand == 0 ? MSGTYPE_NETWORK : subCommand;
    msg.payload[2] = subCommand;
    msg.payload[3] = getType();
    // add name
    for (int i = 0; i < name.length(); i++) {
        msg.payload[4+i] = name[i];
    }
    msg.payload_len = 4+name.length();
}

void
KiSCProtoV2Message_network::dump() {
    switch (subCommand) {
        case MSGTYPE_NETWORK_JOIN:
            DBGLOG(Verbose, "Join Request from %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
            break;
        case MSGTYPE_NETWORK_LEAVE:
            DBGLOG(Verbose, "Leave Request from %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
            break;
        case MSGTYPE_NETWORK_ACCEPT:
            DBGLOG(Verbose, "Accept Response from %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
            break;
        case MSGTYPE_NETWORK_REJECT:
            DBGLOG(Verbose, "Reject Response from %02X %02X %02X %02X %02X %02X", source.getAddress()[0], source.getAddress()[1], source.getAddress()[2], source.getAddress()[3], source.getAddress()[4], source.getAddress()[5]);
            break;
        default:
            DBGLOG(Verbose, "Unknown network message");
            break;
    }

//    DBGLOG(Debug, "KiSCProtoV2Message_network.dump()");
}
