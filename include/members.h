#ifndef INCLUDE_MEMBERS_INCLUDED
#define INCLUDE_MEMBERS_INCLUDED

// 98:F4:AB:DA:C6:05        Motorcontroller (ESP8266)
// 48:27:E2:51:D6:C0        Motorcontroller2 (ESP32S2)
// C8:C9:A3:FC:97:54        Maincontroller (ESP32)
// 94:3C:C6:37:AF:44        Sound and Light Controller (ESP32)
// 94:3C:C6:37:B3:20        Peripheral Controller (ESP32)
// 94:3C:C6:38:9F:8C        BT Remote (ESP32)
static uint8_t master[] = { 0xC8, 0xC9, 0xA3, 0xFC, 0x97, 0x54 };
static uint8_t motor[] = { 0x98, 0xF4, 0xAB, 0xDA, 0xC6, 0x05 };
static uint8_t sound_light[] = { 0x94, 0x3C, 0xC6, 0x37, 0xAF, 0x44 };
static uint8_t peripheral[] = { 0x94, 0x3C, 0xC6, 0x37, 0xB3, 0x20 };
static uint8_t remotecontrol[] = { 0x94, 0x3C, 0xC6, 0x37, 0xB3, 0x20 };
static uint8_t motor2[] = { 0x48, 0x27, 0xE2, 0x51, 0xD6, 0xC0 };
static uint8_t btremote[] = { 0x94, 0x3C, 0xC6, 0x38, 0x9F, 0x8C };

#define MOTOR_CONTROLLER_MAC        motor2
#define MAIN_CONTROLLER_MAC         master
#define SOUND_LIGHT_CONTROLLER_MAC  sound_light
#define PERIPHERAL_CONTROLLER_MAC   peripheral
#define REMOTE_CONTROLLER_MAC       remotecontrol
#define BT_CONTROLLER_MAC       btremote

#endif  /* INCLUDE_MEMBERS_INCLUDED */
