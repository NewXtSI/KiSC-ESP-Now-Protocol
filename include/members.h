#ifndef INCLUDE_MEMBERS_INCLUDED
#define INCLUDE_MEMBERS_INCLUDED

// 98:F4:AB:DA:C6:05        Motorcontroller (ESP8266)
// C8:C9:A3:FC:97:54        Maincontroller (ESP32)

static uint8_t master[] = { 0xC8, 0xC9, 0xA3, 0xFC, 0x97, 0x54 };
static uint8_t motor[] = { 0x98, 0xF4, 0xAB, 0xDA, 0xC6, 0x05 };

#define MOTOR_CONTROLLER_MAC    motor
#define MAIN_CONTROLLER_MAC     master


#endif  /* INCLUDE_MEMBERS_INCLUDED */
