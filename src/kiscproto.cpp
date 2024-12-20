#include "../include/kiscproto.h"

#if USE_LOGGER
#define ESP32DEBUGGING
#include <ESP32Logger.h>
#endif

#if PROTOBUF_USE_BT_AUDIO
BluetoothAudioMessage _bam = BluetoothAudioMessage_init_zero;
BluetoothAudioControlMessage _bacm = BluetoothAudioControlMessage_init_zero;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
RemotecontrolMessage _rcm = RemotecontrolMessage_init_zero;
#endif

/// general buffer for msg sender
uint8_t send_buffer[256];

/// general buffer for receive msgs
uint8_t recv_buffer[256];

/// receivers map (id,macaddr)
std::map<uint32_t, std::string> amp;

void saveReceiver(const uint8_t *macAddr);

KiSCProto::KiSCProto() {
#if PROTOBUF_USE_BT_AUDIO    
    _pBluetoothAudioMessageCallbacks = nullptr;
    _pBluetoothAudioControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
    _pRemotecontrolMessageCallbacks = nullptr;
#endif

    uint32_t chipId = 0;
    #ifdef ARDUINO_ARCH_ESP32
    for (int i = 0; i < 17; i = i + 8) chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    #else
    for (int i = 0; i < 17; i = i + 8) chipId |= ((ESP.getChipId() >> (40 - i)) & 0xff) << i;
    #endif
    _ESP_ID = String(chipId, HEX);    
}
#if PROTOBUF_USE_BT_AUDIO
void 
KiSCProto::setBluetoothAudioMessageCallbacks(BluetoothAudioMessageCallbacks* pCallbacks) {
    _pBluetoothAudioMessageCallbacks = pCallbacks;
}

void
KiSCProto::setBluetoothAudioControlMessageCallbacks(BluetoothAudioControlMessageCallbacks* pCallbacks) {
    _pBluetoothAudioControlMessageCallbacks = pCallbacks;
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
void
KiSCProto::setRemotecontrolMessageCallbacks(RemotecontrolMessageCallbacks* pCallbacks) {
    _pRemotecontrolMessageCallbacks = pCallbacks;
}
#endif
uint32_t getReceiverId(const uint8_t *macAddr){
    return macAddr[0]+macAddr[1]+macAddr[2]+macAddr[3]+macAddr[4]+macAddr[5];
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
    snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

std::string KiSCProto::getFormattedMacAddr(const uint8_t *macAddress){
    char macStr[18];
    formatMacAddress(macAddress, macStr, 18);
    return std::string(macStr);
}

void printMacAddress(const uint8_t * macAddress){
    char macStr[18];
    formatMacAddress(macAddress, macStr, 18);
#if USE_LOGGER
    DBGLOG(Debug, "MAC: %s", macStr);
#else
    ESP_LOGD("ESPNow", "MAC: %s", macStr);
#endif    
//    Serial.println(macStr);
}

void printBuffer(uint8_t *buffer, uint32_t length) {
    char outstr[255];
    memset(outstr, 0, 255);    
    for (uint32_t i = 0; i < length; i++) {
        char sym[4];
        sprintf(sym, "%02X ", buffer[i]);
        strcat(outstr, sym);
//        Serial.printf("%02X", buffer[i]);
    }
#if USE_LOGGER    
    DBGLOG(Debug, "Buffer: %s", outstr);
#else
    ESP_LOGD("ESPNow", "Buffer: %s", outstr);
#endif
//    Serial.println();
}
#if PROTOBUF_USE_BT_AUDIO
bool KiSCProto::sendBluetoothAudioMessage(BluetoothAudioMessage bam) {
    DBGLOG(Verbose, "Sending Bluetooth Audio Message");
    return sendMessage(encodeBluetoothAudioMessage(bam));
}

bool KiSCProto::sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm) {
    return sendMessage(encodeBluetoothAudioControlMessage(bacm));
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
bool KiSCProto::sendRemotecontrolMessage(RemotecontrolMessage rcm) {
    return sendMessage(encodeRemotecontrolMessage(rcm));
}
#endif
#if PROTOBUF_USE_BT_AUDIO
typedef struct
{   char text[32]; } callback_context_t;

bool encode_string(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    // ...and you always cast to the same pointer type, reducing
    // the chance of mistakes
    callback_context_t * ctx = (callback_context_t *)(*arg);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)ctx->text, strlen(ctx->text));
}

bool 
KiSCProto::setBluetoothAudioMessageArtist(BluetoothAudioMessage bam, const char *artist) {
    callback_context_t ctx;    
    strncpy(ctx.text, artist, 32);
    bam.bta.arg = &ctx;
    bam.bta.funcs.encode = &encode_string;
    return true;
}

bool 
KiSCProto::setBluetoothAudioMessageTitle(BluetoothAudioMessage bam, const char *title) {
    callback_context_t ctx;    
    strncpy(ctx.text, title, 32);
    bam.bts.arg = &ctx;
    bam.bts.funcs.encode = &encode_string;
    return true;
}

size_t KiSCProto::encodeBluetoothAudioMessage(BluetoothAudioMessage bam) {
    DBGLOG(Verbose, "Encoding Bluetooth Audio Message");
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, BluetoothAudioMessage_fields, &bam);
    send_buffer[0] = MSG_TYPE_BLUETOOTH_AUDIO_MESSAGE;
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
        DBGLOG(Error, "Encoding failed: %s", PB_GET_ERROR(&stream));
//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length+1;
}

size_t KiSCProto::encodeBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, BluetoothAudioControlMessage_fields, &bacm);
    send_buffer[0] = MSG_TYPE_BLUETOOTH_AUDIO_CONTROL_MESSAGE;
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length+1;
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
size_t KiSCProto::encodeRemotecontrolMessage(RemotecontrolMessage rcm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, RemotecontrolMessage_fields, &rcm);
    send_buffer[0] = MSG_TYPE_REMOTECONTROL_MESSAGE;
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length+1;
}
#endif
#if PROTOBUF_USE_BT_AUDIO
bool BluetoothAudioMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, BluetoothAudioMessage_fields, &_bam);
    if (!status) {
        DBGLOG(Error, "Decoding bluetooth audio msg failed: %s", PB_GET_ERROR(&stream));
//        if(joystick.devmode) printf("Decoding bluetooth audio msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pBluetoothAudioMessageCallbacks != nullptr) {
        kiscproto._pBluetoothAudioMessageCallbacks->onBluetoothAudioMessage(_bam);
    }
    return true;
}

bool BluetoothAudioControlMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, BluetoothAudioControlMessage_fields, &_bacm);
    if (!status) {
//        if(joystick.devmode) printf("Decoding bluetooth audio control msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pBluetoothAudioControlMessageCallbacks != nullptr) {
        kiscproto._pBluetoothAudioControlMessageCallbacks->onBluetoothAudioControlMessage(_bacm);
    }
    return true;
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
bool RemotecontrolMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, RemotecontrolMessage_fields, &_rcm);
    if (!status) {
//        if(joystick.devmode) printf("Decoding remote control msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pRemotecontrolMessageCallbacks != nullptr) {
        kiscproto._pRemotecontrolMessageCallbacks->onRemotecontrolMessage(_rcm);
    }
    return true;
}
#endif

void
KiSCProto::reportError(const char *msg) {
//    if (devmode) Serial.println(msg);
#if PROTOBUF_USE_BT_AUDIO
    if (_pBluetoothAudioMessageCallbacks != nullptr) {
        _pBluetoothAudioMessageCallbacks->onError(msg);
    }
    if (_pBluetoothAudioControlMessageCallbacks != nullptr) {
        _pBluetoothAudioControlMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    if (_pRemotecontrolMessageCallbacks != nullptr) {
        _pRemotecontrolMessageCallbacks->onError(msg);
    }
#endif    
}

#if PROTOBUF_USE_BT_AUDIO
BluetoothAudioMessage KiSCProto::newBluetoothAudioMessage() {
    BluetoothAudioMessage bam = BluetoothAudioMessage_init_zero;
    return bam;
}

BluetoothAudioControlMessage KiSCProto::newBluetoothAudioControlMessage() {
    BluetoothAudioControlMessage bacm = BluetoothAudioControlMessage_init_zero;
    return bacm;
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
RemotecontrolMessage KiSCProto::newRemotecontrolMessage() {
    RemotecontrolMessage rcm = RemotecontrolMessage_init_zero;
    return rcm;
}
#endif
void UniversalMessageRecvCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
    saveReceiver(macAddr);
    #ifdef ARDUINO_ARCH_ESP32
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen-1);
    #else
    int msgLen = dataLen-1;
    #endif
    uint8_t msgType = data[0];
    memcpy(recv_buffer, data+1, msgLen); 
    switch (msgType) {
#if PROTOBUF_USE_BT_AUDIO        
        case MSG_TYPE_BLUETOOTH_AUDIO_MESSAGE:
            if (kiscproto._pBluetoothAudioMessageCallbacks != nullptr) {
                BluetoothAudioMessageDecodeMessage(msgLen);
            }
            break;
        case MSG_TYPE_BLUETOOTH_AUDIO_CONTROL_MESSAGE:
            if (kiscproto._pBluetoothAudioControlMessageCallbacks != nullptr) {
                BluetoothAudioControlMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL            
        case MSG_TYPE_REMOTECONTROL_MESSAGE:
            if (kiscproto._pRemotecontrolMessageCallbacks != nullptr) {
                RemotecontrolMessageDecodeMessage(msgLen);
            }
            break;
#endif
        default:
            break;            
    }
//    RemotecontrolMessageDecodeMessage(msgLen);
//    if (joystick.devmode) printMacAddress(macAddr);
}

#if PROTOBUF_USE_BT_AUDIO
// callback when data is sent. Not necessary for now. 
void BluetoothAudioMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
    // if (!joystick.devmode) return;
    // printMacAddress(macAddr); 
    // Serial.print("Last Packet Send Status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void BluetoothAudioControlMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
void RemotecontrolMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
bool KiSCProto::sendMessage(uint32_t msglen, const uint8_t *mac) {
    #ifdef ARDUINO_ARCH_ESP32
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(&peerInfo);
    }
    DBGLOG(Debug, "Sending message to: %s", getFormattedMacAddr(mac).c_str());
    esp_err_t result = esp_now_send(mac, send_buffer, msglen);
    #else // ESP8266
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer((uint8 *)mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    int result = esp_now_send((uint8 *) mac, (uint8_t *) send_buffer, msglen);
    #endif
    if (result == ESP_OK) {
#if USE_LOGGER
        DBGLOG(Verbose, "Broadcast message success");
        DBGLOG(Verbose, "Send message size: %i", msglen);
        printBuffer(send_buffer, msglen);
#endif

/*        
        if (joystick.devmode) {
            Serial.println("Broadcast message success");
            printMacAddress(mac);
            Serial.printf("Send message size: %i\r\n", msglen);
            printBuffer(send_buffer, msglen);
        }*/
        return true;
    
    #ifdef ARDUINO_ARCH_ESP32
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
        reportError("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
        reportError("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
        reportError("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
        reportError("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
        reportError("Peer not found.");
    #endif
    } else {
        reportError("Unknown error");
    }
    return false;
}

bool checkReceiver(const uint8_t *macAddr) {
  std::map<uint32_t, std::string>::const_iterator iter;
  iter = amp.find(getReceiverId(macAddr));
  if (iter != amp.end()) return true;
  return false;
}

std::vector<uint32_t> KiSCProto::getReceivers() {
  std::vector<uint32_t> vints;
  for (auto const &imap : amp)
    vints.push_back(imap.first);
  return vints;
}

/// returns the MAC address of receiver with this id
const uint8_t * KiSCProto::getReceiverMacAddr(uint32_t receiverId) {
  std::map<uint32_t, std::string>::const_iterator iter;
  iter = amp.find(receiverId);
  if (iter != amp.end()) return (const uint8_t *)(iter->second.c_str());
  return nullptr;
}

void KiSCProto::printReceivers() {
  for (const auto& ka: amp) {
    char macStr[18];
    formatMacAddress((const uint8_t *)ka.second.c_str(), macStr, 18);
#if USE_LOGGER
    DBGLOG(Info, "receiverId: %i [%s]", ka.first, macStr);
#else
    ESP_LOGI("ESPNow", "receiverId: %i [%s]", ka.first, macStr);
#endif    
//    Serial.printf("receiverId: %i [%s]\r\n",ka.first,macStr);
  }
}

void saveReceiver(const uint8_t *macAddr) {
  if (!checkReceiver(macAddr)) {
    uint32_t id = getReceiverId(macAddr);
#if USE_LOGGER
    DBGLOG(Info, "New receiverId: %i with MAC: ", id);
    printMacAddress(macAddr);
#else
    ESP_LOGI("ESPNow", "New receiverId: %i with MAC: ", id);
    printMacAddress(macAddr);
#endif

//    Serial.printf("[%02d] New receiverId: %i with MAC: ", amp.size()+1, id);
//    printMacAddress(macAddr);
    amp.insert(std::make_pair(id, std::string((const char *)macAddr)));
  }
}

bool KiSCProto::sendMessage(uint32_t msglen) {
    return sendMessage(msglen, targetAddress);    
}


bool 
KiSCProto::init() {
    WiFi.mode(WIFI_STA);
    // startup ESP Now
#if USE_LOGGER
    DBGLOG(Info, "ESPNow Init");
    DBGLOG(Info, "ESPNow MAC: %s", WiFi.macAddress().c_str());
#else
    ESP_LOGI("ESPNow", "ESPNow Init");
    ESP_LOGI("ESPNow", "ESPNow MAC: %s", WiFi.macAddress().c_str());    
#endif    
    // shutdown wifi
    WiFi.disconnect();
    delay(100);

    #ifdef ARDUINO_ARCH_ESP32
    if (esp_now_init() != ESP_OK) {
#if USE_LOGGER
        DBGLOG(Error, "Error initializing ESP-NOW");
#else
        ESP_LOGE("ESPNow", "Error initializing ESP-NOW");        
#endif        
        return false;
    }
    #else
    if (esp_now_init() != 0) {
#if USE_LOGGER
        DBGLOG(Error, "Error initializing ESP-NOW");
#else
        ESP_LOGE("ESPNow", "Error initializing ESP-NOW");        
#endif        
        return false;
    }
    #endif
#if USE_LOGGER
        DBGLOG(Info, "ESPNow Init Success");
#else
        ESP_LOGI("ESPNow", "ESPNow Init Success");
#endif                
        if (1) {
            esp_now_register_recv_cb(UniversalMessageRecvCallback);
        } else {
#if USE_LOGGER
            DBGLOG(Error, "No callbacks registered");
#else
            ESP_LOGE("ESPNow", "No callbacks registered");
#endif
            return false;
        }

    return true;
}


