#include <assert.h>
#include <stdint.h>

#include <rtems.h>

// plat
#include <hwinfo.h>
#include <mailbox-map.h>

// libhpsc
#include <command.h>
#include <mailbox-link.h>

#include "devices.h"

int test_rtps_trch_mailbox()
{
    struct hpsc_mbox *mbox_trch = dev_get_mbox(DEV_ID_MBOX_LSIO);
    struct link *trch_link;
    uint32_t arg[] = { CMD_PING, 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    int rc;

    assert(mbox_trch);
    trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK", mbox_trch,
                                  MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                                  /* server */ 0,
                                  /* client */ MASTER_ID_RTPS_CPU0);
    if (!trch_link)
        return -1;

    rc = trch_link->request(trch_link,
                            CMD_TIMEOUT_MS_SEND, arg, sizeof(arg),
                            CMD_TIMEOUT_MS_RECV, reply, sizeof(reply));
    rc = rc <= 0 ? -1 : 0;

    return trch_link->disconnect(trch_link) ? -1 : rc;
}
