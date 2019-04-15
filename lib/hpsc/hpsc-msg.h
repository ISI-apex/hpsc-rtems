#ifndef HPSC_MSG_H
#define HPSC_MSG_H

#include <stdint.h>

#define HPSC_MSG_SIZE 64
#define HPSC_MSG_PAYLOAD_OFFSET 4
#define HPSC_MSG_PAYLOAD_SIZE (HPSC_MSG_SIZE - 4)

#define HPSC_MSG_DEFINE(name) uint8_t name[HPSC_MSG_SIZE] = { 0 }

// Message type enumeration
enum hpsc_msg_type {
    // Value 0 is reserved so empty messages can be recognized
    NOP = 0,
    // test messages
    PING,
    PONG,
    // responses - payload contains ID of the response being acknowledged
    READ_VALUE,
    WRITE_STATUS,
    // general operations
    READ_FILE,
    WRITE_FILE,
    READ_PROP,
    WRITE_PROP,
    READ_ADDR,
    WRITE_ADDR,
    // notifications
    WATCHDOG_TIMEOUT,
    FAULT,
    LIFECYCLE,
    // an enumerated/predefined action
    ACTION,
    // enum counter
    HPSC_MSG_TYPE_COUNT
};

enum hpsc_msg_lifecycle_status {
    LIFECYCLE_UP,
    LIFECYCLE_DOWN
};

void hpsc_msg_wdt_timeout(void *buf, size_t sz, unsigned int cpu);

void hpsc_msg_lifecycle(void *buf, size_t sz,
                        enum hpsc_msg_lifecycle_status status,
                        const char *fmt, ...);

void hpsc_msg_ping(void *buf, size_t sz, void *payload, size_t psz);

void hpsc_msg_pong(void *buf, size_t sz, void *payload, size_t psz);

#endif // HPSC_MSG_H
