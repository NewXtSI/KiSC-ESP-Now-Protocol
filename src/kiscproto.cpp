#include "../include/kiscproto.h"

#if USE_LOGGER
#define ESP32DEBUGGING
#include <ESP32Logger.h>
#endif

BluetoothAudioMessage _bam = BluetoothAudioMessage_init_zero;
BluetoothAudioControlMessage _bacm = BluetoothAudioControlMessage_init_zero;

/// general buffer for msg sender
uint8_t send_buffer[256];

/// general buffer for receive msgs
uint8_t recv_buffer[256];

/// receivers map (id,macaddr)
std::map<uint32_t, std::string> amp;

void saveReceiver(const uint8_t *macAddr);

KiSCProto::KiSCProto() {
    _pBluetoothAudioMessageCallbacks = nullptr;
    _pBluetoothAudioControlMessageCallbacks = nullptr;

    uint32_t chipId = 0;
    #ifdef ARDUINO_ARCH_ESP32
    for (int i = 0; i < 17; i = i + 8) chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    #else
    for (int i = 0; i < 17; i = i + 8) chipId |= ((ESP.getChipId() >> (40 - i)) & 0xff) << i;
    #endif
    _ESP_ID = String(chipId, HEX);    
}

void 
KiSCProto::setBluetoothAudioMessageCallbacks(BluetoothAudioMessageCallbacks* pCallbacks) {
    _pBluetoothAudioMessageCallbacks = pCallbacks;
}

void
KiSCProto::setBluetoothAudioControlMessageCallbacks(BluetoothAudioControlMessageCallbacks* pCallbacks) {
    _pBluetoothAudioControlMessageCallbacks = pCallbacks;
}

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

bool KiSCProto::sendBluetoothAudioMessage(BluetoothAudioMessage bam) {
    return sendMessage(encodeBluetoothAudioMessage(bam));
}

bool KiSCProto::sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm) {
    return sendMessage(encodeBluetoothAudioControlMessage(bacm));
}

size_t KiSCProto::encodeBluetoothAudioMessage(BluetoothAudioMessage bam) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));
    bool status = pb_encode(&stream, BluetoothAudioMessage_fields, &bam);
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length;
}

size_t KiSCProto::encodeBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));
    bool status = pb_encode(&stream, BluetoothAudioControlMessage_fields, &bacm);
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length;
}

bool BluetoothAudioMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, BluetoothAudioMessage_fields, &_bam);
    if (!status) {
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

void
KiSCProto::reportError(const char *msg) {
//    if (devmode) Serial.println(msg);
    if (_pBluetoothAudioMessageCallbacks != nullptr) {
        _pBluetoothAudioMessageCallbacks->onError(msg);
    }
    if (_pBluetoothAudioControlMessageCallbacks != nullptr) {
        _pBluetoothAudioControlMessageCallbacks->onError(msg);
    }
}

BluetoothAudioMessage KiSCProto::newBluetoothAudioMessage() {
    BluetoothAudioMessage bam = BluetoothAudioMessage_init_zero;
    return bam;
}

BluetoothAudioControlMessage KiSCProto::newBluetoothAudioControlMessage() {
    BluetoothAudioControlMessage bacm = BluetoothAudioControlMessage_init_zero;
    return bacm;
}

#ifdef ARDUINO_ARCH_ESP32
void BluetoothAudioControlMessageRecvCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
#else
void BluetoothAudioControlMessageRecvCallback(uint8_t *macAddr, uint8_t *data, uint8_t dataLen) {
#endif
    saveReceiver(macAddr);
    #ifdef ARDUINO_ARCH_ESP32
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
    #else
    int msgLen = dataLen;
    #endif
    memcpy(recv_buffer, data, msgLen); 
    BluetoothAudioControlMessageDecodeMessage(msgLen);
//    if (joystick.devmode) printMacAddress(macAddr);
}

#ifdef ARDUINO_ARCH_ESP32
void BluetoothAudioMessageRecvCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
#else
void BluetoothAudioMessageRecvCallback(uint8_t *macAddr, uint8_t *data, uint8_t dataLen) {
#endif
    saveReceiver(macAddr);
    #ifdef ARDUINO_ARCH_ESP32
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
    #else
    int msgLen = dataLen;
    #endif
    memcpy(recv_buffer, data, msgLen); 
    BluetoothAudioMessageDecodeMessage(msgLen);
//    if (joystick.devmode) printMacAddress(macAddr);
}

// callback when data is sent. Not necessary for now. 
void BluetoothAudioMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
    // if (!joystick.devmode) return;
    // printMacAddress(macAddr); 
    // Serial.print("Last Packet Send Status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void BluetoothAudioControlMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}

bool KiSCProto::sendMessage(uint32_t msglen, const uint8_t *mac) {
    #ifdef ARDUINO_ARCH_ESP32
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(&peerInfo);
    }
    esp_err_t result = esp_now_send(mac, send_buffer, msglen);
    #else // ESP8266
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer((uint8 *)mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    int result = esp_now_send((uint8 *) mac, (uint8_t *) send_buffer, msglen);
    #endif
    if (result == ESP_OK) {
#if USE_LOGGER
        DBGLOG(Debug, "Broadcast message success");
        DBGLOG(Debug, "Send message size: %i", msglen);
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
        if (_pBluetoothAudioMessageCallbacks != nullptr) {
            esp_now_register_recv_cb(BluetoothAudioMessageRecvCallback);
#if USE_LOGGER
            DBGLOG(Info, "Registered Bluetooth Audio Message callback");
#else
            ESP_LOGI("ESPNow", "Registered Bluetooth Audio Message callback");
#endif            
            esp_now_register_send_cb(BluetoothAudioMessageSendCallback);
            return true;
        }
        else if(_pBluetoothAudioControlMessageCallbacks != nullptr) {
            esp_now_register_recv_cb(BluetoothAudioControlMessageRecvCallback);
#if USE_LOGGER
            DBGLOG(Info, "Registered Bluetooth Audio Control Message callback");
#else
            ESP_LOGI("ESPNow", "Registered Bluetooth Audio Control Message callback");
#endif            
            esp_now_register_send_cb(BluetoothAudioControlMessageSendCallback);
            return true;
        }
        else {
#if USE_LOGGER
            DBGLOG(Error, "No callbacks registered");
#else
            ESP_LOGE("ESPNow", "No callbacks registered");
#endif
            return false;
        }

    return true;
}
