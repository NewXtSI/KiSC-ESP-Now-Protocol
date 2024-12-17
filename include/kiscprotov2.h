#ifndef INCLUDE_KISCPROTOV2_INCLUDED
#define INCLUDE_KISCPROTOV2_INCLUDED

#include <Arduino.h>
#if defined ESP8266
#error "ESP8266 not supported at the moment"
#endif

#include <vector>

class KiSCAddress {
    uint8_t address[6];
};

class KiSCProtoV2Message {
 public:
    KiSCProtoV2Message(uint8_t* address[], uint8_t* data, uint8_t len);
    KiSCAddress     target;
};

class KiSCPeer {
 public:
    enum SlaveType { Unidentified, Controller, Motor, Light, Sound, BTAudio, Display, Peripheral };
    enum Role { Master, Slave };
    enum State { Unknown, Idle, Running, Stopped };

    KiSCPeer() : address(), lastMsg(0), active(false), name(), role(Master), state(Unknown), type(Unidentified) {}
    KiSCPeer(KiSCAddress address, String name, Role role, State state, SlaveType type) : address(address), lastMsg(0),
            active(false), name(name), role(role), state(state), type(type) {}

    KiSCAddress     address;
    uint32_t        lastMsg;
    bool            active;

    String          name;
    Role            role;
    State           state;
    SlaveType       type;
};

typedef std::function<void (KiSCAddress address, KiSCProtoV2Message msg, signed int rssi, bool broadcast)> rcvdMsgCallback;

class KiSCProtoV2 {
 public:
                        KiSCProtoV2(String name, KiSCPeer::Role role);

    void                onReceived(rcvdMsgCallback callback) { rcvdMsg = callback; }

    void                start();

    const KiSCPeer&     getPeer() { return peer; }
 protected:
    bool                init();

    static void         dataSent(uint8_t* address, uint8_t status);
    static void         dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

    static void         task(void* param);
    void                send(KiSCProtoV2Message msg);
 private:
    void                _sendViaESPNow(KiSCProtoV2Message msg);

    static bool         ESPNowSent;

    KiSCPeer            peer;
    rcvdMsgCallback     rcvdMsg = nullptr;
    KiSCPeer::State     state = KiSCPeer::Unknown;
    String              name;
    KiSCPeer::Role      role;
};

class KiSCProtoV2Master : public KiSCProtoV2 {
 public:
    explicit KiSCProtoV2Master(String name) : KiSCProtoV2(name, KiSCPeer::Master) {}
 private:
    std::vector<KiSCPeer>  slaves;
};

class KiSCProtoV2Slave : public KiSCProtoV2 {
 public:
    explicit                KiSCProtoV2Slave(String name) : KiSCProtoV2(name, KiSCPeer::Slave) {}
    void                    setType(KiSCPeer::SlaveType type) { this->type = type; }
 protected:
    static void             dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
 private:
    KiSCPeer::SlaveType     type = KiSCPeer::Unidentified;
    bool                    masterFound = false;
    KiSCPeer                master;
};

#endif  /* INCLUDE_KISCPROTOV2_INCLUDED */
