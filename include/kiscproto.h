#ifndef _KISC_PROTO_H_
#define _KISC_PROTO_H_


/*
henni@Desktop MINGW64 /d/Development/KiSC/KiSC-Dashboard/KiSC-ESP-Now-Protocol/src (ProtoBuf)
$ python3 ../../.pio/libdeps/esp32dev/Nanopb/generator/nanopb_generator.py kisc.proto 
Writing to kisc.pb.h and kisc.pb.c


*/
#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <esp_now.h>
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
#define ESP_OK 0
#endif

#include <pb_decode.h>
#include <pb_encode.h>
#include "../src/kisc.pb.h"

#include <map>
#include <vector>
#include <string>

#define USE_LOGGER 1

class BluetoothAudioMessageCallbacks;
class BluetoothAudioControlMessageCallbacks;

class KiSCProto {
   public:

    /// basic constuctor. Please see the init command
    KiSCProto();
    
    BluetoothAudioMessage           newBluetoothAudioMessage();
    BluetoothAudioControlMessage    newBluetoothAudioControlMessage();

    void setBluetoothAudioMessageCallbacks(BluetoothAudioMessageCallbacks* pCallbacks);
    void setBluetoothAudioControlMessageCallbacks(BluetoothAudioControlMessageCallbacks* pCallbacks);

    bool sendBluetoothAudioMessage(BluetoothAudioMessage bam);
    bool sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm);

    bool sendBluetoothAudioMessage(BluetoothAudioMessage bam, const uint8_t* mac);
    bool sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm, const uint8_t* mac);

    /// print all receivers detected in the current session
    void printReceivers();

    /// utility for print formatted mac address 
    std::string getFormattedMacAddr(const uint8_t *mac);

    /// getter to retreive all receivers vector
    std::vector<uint32_t> getReceivers();

    /// get mac address from the receiverId
    const uint8_t * getReceiverMacAddr(uint32_t receiverId);

    /// get the current instance of this object
    KiSCProto* getInstance();


    /// callback for BluetoothAudioMessage
    BluetoothAudioMessageCallbacks* _pBluetoothAudioMessageCallbacks = nullptr;
    BluetoothAudioControlMessageCallbacks* _pBluetoothAudioControlMessageCallbacks = nullptr;

    /// current mac target (default: broadcasting)
    uint8_t targetAddress [6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    bool init();        
 private:
    size_t encodeBluetoothAudioMessage(BluetoothAudioMessage bam);
    size_t encodeBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm);

    void reportError(const char *msg);

    bool sendMessage(uint32_t msglen);

    bool sendMessage(uint32_t msglen, const uint8_t *mac); 
    String _ESP_ID; 
};

class BluetoothAudioMessageCallbacks {
   public:
    virtual ~BluetoothAudioMessageCallbacks(){};
    virtual void onBluetoothAudioMessage(BluetoothAudioMessage bam) {};
    virtual void onError(const char *msg){};
};

class BluetoothAudioControlMessageCallbacks {
   public:
    virtual ~BluetoothAudioControlMessageCallbacks(){};
    virtual void onBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm){};
    virtual void onError(const char *msg){};
};

extern KiSCProto kiscproto;

#endif