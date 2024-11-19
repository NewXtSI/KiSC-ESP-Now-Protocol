#ifndef INCLUDE_MEMBERS_INCLUDED
#define INCLUDE_MEMBERS_INCLUDED

// 98:F4:AB:DA:C6:05        Motorcontroller (ESP8266)
// 98:F4:AB:DA:C6:05        Maincontroller (ESP32)

static uint8_t master[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 };

#define MOTOR_CONTROLLER_MAC    {0x98, 0xF4, 0xAB, 0xDA, 0xC6, 0x05}
#define MAIN_CONTROLLER_MAC     master


#endif  /* INCLUDE_MEMBERS_INCLUDED */
