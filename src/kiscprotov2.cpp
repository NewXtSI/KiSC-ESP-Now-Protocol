#include <Arduino.h>
#include "../include/kiscprotov2.h"

#if USE_LOGGER
#define ESP32DEBUGGING
#include <ESP32Logger.h>
#else
enum class ESP32LogLevel {
	Error,
	Warning,
	Info,
	Verbose,
	Debug
};
#define DBGINI(output, ...) ;
#define DBGSTA ;
#define DBGSTP ;
#define DBGLEV(loglevel) ;
#define DBGCOD(debugexpression) ;
#define DBGLOG(loglevel, logmsg, ...) ;
#define DBGCHK(loglevel, cond, logmsg, ...) ;

#endif

#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#else
#error "Unsupported platform"
#endif  // ESP32

#include <QuickEspNow.h>

#define     DEBUG_OUTPUTENABLED      0
#define     DEBUG_OUTPUTDISABLED     1

#define     DEBUG_OUTPUT_RAW_PACKAGES   DEBUG_OUTPUTENABLED

KiSCAddress BroadcastAddress(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

bool KiSCProtoV2::ESPNowSent = true;
QueueHandle_t KiSCProtoV2::sendQueue = nullptr	;

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
    for (j=0; j<16; j++) {
      if (i+j < buflen) {
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02x ", buf[i+j]);
      } else {
        char temp[4] = "   ";
        strncat(line, temp, sizeof(line) - strlen(line) - 1);
      }
    }
    snprintf(line + strlen(line), sizeof(line) - strlen(line), " ");
    for (j=0; j<16; j++) 
      if (i+j < buflen)
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
     
     ESP_LOGI("KiSCProtoV2", "%s", line);
     Serial.printf("%s\n", line);
     DBGCHK(Info, DEBUG_OUTPUT_RAW_PACKAGES, line);
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
            snprintf(line + strlen(line), sizeof(line) - strlen(line), " %02X", msg.payload[i]);
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

#undef DUMP_MESSAGES

void         
KiSCProtoV2::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast, bool delBuffer) {
    DBGLOG(Debug, "KiSCProtoV2.messageReceived");
    msg->buildFromBuffer();
#if DUMP_MESSAGES
    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = static_cast<KiSCProtoV2Message_Info*>(msg);
        infoMsg->dump();
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
        KiSCProtoV2Message_network* networkMsg = static_cast<KiSCProtoV2Message_network*>(msg);
        networkMsg->dump();
    } else if (msg->getCommand() == MSGTYPE_BT_AUDIO) {
//    } else if (msg->isA<KiSCProtoV2Message_BTAudio>()) {
        KiSCProtoV2Message_BTAudio* btAudioMsg = dynamic_cast<KiSCProtoV2Message_BTAudio*>(msg);
        btAudioMsg->dump();
    } else {
        DBGLOG(Error, "Unknown message type");
    }
#endif    
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
    uint32_t last10ms = millis();
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
        // run 10ms tick every 10ms
        if (millis() - last10ms > 10) {
            proto->taskTick10ms();
            last10ms = millis();
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

        vTaskDelay(10);
    }
    vTaskDelete(NULL);
}

void
KiSCProtoV2::start() {
    DBGLOG(Info, "Starting KiSCProtoV2");

    if (init()) {
        state = KiSCPeer::Idle;
        ESPNowSent = true;
        if (xTaskCreatePinnedToCore(task, "KiSCCommTast", 512 * 12, this, 1, NULL, 1) != pdPASS) {
            Serial.println("Failed to start KiSCCommTask");
            DBGLOG(Error, "Failed to start KiSCCommTask");
        }
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
KiSCProtoV2::checkDirtyData() {

}

void
KiSCProtoV2::taskTick10ms() {
    checkDirtyData();
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
KiSCProtoV2Slave::dumpNetwork() {
/*
    DBGLOG(Info, "This is master: %s (%02X:%02X:%02X:%02X:%02X:%02X)", name.c_str(), getAddress().getAddress()[0], getAddress().getAddress()[1], getAddress().getAddress()[2], getAddress().getAddress()[3], getAddress().getAddress()[4], getAddress().getAddress()[5]);
    for (KiSCPeer *slave : slaves) {
        DBGLOG(Info, "  Slave %s (%02X:%02X:%02X:%02X:%02X:%02X)", slave->name.c_str(), slave->address.getAddress()[0], slave->address.getAddress()[1], slave->address.getAddress()[2], slave->address.getAddress()[3], slave->address.getAddress()[4], slave->address.getAddress()[5]);
    }
*/
    DBGLOG(Info, "This is slave: %s (%02X:%02X:%02X:%02X:%02X:%02X) Typ: %s", name.c_str(), getAddress().getAddress()[0], getAddress().getAddress()[1], getAddress().getAddress()[2], getAddress().getAddress()[3], getAddress().getAddress()[4], getAddress().getAddress()[5], KiSCPeer::getTypeDesc(type).c_str());
    if (masterFound) {
        DBGLOG(Info, "  Master %s (%02X:%02X:%02X:%02X:%02X:%02X)", master.name.c_str(), master.address.getAddress()[0], master.address.getAddress()[1], master.address.getAddress()[2], master.address.getAddress()[3], master.address.getAddress()[4], master.address.getAddress()[5]);
    } else {
        DBGLOG(Info, "  No master found");
    }
}

void
KiSCProtoV2Slave::messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast, bool delBuffer) {
    KiSCProtoV2::messageReceived(msg, rssi, broadcast, false);
 //   if ((!broadcast) && (msg->getSource() == master.address)) {
    if ((msg->getSource() == master.address)) {
        master.lastMsg = millis();
    }
    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = static_cast<KiSCProtoV2Message_Info*>(msg);
        if (!(static_cast<KiSCProtoV2Slave*>(kiscprotoV2))->isMasterFound()) {
            if (infoMsg->getRole() == KiSCPeer::Role::Master) {
                // Send join Request
                KiSCProtoV2Message_network joinMsg(infoMsg->getSource().getAddress());
                joinMsg.setJoinRequest();
                joinMsg.setName(name);
                joinMsg.setType(type);
                kiscprotoV2->send(&joinMsg);
                DBGLOG(Info, "Join request sent to %02X %02X %02X %02X %02X %02X", infoMsg->getSource().getAddress()[0], infoMsg->getSource().getAddress()[1], infoMsg->getSource().getAddress()[2], infoMsg->getSource().getAddress()[3], infoMsg->getSource().getAddress()[4], infoMsg->getSource().getAddress()[5]);
            } else {
                DBGLOG(Warning, "No master, but no master in message");
            }
//            masterFound = true;
//            master = KiSCPeer(infoMsg->source, infoMsg->name, infoMsg->role, infoMsg->state, infoMsg->type);
        } else {
            if (infoMsg->getRole() == KiSCPeer::Role::Master) {
                if (infoMsg->getName() != master.name) {
                    DBGLOG(Info, "Master name changed to: %s", infoMsg->getName().c_str());
                    master.name = infoMsg->getName();
                    if (rcvdNetworkMsg != nullptr) {
                        rcvdNetworkMsg();
                    }
                }
//                (dynamic_cast<KiSCProtoV2Slave*>(kiscprotoV2))->getMaster()->name = infoMsg->getName();
            }
        }
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
//        DBGLOG(Verbose, "Network message");
        KiSCProtoV2Message_network* networkMsg = static_cast<KiSCProtoV2Message_network*>(msg);
        if (networkMsg->getSubCommand() == MSGTYPE_NETWORK_ACCEPT) {
            if (!(static_cast<KiSCProtoV2Slave*>(kiscprotoV2))->isMasterFound()) {
                DBGLOG(Info, "Accept Response from %02X %02X %02X %02X %02X %02X", msg->getSource().getAddress()[0], msg->getSource().getAddress()[1], msg->getSource().getAddress()[2], msg->getSource().getAddress()[3], msg->getSource().getAddress()[4], msg->getSource().getAddress()[5]);
                KiSCPeer _master;
                _master.address = networkMsg->getSource();
                _master.lastMsg = millis();
                _master.active = true;
                _master.name = networkMsg->getName();
                static_cast<KiSCProtoV2Slave*>(kiscprotoV2)->setMaster(_master);
                if (rcvdNetworkMsg != nullptr) {
                    rcvdNetworkMsg();
                }
            }
        }
    } else {
        DBGLOG(Warning, "Unknown message type %02X", msg->getCommand());
    }
    if (delBuffer)
        delete msg;
}

void
KiSCProtoV2Slave::checkDirtyData() {
    KiSCProtoV2::checkDirtyData();
    if (!masterFound) {
        return;
    }
    static int lastSentMsg = millis();
    KiSCProtoV2Message *msg = nullptr;
    switch (type) {
        case KiSCPeer::SlaveType::Controller:
            break;
        case KiSCPeer::SlaveType::Motor:
            break;
        case KiSCPeer::SlaveType::Light:
            break;
        case KiSCPeer::SlaveType::Sound:
            break;
        case KiSCPeer::SlaveType::BTAudio:
            if (data.btAudioData.isDirtyToMaster() || (millis() - lastSentMsg > 5000)) {    // Bei Änderung oder alle 5 Sekunden
                KiSCProtoV2Message_BTAudio *_msg = new KiSCProtoV2Message_BTAudio(master.address.getAddress());
                _msg->subCommand = MSGTYPE_BT_AUDIO_INFO;
                _msg->setConnected(data.btAudioData.isConnected());
//                _msg->setPlaying(data.btAudioData.isPlaying());
                _msg->setAlbum(data.btAudioData.getAlbum());
                _msg->setArtist(data.btAudioData.getArtist());
                /*
                _msg->setTrack(data.btAudioData.getTrack());
                _msg->setVolume(data.btAudioData.getVolume());
                _msg->setDuration(data.btAudioData.getDuration());
                _msg->setPosition(data.btAudioData.getPosition());
                _msg->setPlaying(data.btAudioData.isPlaying());*/
                _msg->setTitle(data.btAudioData.getTitle());
                data.btAudioData.setDirtyToMaster(false);
                lastSentMsg = millis();
                msg = _msg;
            }

            break;
        case KiSCPeer::SlaveType::Display:
            break;
        case KiSCPeer::SlaveType::Peripheral:
            break;
        case KiSCPeer::SlaveType::Unidentified:
            break;
    }
    if (msg != nullptr) {
        send(msg);
        delete msg;
        lastSentMsg = millis();
    }

}

void
KiSCProtoV2Slave::taskTick1s() {
    KiSCProtoV2::taskTick1s();
    DBGLOG(Debug, "KiSCProtoV2Slave.taskTick1s()");
    if (masterFound) {
        if (millis() - master.lastMsg > 10000) {
            DBGLOG(Warning, "Master not responding");
            masterFound = false;
            if (rcvdNetworkMsg != nullptr) {
                rcvdNetworkMsg();
            }
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
        Serial.printf("Master not found, sending broadcast\n");
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
KiSCProtoV2Master::dumpNetwork() {
    DBGLOG(Info, "This is master: %s (%02X:%02X:%02X:%02X:%02X:%02X)", name.c_str(), getAddress().getAddress()[0], getAddress().getAddress()[1], getAddress().getAddress()[2], getAddress().getAddress()[3], getAddress().getAddress()[4], getAddress().getAddress()[5]);
    for (KiSCPeer *slave : slaves) {
        DBGLOG(Info, "  Slave %s (%02X:%02X:%02X:%02X:%02X:%02X) Type: %s", slave->name.c_str(), slave->address.getAddress()[0], slave->address.getAddress()[1], slave->address.getAddress()[2], slave->address.getAddress()[3], slave->address.getAddress()[4], slave->address.getAddress()[5], KiSCPeer::getTypeDesc(slave->type).c_str());
    }
}

void
KiSCProtoV2Master::taskTick1s() {
    KiSCProtoV2::taskTick1s();
    // Send heartbeat to all active slaves
    DBGLOG(Debug , "checking %d slaves", slaves.size());
    for (KiSCPeer *slave : slaves) {
        DBGLOG(Debug, "  Checking slave %s", slave->name.c_str());
        if (millis() - slave->lastMsg > 5000) {
            slave->active = false;
            DBGLOG(Warning, "Slave %s not responding", slave->name.c_str());
        }
        if (slave->active) {
            DBGLOG(Debug, "Pinging %s", slave->name.c_str());
            KiSCProtoV2Message_Info msg(slave->address.getAddress());
            msg.setName(name);
            msg.setRole(role);
            msg.setState(state);
            msg.setType(KiSCPeer::SlaveType::Unidentified);
            send(&msg);
        }
    }
    int numSlaves = slaves.size();

    // Check for offline peers and remove them and delete added pointer
    slaves.erase(std::remove_if(slaves.begin(), slaves.end(), [](KiSCPeer *slave) { return !slave->active; }), slaves.end());

    if (numSlaves != slaves.size()) {
        if (rcvdNetworkMsg != nullptr) {
            rcvdNetworkMsg();
        }
    }


//    slaves.erase(std::remove_if(slaves.begin(), slaves.end(), [](KiSCPeer *slave) { return !slave->active; }), slaves.end());

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
    // check if slave is already in list, if so, update lastMsg time
    for (KiSCPeer *_slave : slaves) {
        if (_slave->address == slave.address) {
            _slave->lastMsg = millis();
            return true;
        }
    }
    // Check existing slaves, if a slave of the given type is already there. If so, reject the new slave
    for (KiSCPeer* _slave : slaves) {
        if (_slave->type == slave.type) {
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
        DBGLOG(Debug, "Broadcast message received, enabling broadcast announcements");
        broadcastActive = true;
        broadcastStart = millis();
    } else {
        // Check, if peer exists, than update the last Msg time
        for (KiSCPeer *slave : slaves) {
            if (slave->address == msg->getSource()) {
                DBGLOG(Debug, "Slave %s found", slave->name.c_str());
                slave->lastMsg = millis();
                slave->active = true;
            }
        }
    }
    if (msg->getCommand() == MSGTYPE_INFO) {
//    if (msg->isA<KiSCProtoV2Message_Info>()) {
        KiSCProtoV2Message_Info* infoMsg = static_cast<KiSCProtoV2Message_Info*>(msg);
//        KiSCPeer slave(i nfoMsg->getSource(), infoMsg->getName(), infoMsg->getRole(), infoMsg->getState(), infoMsg->getType());
//        slaves.push_back(slave);
    } else if ((msg->getCommand() == MSGTYPE_NETWORK) || (msg->getCommand() == MSGTYPE_NETWORK_JOIN) || (msg->getCommand() == MSGTYPE_NETWORK_LEAVE) || (msg->getCommand() == MSGTYPE_NETWORK_ACCEPT) || (msg->getCommand() == MSGTYPE_NETWORK_REJECT)) {
//    } else if (msg->isA<KiSCProtoV2Message_network>()) {
        KiSCProtoV2Message_network* networkMsg = static_cast<KiSCProtoV2Message_network*>(msg);
        if (networkMsg->getSubCommand() == MSGTYPE_NETWORK_JOIN) {
            DBGLOG(Info, "Join Request from %02X %02X %02X %02X %02X %02X", msg->getSource().getAddress()[0], msg->getSource().getAddress()[1], msg->getSource().getAddress()[2], msg->getSource().getAddress()[3], msg->getSource().getAddress()[4], msg->getSource().getAddress()[5]);
            KiSCProtoV2Message_network acceptMsg(msg->getSource().getAddress());
            KiSCPeer _slave;
            _slave.active = true;
            _slave.name = networkMsg->getName();
            _slave.type = networkMsg->getType();
            _slave.address = networkMsg->getSource();
            _slave.lastMsg = millis();
            if (canAdd(_slave)) {
                DBGLOG(Debug, "Slave added %02X %02X %02X %02X %02X %02X", _slave.address.getAddress()[0], _slave.address.getAddress()[1], _slave.address.getAddress()[2], _slave.address.getAddress()[3], _slave.address.getAddress()[4], _slave.address.getAddress()[5]);
                static_cast<KiSCProtoV2Master*>(kiscprotoV2)->addSlave(new KiSCPeer(_slave));
                acceptMsg.setAcceptResponse();
                if (rcvdNetworkMsg != nullptr) {
                    rcvdNetworkMsg();
                }
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
KiSCProtoV2Master::addSlave(KiSCPeer* slave) { 
    // only add, if not already in list
    for (KiSCPeer *_slave : slaves) {
        if (_slave->address == slave->address) {
            return;
        }
    }
    DBGLOG(Debug, "adding slave Type %d Name %s", slave->type, slave->name.c_str());
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
