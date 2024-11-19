#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA 
#else
#error "Unsupported platform"
#endif //ESP32
#include <QuickEspNow.h>
#include "../include/kisc-espnow.h"


KiSCMessageReceivedCallback KiSCReceived_callback = NULL;

bool ESPNowSent = true;

void onKiSCMessageReceived(KiSCMessageReceivedCallback callback) {
    KiSCReceived_callback = callback;
}

void dataSent (uint8_t* address, uint8_t status) {
    ESPNowSent = true;
    Serial.printf ("Message sent to " MACSTR ", status: %d\n", MAC2STR (address), status);
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
    Serial.printf ("%s\n", broadcast ? "Broadcast" : "Unicast");
}

bool initESPNow() {
    WiFi.mode (WIFI_MODE_STA);
#if defined ESP32
    WiFi.disconnect (false, true);
#elif defined ESP8266
    WiFi.disconnect (false);
#endif //ESP32
#ifdef ESP32
    quickEspNow.setWiFiBandwidth (WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif //ESP32

    quickEspNow.begin (1, 0, false);
    quickEspNow.onDataSent (dataSent);
    quickEspNow.onDataRcvd (dataReceived);


    return true;
}

void loopESPNow() {
    return;
}

uint8_t getMessageLength(uint8_t command) {
    switch (command) {
        case kisc::protocol::espnow::Command::Ping:
            return sizeof(kisc::protocol::espnow::KiSCEmptyMessage);
        case kisc::protocol::espnow::Command::MotorControl:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::MotorFeedback:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::PeriphalControl:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::PeriphalFeedback:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::SoundLightControl:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::SoundLightFeedback:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        default:
            return 0;
    }
}
void sendKiSCWireMessage(kisc::protocol::espnow::KiSCWireMessage message) {
    if (ESPNowSent) {
        ESPNowSent = false;
        uint8_t buffer[sizeof(message)];
        buffer[0] = message.command;
        memcpy(&buffer[1], &message, sizeof(message));
        uint8_t msgLen;
        msgLen = getMessageLength(message.command)+1;
        quickEspNow.send(message.address, message.data, msgLen);
    }
}

void sendKiSCMessage(uint8_t *targetAddress, kisc::protocol::espnow::KiSCMessage message) {

        kisc::protocol::espnow::KiSCWireMessage wireMessage;
        memcpy(wireMessage.address, targetAddress, sizeof(wireMessage.address));
        wireMessage.command = message.command;
        memcpy(wireMessage.data,message.raw, sizeof(wireMessage.data));
        sendKiSCWireMessage(wireMessage);
}