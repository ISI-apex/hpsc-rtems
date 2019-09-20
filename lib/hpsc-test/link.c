#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include <rtems.h>

// libhpsc
#include <hpsc-msg.h>
#include <link.h>

#include "hpsc-test.h"

int hpsc_test_link_ping(struct link *link, rtems_interval wtimeout_ticks,
                        rtems_interval rtimeout_ticks,
                        rtems_event_set event_wait)
{
    HPSC_MSG_DEFINE(arg);
    HPSC_MSG_DEFINE(reply);
    uint32_t payload = 42;
    ssize_t sz;
    assert(link);

    hpsc_msg_ping(arg, sizeof(arg), &payload, sizeof(payload));
    sz = link_request(link,
                      wtimeout_ticks, arg, sizeof(arg),
                      rtimeout_ticks, reply, sizeof(reply),
                      event_wait);
    return sz <= 0 || reply[0] != PONG;
}
