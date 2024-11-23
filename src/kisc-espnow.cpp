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

#include <vector>

#define ESPNOW_DEBUG_SEND_RECEIVE   0

KiSCMessageReceivedCallback KiSCReceived_callback = NULL;

bool ESPNowSent = true;

void onKiSCMessageReceived(KiSCMessageReceivedCallback callback) {
    KiSCReceived_callback = callback;
}

void dataSent (uint8_t* address, uint8_t status) {
    ESPNowSent = true;
#if ESPNOW_DEBUG_SEND_RECEIVE
    Serial.printf ("Message sent to " MACSTR ", status: %d\n", MAC2STR (address), status);
#endif    
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
#if ESPNOW_DEBUG_SEND_RECEIVE    
    Serial.print ("Received: ");
    Serial.printf ("%.*s ", len, data);
    Serial.printf ("RSSI: %d dBm ", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
#endif    
#if ESPNOW_DEBUG_SEND_RECEIVE
    Serial.print("RAW: ");
    for (int i = 0; i < len; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
#endif    
    if (KiSCReceived_callback != NULL) {
        kisc::protocol::espnow::KiSCMessage message;
        message.command = kisc::protocol::espnow::Command(data[0]);
        memcpy(message.raw, &data[1], len-1);
#if ESPNOW_DEBUG_SEND_RECEIVE        
        Serial.printf("Received message with command %d --> ", message.command);
        for (int i = 0; i < len-1; i++) {
            Serial.printf("%02X ", message.raw[i]);
        }
        Serial.println();
#endif        
        KiSCReceived_callback(message);
    }
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


uint8_t getMessageLength(uint8_t command) {
    switch (command) {
        case kisc::protocol::espnow::Command::Ping:
            return sizeof(kisc::protocol::espnow::KiSCEmptyMessage);
        case kisc::protocol::espnow::Command::MotorControl:
            return sizeof(kisc::protocol::espnow::KiSCMotorControlMessage);
        case kisc::protocol::espnow::Command::MotorFeedback:
            return sizeof(kisc::protocol::espnow::KiSCMotorFeedbackMessage);
        case kisc::protocol::espnow::Command::PeriphalControl:
            return sizeof(kisc::protocol::espnow::KiSCPeripheralControlMessage);
        case kisc::protocol::espnow::Command::PeriphalFeedback:
            return sizeof(kisc::protocol::espnow::KiSCPeripheralFeedbackMessage);
        case kisc::protocol::espnow::Command::SoundLightControl:
            return sizeof(kisc::protocol::espnow::KiSCSoundAndLightControlMessage);
        case kisc::protocol::espnow::Command::SoundLightFeedback:
            return sizeof(kisc::protocol::espnow::KiSCSoundAndLightFeedbackMessage);
        case kisc::protocol::espnow::Command::BTControl:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::BTFeedback:
            return sizeof(kisc::protocol::espnow::KiSCMessage);
        case kisc::protocol::espnow::Command::BTTitle:
            return sizeof(kisc::protocol::espnow::KiSCStringMessage);
        case kisc::protocol::espnow::Command::BTArtist:
            return sizeof(kisc::protocol::espnow::KiSCStringMessage);
        default:
            return 0;
    }
}

// create a vector for messages to send via quickEspNow
// this is needed because the quickEspNow.send() function
// is blocking and we do not want to block the main loop

std::vector<kisc::protocol::espnow::KiSCWireMessage> messagesToSend;	

// do not send messages directly, push it to a vector
// and send it in the main loop
void sendKiSCWireMessage(kisc::protocol::espnow::KiSCWireMessage message) {
    if (ESPNowSent) {
        ESPNowSent = false;
        uint8_t buffer[512];
        buffer[0] = message.command;
        memcpy(&buffer[1], &message.data, sizeof(message.data));
        uint8_t msgLen;
        msgLen = getMessageLength(message.command)+1;
#if ESPNOW_DEBUG_SEND_RECEIVE
        Serial.print("OUT: ");
        for (int i = 0; i < msgLen; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();
#endif        
        quickEspNow.send(message.address, buffer, msgLen);
    }
}


void loopESPNow() {
    if (ESPNowSent && quickEspNow.readyToSendData()) {
        if (messagesToSend.size() > 0) {
            kisc::protocol::espnow::KiSCWireMessage message = messagesToSend.back();
            messagesToSend.pop_back();
            sendKiSCWireMessage(message);
        }
    }
    return;
}

void sendKiSCMessage(uint8_t *targetAddress, kisc::protocol::espnow::KiSCMessage message) {

        kisc::protocol::espnow::KiSCWireMessage wireMessage;
        memcpy(wireMessage.address, targetAddress, sizeof(wireMessage.address));
        wireMessage.command = message.command;
        if (message.command == kisc::protocol::espnow::Command::BTArtist || message.command == kisc::protocol::espnow::Command::BTTitle) {
            memcpy(wireMessage.data, message.string.value, sizeof(wireMessage.data));
        } else {
            if (message.command == kisc::protocol::espnow::Command::PeriphalFeedback) {
                memcpy(wireMessage.data, &message.peripheralFeedback, sizeof(wireMessage.data));
            } else {
                memcpy(wireMessage.data, message.raw, sizeof(wireMessage.data));
            }
        }
//        memcpy(wireMessage.data, message.raw, sizeof(wireMessage.data));
        messagesToSend.push_back(wireMessage);
     //   sendKiSCWireMessage(wireMessage
}