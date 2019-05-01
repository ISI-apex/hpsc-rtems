#include <assert.h>

// plat
#include <mailbox-map.h>

// drivers
#include <hpsc-mbox.h>

// drivers-selftest
#include <hpsc-mbox-test.h>

// libhpsc
#include <devices.h>

#include "test.h"

#define WTIMEOUT_TICKS 10
#define RTIMEOUT_TICKS 10

int test_mbox_lsio_loopback()
{
    enum hpsc_mbox_test_rc rc;
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    assert(mbox_lsio);
    test_begin("test_mbox_lsio_loopback");
    rc = hpsc_mbox_chan_test(mbox_lsio, MBOX_LSIO__RTPS_LOOPBACK,
                             /* owner */ 0, /* src */ 0, /* dest */ 0);
    test_end("test_mbox_lsio_loopback", rc);
    return rc;
}
