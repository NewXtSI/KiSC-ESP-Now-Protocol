#ifndef INCLUDE_PROTOCOL_INCLUDED
#define INCLUDE_PROTOCOL_INCLUDED
#include <stdint.h>
#define MAX_MESSAGE_SIZE 200

#include "commands.h"

namespace kisc {
namespace protocol {
namespace espnow {

typedef struct {
} KiSCEmptyMessage;

typedef struct {
    Command command;
    union {
        KiSCEmptyMessage empty;
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
