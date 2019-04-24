#include <assert.h>
#include <stdint.h>
#include <unistd.h>

// libhpsc
#include <hpsc-msg.h>
#include <link.h>
#include <link-store.h>

#include "link-names.h"
#include "test.h"

#define WTIMEOUT_TICKS 100
#define RTIMEOUT_TICKS 100

int test_link_shmem_trch()
{
    HPSC_MSG_DEFINE(arg);
    HPSC_MSG_DEFINE(reply);
    uint32_t payload = 42;
    ssize_t rc;
    struct link *trch_link = link_store_get(LINK_NAME__SHMEM__TRCH_CLIENT);
    assert(trch_link);

    test_begin("test_link_shmem_trch");
    hpsc_msg_ping(arg, sizeof(arg), &payload, sizeof(payload));
    rc = link_request(trch_link,
                      WTIMEOUT_TICKS, arg, sizeof(arg),
                      RTIMEOUT_TICKS, reply, sizeof(reply));
    rc = rc <= 0 ? -1 : 0;
    test_end("test_link_shmem_trch", rc);
    return (int) rc;
}
