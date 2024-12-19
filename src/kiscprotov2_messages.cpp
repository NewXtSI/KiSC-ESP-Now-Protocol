#include <Arduino.h>
#include "../include/kiscprotov2.h"

#define USE_LOGGER  0
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message_Info
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message_Info::KiSCProtoV2Message_Info(uint8_t address[]) : KiSCProtoV2Message(address) {
    DBGLOG(ESP32LogLevel::Debug, "KiSCProtoV2Message_Info()");
}

void KiSCProtoV2Message_Info::buildBufferedMessage() {
    DBGLOG(ESP32LogLevel::Debug, "KiSCProtoV2Message_Info.buildBufferedMessage()");
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
    snprintf(line, sizeof(line), "Payload: %d", msg.payload_len);
    for (int i=0; i<msg.payload_len; i++) {
        snprintf(line + strlen(line), sizeof(line) - strlen(line), " %02X", msg.payload[i]);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// KiSCProtoV2Message_BTAudio
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KiSCProtoV2Message_BTAudio::KiSCProtoV2Message_BTAudio(uint8_t address[]) : KiSCProtoV2Message(address) {
    DBGLOG(ESP32LogLevel::Debug, "KiSCProtoV2Message_BTAudio");
}

bool        
KiSCProtoV2Message_BTAudio::buildFromBuffer() {
    DBGLOG(ESP32LogLevel::Debug, "try to build");
    KiSCProtoV2Message::buildFromBuffer();
    if (msg.payload_len < 2) {
        return false;
    }
    if ((msg.payload[1] != MSGTYPE_BT_AUDIO) && (msg.payload[1] != MSGTYPE_BT_AUDIO_INFO) && (msg.payload[1] != MSGTYPE_BT_AUDIO_CTRL)) {
        return false;
    }
    subCommand = msg.payload[2];
    return true;
}

void                
KiSCProtoV2Message_BTAudio::buildBufferedMessage() {
    DBGLOG(Debug, "KiSCProtoV2Message_BTAudio.buildBufferedMessage()");
    KiSCProtoV2Message::buildBufferedMessage();
    memcpy(msg.dstAddress, target.getAddress(), sizeof(msg.dstAddress));
    msg.payload[0] = PROTO_VERSION;
    msg.payload[1] = MSGTYPE_BT_AUDIO;
    msg.payload[2] = subCommand;
    msg.payload[3] = btConnected;
    // Add title, artist, album with maximum length of 20 characters each
    for (int i = 0; i < 20; i++) {
        msg.payload[4+i] = title[i];
    }
    for (int i = 0; i < 20; i++) {
        msg.payload[24+i] = artist[i];
    }
    for (int i = 0; i < 20; i++) {
        msg.payload[44+i] = album[i];
    }
    for (int i = 0; i < 20; i++) {
        msg.payload[64+i] = genre[i];
    }
    msg.payload_len = 84;
}

void
KiSCProtoV2Message_BTAudio::dump() {
    switch (subCommand) {
        case MSGTYPE_BT_AUDIO_INFO:
            DBGLOG(Verbose, "BT Audio Info");
            break;
        case MSGTYPE_BT_AUDIO_CTRL:
            DBGLOG(Verbose, "BT Audio Control");
            break;
        default:
            DBGLOG(Verbose, "Unknown BT Audio message");
            break;
    }
}
