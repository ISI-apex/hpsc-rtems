#include <stdint.h>
#include <stdio.h>

#include <rtems.h>

#include "command.h"
#include "gic.h"
#include "hwinfo.h"
#include "mailbox-link.h"
#include "mailbox-map.h"

int test_rtps_trch_mailbox()
{
    rtems_status_code sc;
    struct hpsc_mbox *mbox_trch;
    struct link *trch_link;
    rtems_vector_number vec_a =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_RCV_INT,
                       GIC_IRQ_TYPE_SPI);
    rtems_vector_number vec_b =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_ACK_INT,
                       GIC_IRQ_TYPE_SPI);
    uint32_t arg[] = { CMD_PING, 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    int rc;

    sc = hpsc_mbox_probe(&mbox_trch, "TRCH-RTPS Mailbox", MBOX_LSIO__BASE,
                         vec_a, MBOX_LSIO__RTPS_RCV_INT,
                         vec_b, MBOX_LSIO__RTPS_ACK_INT);
    if (sc != RTEMS_SUCCESSFUL)
        return -1;

    trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK",
                    mbox_trch, MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                    /* server */ 0, /* client */ MASTER_ID_RTPS_CPU0);
    if (!trch_link)
        rtems_panic("TRCH link");

    printf("arg len: %u\n", sizeof(arg) / sizeof(arg[0]));
    rc = trch_link->request(trch_link,
                            CMD_TIMEOUT_MS_SEND, arg, sizeof(arg),
                            CMD_TIMEOUT_MS_RECV, reply, sizeof(reply));
    if (rc <= 0)
        return -1;

    rc = trch_link->disconnect(trch_link);
    if (rc)
        return rc;

    sc = hpsc_mbox_remove(mbox_trch);
    if (sc != RTEMS_SUCCESSFUL)
        return -1;

    return 0;
}
