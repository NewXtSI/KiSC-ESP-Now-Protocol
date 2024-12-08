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
    BTControl = 40,
    BTFeedback = 41,
    BTTitle = 42,
    BTArtist = 43,
    RemoteControl = 50,
    RemoteFeedback = 51,
} Command;

}  // namespace espnow
}  // namespace protocol
}  // namespace kisc

#endif  /* INCLUDE_COMMANDS_INCLUDED */
