#ifndef INCLUDE_PROTOCOL_INCLUDED
#define INCLUDE_PROTOCOL_INCLUDED
#include <stdint.h>
#define MAX_MESSAGE_SIZE 200

#include "commands.h"


// Control immer in Richtung der SubController, Feedback immer in Richtung  
namespace kisc {
namespace protocol {
namespace espnow {

typedef struct {
} KiSCEmptyMessage;

typedef struct {
    char value[32];
} KiSCStringMessage;

typedef struct {
    uint8_t     enable;

    uint8_t     type;
    uint8_t     mode;

    int16_t     pwm;
    uint8_t     iMotMax;
    uint16_t    nMotMax;
    uint8_t     fieldWeakMax;
    bool        cruiseCtrlEna;
    int16_t     cruiseMotTgt;

    uint8_t     standStillOnHold;
    uint8_t     electricBrakeFactor; // Rekupationsfaktor 0: keine Rekuperation, 100: maximale Rekuperation
} KiSCMotorControl_Motorsettings;

typedef struct {
    uint8_t     poweroff;
    KiSCMotorControl_Motorsettings left;
    KiSCMotorControl_Motorsettings right;

} KiSCMotorControlMessage;

typedef struct {
    int16_t   target;
    
    int16_t   speed;
    uint8_t   error;
} KiSCMotorFeedback_Motorsettings;

typedef struct {
    int16_t   batVoltage;
    int16_t   boardTemp;
    bool      bCharging;
    bool      motorboardConnected;
    KiSCMotorFeedback_Motorsettings left;
    KiSCMotorFeedback_Motorsettings right;
} KiSCMotorFeedbackMessage;

typedef struct {
    int16_t     steering;                   // -1023 - 1023     // 0 center, -1023 left, 1023 right
    bool        steeringActive;
    bool        parkingBrakeActive;
} KiSCPeripheralControlMessage;

typedef struct {
    uint8_t     leds;
    uint16_t     rumbledelay;
    uint16_t    rumbleduration;
    uint16_t    rumbleweakMagnitude;
    uint16_t    rumblestrongMagnitude;
} KiSCRemoteControlMessage;

   // DPAD hoch: Gang hoch
    // DPAD runter: Gang runter
    // Linker Stick: Lenkung
    // Rechter Stick: Gas/Bremse
    // Y: Motor an/aus
    // o: Cruise control

    // L: Blinker links
    // R: Blinker rechts
    
typedef struct {
    uint16_t    throttle;                   // 0 - 1023
    uint16_t    brake;                      // 0 - 1023
    int16_t     steering;                   // -1023 - 1023     // 0 center, -1023 left, 1023 right

    bool        motorButton;
    bool        lightButton;
    bool        hornButton;
    bool        gearUpButton;
    bool        gearDownButton;
    bool        indicatorLeftButton;
    bool        indicatorRightButton;
    bool        sirenButton;
    bool        cruiseControlButton;

    bool        controllerConnected;
    uint8_t     batteryLevel;
} KiSCRemoteFeedbackMessage;

typedef struct {
    uint16_t    throttle;                   // 0 - 1023
    uint16_t    brake;                      // 0 - 1023
    int16_t     steering;                   // -1023 - 1023     // 0 center, -1023 left, 1023 right
    
    bool        motorButton;
    bool        lightButton;
    bool        hornButton;
    bool        gearUpButton;
    bool        gearDownButton;
    bool        indicatorLeftButton;
    bool        indicatorRightButton;
    bool        sirenButton;
    bool        cruiseControlButton;

    int16_t     ypr[3];
    int16_t     acc[3];
    bool        nfcCardActive;
    char        nfcCardUID[20];
    
    bool        servoPowerActive;

    uint16_t    ambientLight;
    uint32_t    subsystemStatus;
} KiSCPeripheralFeedbackMessage;

typedef struct {
    uint8_t    parkingLight:1;
    uint8_t    lowBeam:1;
    uint8_t    highBeam:1;
    uint8_t    brakeLight:1;
    uint8_t    reverseLight:1;
    uint8_t    indicatorLeft:1;
    uint8_t    indicatorRight:1;
    uint8_t    horn:1;
    uint8_t    siren:1;
    uint8_t    alarm:1;
} KiSCSoundAndLight_Lightsettings;

typedef struct {
    float                               volumeMain;  // 1.0 = 100%
    uint8_t                             volumeMotor;
    uint8_t                             volumeSpeech;
    uint8_t                             volumeMusic;

    bool                                motorOn;
    uint16_t                            throttle;
    uint16_t                            brake;

    uint16_t                            rpm;             // Reale RPM von den Motoren
    uint8_t                             gearSelection;
    
    KiSCSoundAndLight_Lightsettings     lights;
} KiSCSoundAndLightControlMessage;

typedef struct {
    bool        motorOn;
    uint16_t    throttle;
    uint16_t    rpm;                // Simulierte RPM, wenn Neutral, sonst reale
    uint8_t     selectedGear;
} KiSCSoundAndLightFeedbackMessage;

typedef struct {
    Command command;
    union {
        KiSCEmptyMessage                    empty;
        KiSCMotorControlMessage             motorControl;
        KiSCMotorFeedbackMessage            motorFeedback;
        KiSCPeripheralControlMessage        peripheralControl;
        KiSCPeripheralFeedbackMessage       peripheralFeedback;
        KiSCSoundAndLightControlMessage     soundAndLightControl;
        KiSCSoundAndLightFeedbackMessage    soundAndLightFeedback;
        KiSCRemoteControlMessage            remoteControl;
        KiSCRemoteFeedbackMessage           remoteFeedback;
//        KiSCStringMessage string;
        uint8_t          raw[MAX_MESSAGE_SIZE];
    };
} KiSCMessage;

typedef struct {
    uint8_t address[6];
    uint8_t command;
    uint8_t data[MAX_MESSAGE_SIZE];
} KiSCWireMessage;

}  // namespace espnow
}  // namespace protocol
}  // namespace kisc

#endif  /* INCLUDE_PROTOCOL_INCLUDED */
