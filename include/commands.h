#ifndef INCLUDE_COMMANDS_INCLUDED
#define INCLUDE_COMMANDS_INCLUDED

namespace kisc {
namespace protocol {
namespace espnow {
typedef enum {
    Ping = 0,
    MotorControl = 10,
    MotorFeedback = 11,
    PeriphalControl = 20,
    PeriphalFeedback = 21,
    SoundLightControl = 30,
    SoundLightFeedback = 31,

} Command;

}  // namespace espnow
}  // namespace protocol
}  // namespace kisc

#endif  /* INCLUDE_COMMANDS_INCLUDED */
