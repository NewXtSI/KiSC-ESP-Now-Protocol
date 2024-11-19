#ifndef INCLUDE_KISC_ESPNOW_INCLUDED
#define INCLUDE_KISC_ESPNOW_INCLUDED

#include "./protocol.h"
#include "./members.h"

// define a callback funtion, when a KiSCMessage is received
typedef void (*KiSCMessageReceivedCallback)(kisc::protocol::espnow::KiSCMessage message);

bool initESPNow();
void loopESPNow();
void onKiSCMessageReceived(KiSCMessageReceivedCallback callback);
void sendKiSCMessage(uint8_t *targetAddress, kisc::protocol::espnow::KiSCMessage message);

#endif  /* INCLUDE_KISC_ESPNOW_INCLUDED */
