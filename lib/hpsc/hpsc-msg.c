#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hpsc-msg.h"

// info is for debugging, use real data types if we need more detail
#define LIFECYCLE_INFO_SIZE (HPSC_MSG_PAYLOAD_SIZE - sizeof(uint32_t))
struct hpsc_msg_lifeycle_payload {
    uint32_t status;
    char info[LIFECYCLE_INFO_SIZE];
};

static void msg_fill(uint8_t *buf, enum hpsc_msg_type t, const void *payload,
                     size_t psz)
{
    assert(buf);
    assert(psz <= HPSC_MSG_PAYLOAD_SIZE);
    buf[0] = t;
    if (payload)
        memcpy(&buf[HPSC_MSG_PAYLOAD_OFFSET], payload, psz);
}

void hpsc_msg_wdt_timeout(void *buf, size_t sz, unsigned int cpu)
{
    assert(sz == HPSC_MSG_SIZE);
    msg_fill(buf, WATCHDOG_TIMEOUT, &cpu, sizeof(cpu));
}

void hpsc_msg_lifecycle(void *buf, size_t sz,
                        enum hpsc_msg_lifecycle_status status,
                        const char *fmt, ...)
{
    // payload is the status enumeration and a string of debug data
    va_list args;
    struct hpsc_msg_lifeycle_payload p = {
        .status = status,
        .info = {0}
    };
    assert(sz == HPSC_MSG_SIZE);
    if (fmt) {
        va_start(args, fmt);
        vsnprintf(p.info, LIFECYCLE_INFO_SIZE - 1, fmt, args);
        va_end(args);
    }
    msg_fill(buf, LIFECYCLE, &p, sizeof(p));
}

void hpsc_msg_ping(void *buf, size_t sz, void *payload, size_t psz)
{
    assert(sz == HPSC_MSG_SIZE);
    msg_fill(buf, PING, payload, psz);
}

void hpsc_msg_pong(void *buf, size_t sz, void *payload, size_t psz)
{
    assert(sz == HPSC_MSG_SIZE);
    msg_fill(buf, PONG, payload, psz);
}
