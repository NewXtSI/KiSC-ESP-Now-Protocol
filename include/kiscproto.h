#ifndef INCLUDE_KISCPROTO_INCLUDED
#define INCLUDE_KISCPROTO_INCLUDED

#define USE_LOGGER 1
/*
henni@Desktop MINGW64 /d/Development/KiSC/KiSC-Dashboard/KiSC-ESP-Now-Protocol/src (ProtoBuf)
$ python3 ../../.pio/libdeps/esp32dev/Nanopb/generator/nanopb_generator.py kisc.proto 
Writing to kisc.pb.h and kisc.pb.c


*/
#ifndef PROTOBUF_ALL_MESSAGES
#define PROTOBUF_ALL_MESSAGES    0
#endif

#if PROTOBUF_ALL_MESSAGES
#define PROTOBUF_USE_BT_AUDIO          1
#define PROTOBUF_USE_REMOTE_CONTROL    1
#define PROTOBUF_USE_LIGHT             1
#define PROTOBUF_USE_MOTOR             1
#define PROTOBUF_USE_SOUND_GENERATOR   1
#define PROTOBUF_USE_DISPLAY           1
#define PROTOBUF_USE_SYSTEM            1  
#define PROTOBUF_USE_PERIPHERALS       1
#else
#ifndef PROTOBUF_USE_BT_AUDIO
#define PROTOBUF_USE_BT_AUDIO          1
#endif
#ifndef PROTOBUF_USE_REMOTE_CONTROL
#define PROTOBUF_USE_REMOTE_CONTROL    1
#endif
#ifndef PROTOBUF_USE_LIGHT
#define PROTOBUF_USE_LIGHT             1
#endif
#ifndef PROTOBUF_USE_MOTOR
#define PROTOBUF_USE_MOTOR             0
#endif
#ifndef PROTOBUF_USE_SOUND_GENERATOR
#define PROTOBUF_USE_SOUND_GENERATOR   1
#endif
#ifndef PROTOBUF_USE_DISPLAY
#define PROTOBUF_USE_DISPLAY           0
#endif
#ifndef PROTOBUF_USE_SYSTEM
#define PROTOBUF_USE_SYSTEM            1
#endif
#endif
#ifndef PROTOBUF_USE_PERIPHERALS
#define PROTOBUF_USE_PERIPHERALS       0
#endif


#if PROTOBUF_USE_BT_AUDIO
#define MSG_TYPE_BLUETOOTH_AUDIO_MESSAGE 1
#define MSG_TYPE_BLUETOOTH_AUDIO_CONTROL_MESSAGE 2
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
#define MSG_TYPE_REMOTECONTROL_MESSAGE 3
#endif
#if PROTOBUF_USE_LIGHT
#define MSG_TYPE_LIGHT_MESSAGE 4
#endif
#if PROTOBUF_USE_MOTOR
#define MSG_TYPE_MOTOR_MESSAGE 5
#define MSG_TYPE_MOTOR_CONTROL_MESSAGE 6
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
#define MSG_TYPE_SOUND_GENERATOR_MESSAGE 7
#define MSG_TYPE_SOUND_GENERATOR_CONTROL_MESSAGE 8
#endif
#if PROTOBUF_USE_DISPLAY
#define MSG_TYPE_DISPLAY_MESSAGE 9
#endif
#if PROTOBUF_USE_SYSTEM
#define MSG_TYPE_SYSTEM_MESSAGE 10
#endif	
#if PROTOBUF_USE_PERIPHERALS
#define MSG_TYPE_PERIPHERALS_MESSAGE 11
#define MSG_TYPE_PERIPHERALS_FEEDBACK 12
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

#if PROTOBUF_USE_BT_AUDIO
class BluetoothAudioMessageCallbacks;
class BluetoothAudioControlMessageCallbacks;
#endif
#if PROTOBUF_USE_REMOTE_CONTROL
class RemotecontrolMessageCallbacks;
#endif
#if PROTOBUF_USE_LIGHT
class LightMessageCallbacks;
#endif
#if PROTOBUF_USE_MOTOR
class MotorMessageCallbacks;
class MotorControlMessageCallbacks;
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
class SoundGeneratorMessageCallbacks;
class SoundGeneratorControlMessageCallbacks;
#endif
#if PROTOBUF_USE_DISPLAY
class DisplayMessageCallbacks;
#endif
#if PROTOBUF_USE_SYSTEM
class SystemMessageCallbacks;
#endif
#if PROTOBUF_USE_PERIPHERALS
class PeripheralsMessageCallbacks;
class PeripheralsFeedbackCallbacks;
#endif

class KiSCProto {
   public:

    /// basic constuctor. Please see the init command
    KiSCProto();
#if PROTOBUF_USE_BT_AUDIO    
    BluetoothAudioMessage           newBluetoothAudioMessage();
    BluetoothAudioControlMessage    newBluetoothAudioControlMessage();
#endif
#if PROTOBUF_USE_MOTOR
    MotorboardFeedback              newMotorMessage();
    MotorboardControl               newMotorControlMessage();
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    RemotecontrolMessage            newRemotecontrolMessage();
#endif    
#if PROTOBUF_USE_LIGHT
    LightMessage                    newLightMessage();
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
    SoundGeneratorMessage           newSoundGeneratorMessage();
    SoundGeneratorControlMessage    newSoundGeneratorControlMessage();
#endif
#if PROTOBUF_USE_DISPLAY
    DisplayMessage                  newDisplayMessage();
#endif
#if PROTOBUF_USE_SYSTEM
    SysMessage                   newSystemMessage();
#endif

    /// set the callbacks for the messages
#if PROTOBUF_USE_BT_AUDIO
    void setBluetoothAudioMessageCallbacks(BluetoothAudioMessageCallbacks* pCallbacks);
    void setBluetoothAudioControlMessageCallbacks(BluetoothAudioControlMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    void setRemotecontrolMessageCallbacks(RemotecontrolMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_MOTOR
      void setMotorMessageCallbacks(MotorMessageCallbacks* pCallbacks);
      void setMotorControlMessageCallbacks(MotorControlMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_LIGHT
      void setLightMessageCallbacks(LightMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
      void setSoundGeneratorMessageCallbacks(SoundGeneratorMessageCallbacks* pCallbacks);
      void setSoundGeneratorControlMessageCallbacks(SoundGeneratorControlMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_DISPLAY
      void setDisplayMessageCallbacks(DisplayMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_SYSTEM
      void setSystemMessageCallbacks(SystemMessageCallbacks* pCallbacks);
#endif
#if PROTOBUF_USE_PERIPHERALS
    void setPeripheralsMessageCallbacks(PeripheralsMessageCallbacks* pCallbacks);
    void setPeripheralsFeedbackCallbacks(PeripheralsFeedbackCallbacks* pCallbacks);
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
#if PROTOBUF_USE_LIGHT
    bool sendLightMessage(LightMessage lm);
#endif
#if PROTOBUF_USE_MOTOR
    bool sendMotorMessage(MotorboardFeedback mm);
    bool sendMotorControlMessage(MotorboardControl mcm);
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
    bool sendSoundGeneratorMessage(SoundGeneratorMessage sgm);
    bool sendSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm);
#endif
#if PROTOBUF_USE_DISPLAY
    bool sendDisplayMessage(DisplayMessage dm);
#endif   
#if PROTOBUF_USE_SYSTEM
    bool sendSystemMessage(SysMessage sm);
#endif
#if PROTOBUF_USE_BT_AUDIO
    bool sendBluetoothAudioMessage(BluetoothAudioMessage bam, const uint8_t* mac);
    bool sendBluetoothAudioControlMessage(BluetoothAudioControlMessage bacm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_REMOTE_CONTROL    
    bool sendRemotecontrolMessage(RemotecontrolMessage rcm, const uint8_t* mac);
#endif    
#if PROTOBUF_USE_LIGHT
      bool sendLightMessage(LightMessage lm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_MOTOR
      bool sendMotorMessage(MotorboardFeedback mm, const uint8_t* mac);
      bool sendMotorControlMessage(MotorboardControl mcm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
      bool sendSoundGeneratorMessage(SoundGeneratorMessage sgm, const uint8_t* mac);
      bool sendSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_DISPLAY
      bool sendDisplayMessage(DisplayMessage dm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_SYSTEM
      bool sendSystemMessage(SysMessage sm, const uint8_t* mac);
#endif
#if PROTOBUF_USE_PERIPHERALS
    bool sendPeripheralsMessage(PeripheralsControlMessage pm);
    bool sendPeripheralsFeedbackMessage(PeripheralsFeedbackMessage pfm);
    bool sendPeripheralsMessage(PeripheralsControlMessage pm, const uint8_t* mac);
    bool sendPeripheralsFeedbackMessage(PeripheralsFeedbackMessage pfm, const uint8_t* mac);
    
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
#if PROTOBUF_USE_LIGHT
      LightMessageCallbacks* _pLightMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_MOTOR
      MotorMessageCallbacks* _pMotorMessageCallbacks = nullptr;
      MotorControlMessageCallbacks* _pMotorControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
      SoundGeneratorMessageCallbacks* _pSoundGeneratorMessageCallbacks = nullptr;
      SoundGeneratorControlMessageCallbacks* _pSoundGeneratorControlMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_DISPLAY
      DisplayMessageCallbacks* _pDisplayMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_SYSTEM
      SystemMessageCallbacks* _pSystemMessageCallbacks = nullptr;
#endif
#if PROTOBUF_USE_PERIPHERALS
    PeripheralsMessageCallbacks* _pPeripheralsMessageCallbacks = nullptr;
    PeripheralsFeedbackCallbacks* _pPeripheralsFeedbackCallbacks = nullptr;
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
#if PROTOBUF_USE_LIGHT
      size_t encodeLightMessage(LightMessage lm);
#endif
#if PROTOBUF_USE_MOTOR
      size_t encodeMotorMessage(MotorboardFeedback mm);
      size_t encodeMotorControlMessage(MotorboardControl mcm);
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
      size_t encodeSoundGeneratorMessage(SoundGeneratorMessage sgm);
      size_t encodeSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm);
#endif
#if PROTOBUF_USE_DISPLAY
      size_t encodeDisplayMessage(DisplayMessage dm);
#endif
#if PROTOBUF_USE_SYSTEM
      size_t encodeSystemMessage(SysMessage sm);
#endif
#if PROTOBUF_USE_PERIPHERALS
    size_t encodePeripheralsMessage(PeripheralsControlMessage pm);
    size_t encodePeripheralsFeedbackMessage(PeripheralsFeedbackMessage pfm);
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

#if PROTOBUF_USE_LIGHT
class LightMessageCallbacks {
   public:
    virtual ~LightMessageCallbacks(){};
    virtual void onLightMessage(LightMessage lm){};
    virtual void onError(const char *msg){};
};
#endif
#if PROTOBUF_USE_MOTOR
class MotorMessageCallbacks {
   public:
    virtual ~MotorMessageCallbacks(){};
    virtual void onMotorMessage(MotorboardFeedback mm){};
    virtual void onError(const char *msg){};
};
class MotorControlMessageCallbacks {
   public:
    virtual ~MotorControlMessageCallbacks(){};
    virtual void onMotorControlMessage(MotorboardControl mcm){};
    virtual void onError(const char *msg){};
};
#endif
#if PROTOBUF_USE_SOUND_GENERATOR
class SoundGeneratorMessageCallbacks {
   public:
    virtual ~SoundGeneratorMessageCallbacks(){};
    virtual void onSoundGeneratorMessage(SoundGeneratorMessage sgm){};
    virtual void onError(const char *msg){};
};
class SoundGeneratorControlMessageCallbacks {
   public:
    virtual ~SoundGeneratorControlMessageCallbacks(){};
    virtual void onSoundGeneratorControlMessage(SoundGeneratorControlMessage sgcm){};
    virtual void onError(const char *msg){};
};
#endif
#if PROTOBUF_USE_DISPLAY
class DisplayMessageCallbacks {
   public:
    virtual ~DisplayMessageCallbacks(){};
    virtual void onDisplayMessage(DisplayMessage dm){};
    virtual void onError(const char *msg){};
};
#endif
#if PROTOBUF_USE_SYSTEM
class SystemMessageCallbacks {
   public:
    virtual ~SystemMessageCallbacks(){};
    virtual void onSystemMessage(SysMessage sm){};
    virtual void onError(const char *msg){};
};
#endif

#if PROTOBUF_USE_PERIPHERALS
class PeripheralsMessageCallbacks {
   public:
    virtual ~PeripheralsMessageCallbacks(){};
    virtual void onPeripheralsMessage(PeripheralsControlMessage pm){};
    virtual void onError(const char *msg){};
};
class PeripheralsFeedbackCallbacks {
   public:
    virtual ~PeripheralsFeedbackCallbacks(){};
    virtual void onPeripheralsFeedback(PeripheralsFeedbackMessage pfm){};
    virtual void onError(const char *msg){};
};
#endif


extern KiSCProto kiscproto;

#endif  /* INCLUDE_KISCPROTO_INCLUDED */
