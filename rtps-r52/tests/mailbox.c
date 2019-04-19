#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// plat
#include <hwinfo.h>
#include <mailbox-map.h>

// drivers-selftest
#include <hpsc-mbox-test.h>

// libhpsc
#include <hpsc-msg.h>
#include <mailbox-link.h>

#include "devices.h"

#define WTIMEOUT_TICKS 10
#define RTIMEOUT_TICKS 10

int test_mbox_lsio_loopback()
{
    enum hpsc_mbox_test_rc rc;
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    assert(mbox_lsio);
    rc = hpsc_mbox_chan_test(mbox_lsio, MBOX_LSIO__RTPS_LOOPBACK,
                             /* owner */ 0, /* src */ 0, /* dest */ 0);
    printf("TEST: mbox_lsio_loopback: %s\n", rc ? "failed" : "success");
    return rc;
}

int test_mbox_lsio_trch()
{
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    struct link *trch_link;
    HPSC_MSG_DEFINE(arg);
    HPSC_MSG_DEFINE(reply);
    uint32_t payload = 42;
    ssize_t rc;

    hpsc_msg_ping(arg, sizeof(arg), &payload, sizeof(payload));

    assert(mbox_lsio);
    trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK", mbox_lsio,
                                  MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                                  /* server */ 0,
                                  /* client */ MASTER_ID_RTPS_CPU0);
    if (!trch_link)
        return -1;

    rc = link_request(trch_link,
                      WTIMEOUT_TICKS, arg, sizeof(arg),
                      RTIMEOUT_TICKS, reply, sizeof(reply));
    rc = rc <= 0 ? -1 : 0;

    return link_disconnect(trch_link) ? -1 : (int) rc;
}
