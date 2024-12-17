#ifndef INCLUDE_KISCPROTOV2_INCLUDED
#define INCLUDE_KISCPROTOV2_INCLUDED

#include <Arduino.h>
#if defined ESP8266
#error "ESP8266 not supported at the moment"
#endif

#include <vector>

typedef struct {
    uint8_t dstAddress[6]; /**< Message topic*/
    uint8_t srcAddress[6]; /**< Message topic*/
    uint8_t payload[200]; /**< Message payload*/
    size_t payload_len; /**< Payload length*/
} espnowmsg_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class KiSCAddress {
 public:
    KiSCAddress() : address() {}
    KiSCAddress(uint8_t* address) : address() { memcpy(this->address, address, 6); }
    KiSCAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) : address() {
        this->address[0] = a;
        this->address[1] = b;
        this->address[2] = c;
        this->address[3] = d;
        this->address[4] = e;
        this->address[5] = f;
    }
    KiSCAddress(const KiSCAddress& other) : address() { memcpy(this->address, other.address, 6); }
    uint8_t* getAddress() { return address; }
    uint8_t address[6];
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class KiSCProtoV2Message {
 public:
    KiSCProtoV2Message(uint8_t address[]);
    espnowmsg_t*    getBufferedMessage() { buildBufferedMessage(); return &msg; }
    void            setBufferedMessage(espnowmsg_t* msg) { memcpy(&this->msg, msg, sizeof(espnowmsg_t)); }
    virtual bool    buildFromBuffer();
    virtual void    buildBufferedMessage();
    virtual void    dump();
    template<typename T>
        bool isA() {
            return (dynamic_cast<T*>(this) != NULL);
        }    
    void            setSource(KiSCAddress source) { this->source = source; }
    void            setTarget(KiSCAddress target) { this->target = target; }
 protected:        
    espnowmsg_t     msg;
 private:
   
    KiSCAddress     target;
    KiSCAddress     source;

    friend class KiSCProtoV2Message_Info;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

    static void         messageReceived(KiSCProtoV2Message* msg, signed int rssi, bool broadcast);
    static void         task(void* param);
    void                send(KiSCProtoV2Message *msg);
    KiSCAddress         getAddress() { return address; }
    static KiSCProtoV2Message* buildProtoMessage(espnowmsg_t msg);
    virtual void                taskTick100ms();
    virtual void                taskTick500ms();
    virtual void                taskTick1s();
 private:
    static QueueHandle_t  sendQueue;

    static void         _sendViaESPNow(espnowmsg_t msg);

    static bool         ESPNowSent;

    const uint8_t       queueSize = 10;
    KiSCPeer            peer;
    rcvdMsgCallback     rcvdMsg = nullptr;
    KiSCPeer::State     state = KiSCPeer::Unknown;
    String              name;
    KiSCPeer::Role      role;
    KiSCAddress         address;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class KiSCProtoV2Master : public KiSCProtoV2 {
 public:
    explicit                KiSCProtoV2Master(String name);
    void                    taskTick500ms(); 
 private:
    bool                    broadcastActive;
    void                    sendBroadcastOffer();
    std::vector<KiSCPeer>   slaves;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class KiSCProtoV2Slave : public KiSCProtoV2 {
 public:
    explicit                KiSCProtoV2Slave(String name);
    void                    setType(KiSCPeer::SlaveType type) { this->type = type; }
    void                    taskTick500ms(); 
 protected:
    static void             dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
 private:
    KiSCPeer::SlaveType     type = KiSCPeer::Unidentified;
    bool                    masterFound = false;
    KiSCPeer                master;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class KiSCProtoV2Message_Info : public KiSCProtoV2Message {
 public:
    KiSCProtoV2Message_Info(uint8_t address[]);

    virtual bool    buildFromBuffer();
    void    buildBufferedMessage() override;
    virtual void    dump();
    void           setName(String name) { this->name = name; }
 private:
    String              name;
    KiSCPeer::Role      role;
    KiSCPeer::State     state;  
};

#endif  /* INCLUDE_KISCPROTOV2_INCLUDED */
