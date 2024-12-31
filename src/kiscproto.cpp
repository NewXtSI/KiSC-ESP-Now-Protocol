#include "../include/kiscproto.h"

#if USE_LOGGER
#define ESP32DEBUGGING
#include <ESP32Logger.h>
#endif

#include "esp_wifi.h"


#if PROTOBUF_USE_BT_AUDIO
//BluetoothAudioMessage _bam = BluetoothAudioMessage_init_zero;
//BluetoothAudioControlMessage _bacm = BluetoothAudioControlMessage_init_zero;
BluetoothAudioMessage *_bam = nullptr;
BluetoothAudioControlMessage *_bacm = nullptr;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
RemotecontrolMessage _rcm = RemotecontrolMessage_init_zero;
#endif
#if PROTOBUF_USE_LIGHT
LightMessage *_lm = nullptr;
#endif
#if PROTOBUF_USE_MOTOR
MotorboardFeedback _mm = MotorboardFeedback_init_zero;
MotorboardControl _mcm = MotorboardControl_init_zero;
#endif
#if PROTOBUF_USE_SYSTEM
SysMessage *_sm = nullptr;
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
SoundGeneratorMessage *_sgm = nullptr;
SoundGeneratorControlMessage *_sgcm = nullptr;
#endif
#if PROTOBUF_USE_DISPLAY
DisplayMessage _dm = DisplayMessage_init_zero;
#endif
#if PROTOBUF_USE_PERIPHERALS
PeripheralsControlMessage _pm = PeripheralsControlMessage_init_zero;
PeripheralsFeedbackMessage _pfm = PeripheralsFeedbackMessage_init_zero;
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
#if PROTOBUF_USE_LIGHT
    _pLightMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_MOTOR
    _pMotorMessageCallbacks = nullptr;
    _pMotorControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_SYSTEM
    _pSystemMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_DISPLAY
    _pDisplayMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
    _pSoundGeneratorMessageCallbacks = nullptr;
    _pSoundGeneratorControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_PERIPHERALS
    _pPeripheralsMessageCallbacks = nullptr;
    _pPeripheralsFeedbackCallbacks = nullptr;
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

#if PROTOBUF_USE_LIGHT
void
KiSCProto::setLightMessageCallbacks(LightMessageCallbacks* pCallbacks) {
    _pLightMessageCallbacks = pCallbacks;
}
#endif

#if PROTOBUF_USE_MOTOR
void
KiSCProto::setMotorMessageCallbacks(MotorMessageCallbacks* pCallbacks) {
    _pMotorMessageCallbacks = pCallbacks;
}
void
KiSCProto::setMotorControlMessageCallbacks(MotorControlMessageCallbacks* pCallbacks) {
    _pMotorControlMessageCallbacks = pCallbacks;
}
#endif

#if PROTOBUF_USE_SYSTEM
void
KiSCProto::setSystemMessageCallbacks(SystemMessageCallbacks* pCallbacks) {
    _pSystemMessageCallbacks = pCallbacks;
}
#endif

#if PROTOBUF_USE_SOUND_GENERATOR
void
KiSCProto::setSoundGeneratorMessageCallbacks(SoundGeneratorMessageCallbacks* pCallbacks) {
    _pSoundGeneratorMessageCallbacks = pCallbacks;
}
void
KiSCProto::setSoundGeneratorControlMessageCallbacks(SoundGeneratorControlMessageCallbacks* pCallbacks) {
    _pSoundGeneratorControlMessageCallbacks = pCallbacks;
}
#endif

#if PROTOBUF_USE_DISPLAY
void
KiSCProto::setDisplayMessageCallbacks(DisplayMessageCallbacks* pCallbacks) {
    _pDisplayMessageCallbacks = pCallbacks;
}
#endif

#if PROTOBUF_USE_PERIPHERALS
void
KiSCProto::setPeripheralsMessageCallbacks(PeripheralsMessageCallbacks* pCallbacks) {
    _pPeripheralsMessageCallbacks = pCallbacks;
}
void
KiSCProto::setPeripheralsFeedbackCallbacks(PeripheralsFeedbackCallbacks* pCallbacks) {
    _pPeripheralsFeedbackCallbacks = pCallbacks;
}
#endif


KiSCProto* KiSCProto::getInstance() {
    return this;
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
#if 0    
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
    ESP_LOGI("ESPNow", "Buffer: %s", outstr);
#endif
#endif    
//    Serial.println();
}
#if PROTOBUF_USE_BT_AUDIO
bool KiSCProto::sendBluetoothAudioMessage(BluetoothAudioMessage bam, char *artist, char *title, char *album) {
    DBGLOG(Verbose, "Sending Bluetooth Audio Message");
    return sendMessage(encodeBluetoothAudioMessage(bam, artist, title, album));
}

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
bool KiSCProto::sendRemotecontrolMessage(RemotecontrolMessage rcm, const uint8_t *mac) {
    return sendMessage(encodeRemotecontrolMessage(rcm), mac);
}
#endif
#if PROTOBUF_USE_LIGHT
bool KiSCProto::sendLightMessage(LightMessage lm) {
    return sendMessage(encodeLightMessage(lm));
}
#endif
#if PROTOBUF_USE_MOTOR
bool KiSCProto::sendMotorMessage(MotorboardFeedback mm) {
    return sendMessage(encodeMotorMessage(mm));
}
bool KiSCProto::sendMotorControlMessage(MotorboardControl mcm) {
    return sendMessage(encodeMotorControlMessage(mcm));
}
#endif

#if PROTOBUF_USE_SYSTEM
bool KiSCProto::sendSystemMessage(SysMessage sm) {
    return sendMessage(encodeSystemMessage(sm));
}
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
bool KiSCProto::sendSoundGeneratorMessage(SoundGeneratorMessage sgm) {
    return sendMessage(encodeSoundGeneratorMessage(sgm));
}
bool KiSCProto::sendSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm) {
    return sendMessage(encodeSoundGeneratorControlMessage(sgcm));
}
#endif
#if PROTOBUF_USE_DISPLAY
bool KiSCProto::sendDisplayMessage(DisplayMessage dm) {
    return sendMessage(encodeDisplayMessage(dm));
}
#endif
#if PROTOBUF_USE_PERIPHERALS
bool KiSCProto::sendPeripheralsMessage(PeripheralsControlMessage pm) {
    return sendMessage(encodePeripheralsMessage(pm));
}
bool KiSCProto::sendPeripheralsFeedbackMessage(PeripheralsFeedbackMessage pfm) {
    return sendMessage(encodePeripheralsFeedbackMessage(pfm));
}
#endif

#if PROTOBUF_USE_BT_AUDIO
typedef struct
{   char text[32]; } callback_context_t;

bool encode_string(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    // ...and you always cast to the same pointer type, reducing
    // the chance of mistakes
    char * str = (char *)(*arg);
    DBGLOG(Warning, "Encoding string: %s", str);

    if (!pb_encode_tag_for_field(stream, field))
        return false;
    return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

size_t KiSCProto::encodeBluetoothAudioMessage(BluetoothAudioMessage bam, char *artist, char *title, char *album) {
    DBGLOG(Verbose, "Encoding Bluetooth Audio Message");
    char buffer1[ESPNOW_MAX_STR];
    char buffer2[ESPNOW_MAX_STR];
    char buffer3[ESPNOW_MAX_STR];
    memset(buffer1, 0, ESPNOW_MAX_STR);
    memset(buffer2, 0, ESPNOW_MAX_STR);
    memset(buffer3, 0, ESPNOW_MAX_STR);
    strncpy(buffer1, artist, sizeof(buffer1)-1);
    strncpy(buffer2, title, sizeof(buffer2)-1);
    strncpy(buffer3, album, sizeof(buffer3)-1);
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bam.bta.funcs.encode = &encode_string;
    bam.bts.funcs.encode = &encode_string;
    bam.bta.arg = &buffer1;
    bam.bts.arg = &buffer2;
    bool status = pb_encode(&stream, BluetoothAudioMessage_fields, &bam);
    send_buffer[0] = MSG_TYPE_BLUETOOTH_AUDIO_MESSAGE;
    #ifndef ARDUINO_ARCH_ESP32
    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
        DBGLOG(Error, "Encoding failed: %s", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length+1;
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
//    delay(5); // ESP8266 needs it or die
    #endif
    size_t message_length = stream.bytes_written;
    if (!status) {
#if USE_LOGGER
        DBGLOG(Error, "Encoding failed: %s", PB_GET_ERROR(&stream));
#else
        ESP_LOGE("ESPNow", "Encoding failed: %s", PB_GET_ERROR(&stream));
#endif

//        if(devmode) printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
        return 0;
    }
    return message_length+1;
}
#endif

#if PROTOBUF_USE_LIGHT
size_t KiSCProto::encodeLightMessage(LightMessage lm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, LightMessage_fields, &lm);
    send_buffer[0] = MSG_TYPE_LIGHT_MESSAGE;
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
#if PROTOBUF_USE_MOTOR
size_t KiSCProto::encodeMotorMessage(MotorboardFeedback mm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, MotorboardFeedback_fields, &mm);
    send_buffer[0] = MSG_TYPE_MOTOR_MESSAGE;
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
size_t KiSCProto::encodeMotorControlMessage(MotorboardControl mcm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, MotorboardControl_fields, &mcm);
    send_buffer[0] = MSG_TYPE_MOTOR_CONTROL_MESSAGE;
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
#if PROTOBUF_USE_SYSTEM
size_t KiSCProto::encodeSystemMessage(SysMessage sm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, SysMessage_fields, &sm);
    send_buffer[0] = MSG_TYPE_SYSTEM_MESSAGE;
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
#if PROTOBUF_USE_SOUND_GENERATOR
size_t KiSCProto::encodeSoundGeneratorMessage(SoundGeneratorMessage sgm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, SoundGeneratorMessage_fields, &sgm);
    send_buffer[0] = MSG_TYPE_SOUND_GENERATOR_MESSAGE;
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
size_t KiSCProto::encodeSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, SoundGeneratorControlMessage_fields, &sgcm);
    send_buffer[0] = MSG_TYPE_SOUND_GENERATOR_CONTROL_MESSAGE;
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
#if PROTOBUF_USE_DISPLAY
size_t KiSCProto::encodeDisplayMessage(DisplayMessage dm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, DisplayMessage_fields, &dm);
    send_buffer[0] = MSG_TYPE_DISPLAY_MESSAGE;
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
#if PROTOBUF_USE_PERIPHERALS
size_t KiSCProto::encodePeripheralsMessage(PeripheralsControlMessage pm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, PeripheralsControlMessage_fields, &pm);
    send_buffer[0] = MSG_TYPE_PERIPHERALS_MESSAGE;
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
size_t KiSCProto::encodePeripheralsFeedbackMessage(PeripheralsFeedbackMessage pfm) {
    pb_ostream_t stream = pb_ostream_from_buffer(send_buffer+1, sizeof(send_buffer)-1);
    bool status = pb_encode(&stream, PeripheralsFeedbackMessage_fields, &pfm);
    send_buffer[0] = MSG_TYPE_PERIPHERALS_FEEDBACK;
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

bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint8_t buffer[ESPNOW_MAX_STR] = {0};
    
    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > sizeof(buffer) - 1)
        return false;
    
    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;
    
    /* Print the string, in format comparable with protoc --decode.
     * Format comes from the arg defined in main().
     */
    sprintf((char*)*arg, "%s", buffer);
    return true;
}


#if PROTOBUF_USE_BT_AUDIO
bool BluetoothAudioMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    char *buffer1 = (char *)malloc(ESPNOW_MAX_STR);
    char *buffer2 = (char *)malloc(ESPNOW_MAX_STR);
    char *buffer3 = (char *)malloc(ESPNOW_MAX_STR);

    _bam->bta.funcs.decode = &decode_string;
    _bam->bts.funcs.decode = &decode_string;
    _bam->bta.arg = &buffer1;
    _bam->bts.arg = &buffer2;
    bool status = pb_decode(&stream, BluetoothAudioMessage_fields, _bam);
    if (!status) {
        DBGLOG(Error, "Decoding bluetooth audio msg failed: %s", PB_GET_ERROR(&stream));
//        if(joystick.devmode) printf("Decoding bluetooth audio msg failed: %s\r\n", PB_GET_ERROR(&stream));
        free(buffer1);
        free(buffer2);
        free(buffer3);
        return false;
    }
    if (kiscproto._pBluetoothAudioMessageCallbacks != nullptr) {
        kiscproto._pBluetoothAudioMessageCallbacks->onBluetoothAudioMessage(*_bam, buffer1, buffer2, buffer3);
    }
    free(buffer1);
    free(buffer2);
    free(buffer3);
    return true;
}

bool BluetoothAudioControlMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, BluetoothAudioControlMessage_fields, _bacm);
    if (!status) {
//        if(joystick.devmode) printf("Decoding bluetooth audio control msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pBluetoothAudioControlMessageCallbacks != nullptr) {
        kiscproto._pBluetoothAudioControlMessageCallbacks->onBluetoothAudioControlMessage(*_bacm);
    }
    return true;
}
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
bool RemotecontrolMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, RemotecontrolMessage_fields, &_rcm);
    if (!status) {
#if USE_LOGGER
        DBGLOG(Error, "Decoding remote control msg failed: %s", PB_GET_ERROR(&stream));
#else
        ESP_LOGE("ESPNow", "Decoding remote control msg failed: %s", PB_GET_ERROR(&stream));
#endif        
//        if(joystick.devmode) printf("Decoding remote control msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pRemotecontrolMessageCallbacks != nullptr) {
#if USE_LOGGER
#endif                
        kiscproto._pRemotecontrolMessageCallbacks->onRemotecontrolMessage(_rcm);
    }
    return true;
}
#endif

#if PROTOBUF_USE_LIGHT
bool LightMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, LightMessage_fields, _lm);
    if (!status) {
//        if(joystick.devmode) printf("Decoding light msg failed: %s\r\n", PB_GET_ERROR(&stream));
        return false;
    }
    if (kiscproto._pLightMessageCallbacks != nullptr) {
        kiscproto._pLightMessageCallbacks->onLightMessage(*_lm);
    }
    return true;
}
#endif

#if PROTOBUF_USE_SOUND_GENERATOR
bool SoundGeneratorMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, SoundGeneratorMessage_fields, _sgm);
    if (!status) {
        return false;
    }
    if (kiscproto._pSoundGeneratorMessageCallbacks != nullptr) {
        kiscproto._pSoundGeneratorMessageCallbacks->onSoundGeneratorMessage(*_sgm);
    }
    return true;
}
bool SoundGeneratorControlMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, SoundGeneratorControlMessage_fields, _sgcm);
    if (!status) {
        return false;
    }
    if (kiscproto._pSoundGeneratorControlMessageCallbacks != nullptr) {
        kiscproto._pSoundGeneratorControlMessageCallbacks->onSoundGeneratorControlMessage(*_sgcm);
    }
    return true;
}
#endif
#if PROTOBUF_USE_DISPLAY
bool DisplayMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, DisplayMessage_fields, &_dm);
    if (!status) {
        return false;
    }
    if (kiscproto._pDisplayMessageCallbacks != nullptr) {
        kiscproto._pDisplayMessageCallbacks->onDisplayMessage(_dm);
    }
    return true;
}
#endif


#if PROTOBUF_USE_MOTOR
bool MotorMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, MotorboardFeedback_fields, &_mm);
    if (!status) {
        return false;
    }
    if (kiscproto._pMotorMessageCallbacks != nullptr) {
        kiscproto._pMotorMessageCallbacks->onMotorMessage(_mm);
    }
    return true;
}
bool MotorControlMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, MotorboardControl_fields, &_mcm);
    if (!status) {
        return false;
    }
    if (kiscproto._pMotorControlMessageCallbacks != nullptr) {
        kiscproto._pMotorControlMessageCallbacks->onMotorControlMessage(_mcm);
    }
    return true;
}
#endif
#if PROTOBUF_USE_SYSTEM
bool SystemMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, SysMessage_fields, _sm);
    if (!status) {
        return false;
    }
    if (kiscproto._pSystemMessageCallbacks != nullptr) {
        kiscproto._pSystemMessageCallbacks->onSystemMessage(*_sm);
    }
    return true;
}
#endif
#if PROTOBUF_USE_PERIPHERALS
bool PeripheralsMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, PeripheralsControlMessage_fields, &_pm);
    if (!status) {
        return false;
    }
    if (kiscproto._pPeripheralsMessageCallbacks != nullptr) {
        kiscproto._pPeripheralsMessageCallbacks->onPeripheralsMessage(_pm);
    }
    return true;
}
bool PeripheralsFeedbackMessageDecodeMessage(uint16_t message_length) {
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, message_length);
    bool status = pb_decode(&stream, PeripheralsFeedbackMessage_fields, &_pfm);
    if (!status) {
        return false;
    }
    if (kiscproto._pPeripheralsFeedbackCallbacks != nullptr) {
        kiscproto._pPeripheralsFeedbackCallbacks->onPeripheralsFeedback(_pfm);
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
#if PROTOBUF_USE_LIGHT
    if (_pLightMessageCallbacks != nullptr) {
        _pLightMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_MOTOR
    if (_pMotorMessageCallbacks != nullptr) {
        _pMotorMessageCallbacks->onError(msg);
    }
    if (_pMotorControlMessageCallbacks != nullptr) {
        _pMotorControlMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_SYSTEM
    if (_pSystemMessageCallbacks != nullptr) {
        _pSystemMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
    if (_pSoundGeneratorMessageCallbacks != nullptr) {
        _pSoundGeneratorMessageCallbacks->onError(msg);
    }
    if (_pSoundGeneratorControlMessageCallbacks != nullptr) {
        _pSoundGeneratorControlMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_DISPLAY
    if (_pDisplayMessageCallbacks != nullptr) {
        _pDisplayMessageCallbacks->onError(msg);
    }
#endif
#if PROTOBUF_USE_PERIPHERALS
    if (_pPeripheralsMessageCallbacks != nullptr) {
        _pPeripheralsMessageCallbacks->onError(msg);
    }
    if (_pPeripheralsFeedbackCallbacks != nullptr) {
        _pPeripheralsFeedbackCallbacks->onError(msg);
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
#if PROTOBUF_USE_LIGHT
LightMessage KiSCProto::newLightMessage() {
    LightMessage lm = LightMessage_init_zero;
    return lm;
}
#endif
#if PROTOBUF_USE_MOTOR
MotorboardFeedback KiSCProto::newMotorMessage() {
    MotorboardFeedback mm = MotorboardFeedback_init_zero;
    return mm;
}
MotorboardControl KiSCProto::newMotorControlMessage() {
    MotorboardControl mcm = MotorboardControl_init_zero;
    return mcm;
}
#endif
#if PROTOBUF_USE_SYSTEM
SysMessage KiSCProto::newSystemMessage() {
    SysMessage sm = SysMessage_init_zero;
    return sm;
}
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
SoundGeneratorMessage KiSCProto::newSoundGeneratorMessage() {
    SoundGeneratorMessage sgm = SoundGeneratorMessage_init_zero;
    return sgm;
}
SoundGeneratorControlMessage KiSCProto::newSoundGeneratorControlMessage() {
    SoundGeneratorControlMessage sgcm = SoundGeneratorControlMessage_init_zero;
    return sgcm;
}
#endif
#if PROTOBUF_USE_DISPLAY
DisplayMessage KiSCProto::newDisplayMessage() {
    DisplayMessage dm = DisplayMessage_init_zero;
    return dm;
}
#endif


// typedef void (*esp_now_recv_cb_t)(const uint8_t *mac_addr, const uint8_t *data, int data_len);

void UniversalMessageRecvCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
//    const uint8_t *macAddr = recv_info->src_addr;
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
#if PROTOBUF_USE_LIGHT
        case MSG_TYPE_LIGHT_MESSAGE:
            if (kiscproto._pLightMessageCallbacks != nullptr) {
                LightMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_MOTOR
        case MSG_TYPE_MOTOR_MESSAGE:
            if (kiscproto._pMotorMessageCallbacks != nullptr) {
                MotorMessageDecodeMessage(msgLen);
            }
            break;
        case MSG_TYPE_MOTOR_CONTROL_MESSAGE:        
            if (kiscproto._pMotorControlMessageCallbacks != nullptr) {
                MotorControlMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_SYSTEM
        case MSG_TYPE_SYSTEM_MESSAGE:
            if (kiscproto._pSystemMessageCallbacks != nullptr) {
                SystemMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
        case MSG_TYPE_SOUND_GENERATOR_MESSAGE:
            if (kiscproto._pSoundGeneratorMessageCallbacks != nullptr) {
                SoundGeneratorMessageDecodeMessage(msgLen);
            }
            break;
        case MSG_TYPE_SOUND_GENERATOR_CONTROL_MESSAGE:
            if (kiscproto._pSoundGeneratorControlMessageCallbacks != nullptr) {
                SoundGeneratorControlMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_DISPLAY
        case MSG_TYPE_DISPLAY_MESSAGE:
            if (kiscproto._pDisplayMessageCallbacks != nullptr) {
                DisplayMessageDecodeMessage(msgLen);
            }
            break;
#endif
#if PROTOBUF_USE_PERIPHERALS
        case MSG_TYPE_PERIPHERALS_MESSAGE:
            if (kiscproto._pPeripheralsMessageCallbacks != nullptr) {
                PeripheralsMessageDecodeMessage(msgLen);
            }
            break;
        case MSG_TYPE_PERIPHERALS_FEEDBACK:
            if (kiscproto._pPeripheralsFeedbackCallbacks != nullptr) {
                PeripheralsFeedbackMessageDecodeMessage(msgLen);
            }
            break;
#endif



        default:
        #if USE_LOGGER
            DBGLOG(Error, "Unknown message type: %i", msgType);
        #else
            ESP_LOGE("ESPNow", "Unknown message type: %i", msgType);
        #endif
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
#if PROTOBUF_USE_LIGHT
void LightMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
#if PROTOBUF_USE_MOTOR
void MotorMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
void MotorControlMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif

#if PROTOBUF_USE_SYSTEM
void SystemMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
void SoundGeneratorMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
void SoundGeneratorControlMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
#if PROTOBUF_USE_DISPLAY
void DisplayMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif
#if PROTOBUF_USE_PERIPHERALS
void PeripheralsMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
void PeripheralsFeedbackMessageSendCallback(const uint8_t *macAddr, esp_now_send_status_t status) {

}
#endif


bool KiSCProto::sendMessage(uint32_t msglen, const uint8_t *mac) {
    #ifdef ARDUINO_ARCH_ESP32
    esp_now_peer_info_t peerInfo = {};
    memcpy(&peerInfo.peer_addr, mac, 6);
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_add_peer(&peerInfo);
    }
#if USE_LOGGER    
    DBGLOG(Debug, "Sending message to: %s", getFormattedMacAddr(mac).c_str());
#else
    ESP_LOGI("ESPNow", "Sending message to: %s", getFormattedMacAddr(mac).c_str());
#endif
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
#else
        ESP_LOGI("ESPNow", "Broadcast message success");
        ESP_LOGI("ESPNow", "Send message size: %lu", msglen);
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
    ESP_LOGI("ESPNow", "receiverId: %lu [%s]", ka.first, macStr);
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
    ESP_LOGI("ESPNow", "New receiverId: %lu with MAC: ", id);
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

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define ESPNOW_WIFI_CHANNEL 1
/* WiFi should start before using ESPNOW */
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.static_rx_buf_num = 2; // War 8
    cfg.dynamic_rx_buf_num = 4; // War 32
    cfg.static_tx_buf_num = 2; // War 8

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

bool 
KiSCProto::init() {

    /*
    BluetoothAudioMessage *_bam = nullptr;
BluetoothAudioControlMessage *_bacm = nullptr;

*/
/*    WiFi.mode(WIFI_STA);
    // startup ESP Now
#if USE_LOGGER
    DBGLOG(Info, "ESPNow Init");
    DBGLOG(Info, "ESPNow MAC: %s", WiFi.macAddress().c_str());
#else
    ESP_LOGI("ESPNow", "ESPNow Init");
    ESP_LOGI("ESPNow", "ESPNow MAC: %s", WiFi.macAddress().c_str());    
#endif    */
    // shutdown wifi
    DBGLOG(Info, "Initializing global variables");

#ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
#if PROTOBUF_USE_BT_AUDIO
        _bam = (BluetoothAudioMessage *)ps_malloc(sizeof(BluetoothAudioMessage));
        _bacm = (BluetoothAudioControlMessage *)ps_malloc(sizeof(BluetoothAudioControlMessage));
#endif        
#if PROTOBUF_USE_SYSTEM
        _sm = (SysMessage *)ps_malloc(sizeof(SysMessage));
#endif
#if PROTOBUF_USE_LIGHT
        _lm = (LightMessage *)ps_malloc(sizeof(LightMessage));
#endif
#if PROTOBUF_USE_MOTOR
        _mm = (MotorboardFeedback *)ps_malloc(sizeof(MotorboardFeedback));
        _mcm = (MotorboardControl *)ps_malloc(sizeof(MotorboardControl));
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
        _sgm = (SoundGeneratorMessage *)ps_malloc(sizeof(SoundGeneratorMessage));
        _sgcm = (SoundGeneratorControlMessage *)ps_malloc(sizeof(SoundGeneratorControlMessage));
#endif
#if PROTOBUF_USE_DISPLAY
        _dm = (DisplayMessage *)ps_malloc(sizeof(DisplayMessage));
#endif
#if PROTOBUF_USE_PERIPHERALS
        _pm = (PeripheralsControlMessage *)ps_malloc(sizeof(PeripheralsControlMessage));
        _pfm = (PeripheralsFeedbackMessage *)ps_malloc(sizeof(PeripheralsFeedbackMessage));
#endif

    } else {
        DBGLOG(Error, "No PSRAM found");
        return false;
    }
#else
#if PROTOBUF_USE_BT_AUDIO
    _bam = (BluetoothAudioMessage *)malloc(sizeof(BluetoothAudioMessage));
    _bacm = (BluetoothAudioControlMessage *)malloc(sizeof(BluetoothAudioControlMessage));    
#endif    
#if PROTOBUF_USE_SYSTEM
    _sm = (SysMessage *)malloc(sizeof(SysMessage));
#endif
#if PROTOBUF_USE_LIGHT
    _lm = (LightMessage *)malloc(sizeof(LightMessage));
#endif    
#if PROTOBUF_USE_MOTOR
    _mm = (MotorboardFeedback *)malloc(sizeof(MotorboardFeedback));
    _mcm = (MotorboardControl *)malloc(sizeof(MotorboardControl));
    #endif
#if PROTOBUF_USE_SOUND_GENERATOR
    _sgm = (SoundGeneratorMessage *)malloc(sizeof(SoundGeneratorMessage));
    _sgcm = (SoundGeneratorControlMessage *)malloc(sizeof(SoundGeneratorControlMessage));
    #   endif
#if PROTOBUF_USE_DISPLAY
    _dm = (DisplayMessage *)malloc(sizeof(DisplayMessage));
    #endif
#if PROTOBUF_USE_PERIPHERALS
    _pm = (PeripheralsControlMessage *)malloc(sizeof(PeripheralsControlMessage));
    _pfm = (PeripheralsFeedbackMessage *)malloc(sizeof(PeripheralsFeedbackMessage));
    #endif

#endif

    DBGLOG(Warning, "Free heap: %d", ESP.getFreeHeap());
    DBGLOG(Info, "KiSCProto init WiFi");
    example_wifi_init();
    DBGLOG(Warning, "Free heap: %d", ESP.getFreeHeap());
     
//    WiFi.disconnect();
//    delay(100);
    DBGLOG(Info, "Initializing ESP-Now");

    #ifdef ARDUINO_ARCH_ESP32
    if (esp_now_init() != ESP_OK) {
#if USE_LOGGER
        DBGLOG(Error, "Error initializing ESP-NOW");
#else
        ESP_LOGE("ESPNow", "Error initializing ESP-NOW");        
#endif        
        return false;
    }
    DBGLOG(Warning, "Free heap: %d", ESP.getFreeHeap());
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
    DBGLOG(Warning, "End of init() Free heap: %d", ESP.getFreeHeap());
    return true;
}


