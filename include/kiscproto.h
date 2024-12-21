#ifndef INCLUDE_KISCPROTO_INCLUDED
#define INCLUDE_KISCPROTO_INCLUDED


/*
henni@Desktop MINGW64 /d/Development/KiSC/KiSC-Dashboard/KiSC-ESP-Now-Protocol/src (ProtoBuf)
$ python3 ../../.pio/libdeps/esp32dev/Nanopb/generator/nanopb_generator.py kisc.proto 
Writing to kisc.pb.h and kisc.pb.c


*/
#define PROTOBUF_ALL_MESSAGES    0
#if PROTOBUF_ALL_MESSAGES
#define PROTOBUF_USE_BT_AUDIO          1
#define PROTOBUF_USE_REMOTE_CONTROL    1
#define PROTOBUF_USE_LIGHT             1
#define PROTOBUF_USE_MOTOR             1
#define PROTOBUF_USE_SOUND_GENERATOR   1
#define PROTOBUF_USE_DISPLAY           1
#else
#define PROTOBUF_USE_BT_AUDIO          0
#define PROTOBUF_USE_REMOTE_CONTROL    1
#define PROTOBUF_USE_LIGHT             0
#define PROTOBUF_USE_MOTOR             0
#define PROTOBUF_USE_SOUND_GENERATOR   0
#define PROTOBUF_USE_DISPLAY           0
#endif


#if PROTOBUF_USE_BT_AUDIO
#define MSG_TYPE_BLUETOOTH_AUDIO_MESSAGE 1
#define MSG_TYPE_BLUETOOTH_AUDIO_CONTROL_MESSAGE 2
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
#define MSG_TYPE_REMOTECONTROL_MESSAGE 3
#endif

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

#define USE_LOGGER 0
#if PROTOBUF_USE_BT_AUDIO
class BluetoothAudioMessageCallbacks;
class BluetoothAudioControlMessageCallbacks;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
class RemotecontrolMessageCallbacks;
#endif

class KiSCProto {
   public:

    /// basic constuctor. Please see the init command
    KiSCProto();
#if PROTOBUF_USE_BT_AUDIO    
    BluetoothAudioMessage           newBluetoothAudioMessage();
    BluetoothAudioControlMessage    newBluetoothAudioControlMessage();
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    RemotecontrolMessage            newRemotecontrolMessage();
#endif    

#if PROTOBUF_USE_BT_AUDIO
    void setBluetoothAudioMessageCallbacks(BluetoothAudioMessageCallbacks* pCallbacks);
    void setBluetoothAudioControlMessageCallbacks(BluetoothAudioControlMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    void setRemotecontrolMessageCallbacks(RemotecontrolMessageCallbacks* pCallbacks);
#endif

#if PROTOBUF_USE_BT_AUDIO
    bool setBluetoothAudioMessageArtist(BluetoothAudioMessage bam, const char *artist);
    bool setBluetoothAudioMessageTitle(BluetoothAudioMessage bam, const char *title);
    bool sendBluetoothAudioMessage(BluetoothAudioMessage bam);
    bool sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    bool sendRemotecontrolMessage(RemotecontrolMessage rcm);
#endif
#if PROTOBUF_USE_BT_AUDIO
    bool sendBluetoothAudioMessage(BluetoothAudioMessage bam, const uint8_t* mac);
    bool sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    bool sendRemotecontrolMessage(RemotecontrolMessage rcm, const uint8_t* mac);
#endif    

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
#if PROTOBUF_USE_BT_AUDIO    
    BluetoothAudioMessageCallbacks* _pBluetoothAudioMessageCallbacks = nullptr;
    BluetoothAudioControlMessageCallbacks* _pBluetoothAudioControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
    RemotecontrolMessageCallbacks* _pRemotecontrolMessageCallbacks = nullptr;
#endif    

    /// current mac target (default: broadcasting)
    uint8_t targetAddress [6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    bool init();        
 private:
#if PROTOBUF_USE_BT_AUDIO 
    size_t encodeBluetoothAudioMessage(BluetoothAudioMessage bam);
    size_t encodeBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    size_t encodeRemotecontrolMessage(RemotecontrolMessage rcm);
#endif    

    void reportError(const char *msg);

    bool sendMessage(uint32_t msglen);

    bool sendMessage(uint32_t msglen, const uint8_t *mac); 
    String _ESP_ID; 
};

/*
undefined reference to `typeinfo for BluetoothAudioMessageCallbacks'
undefined reference to `BluetoothAudioMessageCallbacks::onBluetoothAudioMessage(_BluetoothAudioMessage)'
*/
#if PROTOBUF_USE_BT_AUDIO
class BluetoothAudioMessageCallbacks {
   public:
    virtual ~BluetoothAudioMessageCallbacks(){};
    virtual void onBluetoothAudioMessage(BluetoothAudioMessage bam){};
    virtual void onError(const char *msg){};
};

class BluetoothAudioControlMessageCallbacks {
   public:
    virtual ~BluetoothAudioControlMessageCallbacks(){};
    virtual void onBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm){};
    virtual void onError(const char *msg){};
};
#endif

#if PROTOBUF_USE_REMOTE_CONTROL
class RemotecontrolMessageCallbacks {
   public:
    virtual ~RemotecontrolMessageCallbacks(){};
    virtual void onRemotecontrolMessage(RemotecontrolMessage rcm){};
    virtual void onError(const char *msg){};
};
#endif

extern KiSCProto kiscproto;

#endif  /* INCLUDE_KISCPROTO_INCLUDED */
