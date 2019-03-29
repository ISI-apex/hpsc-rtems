#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>

#include "../plat/hwinfo.h"
#include "../plat/mailbox-map.h"
#include "../plat/mem-map.h"

#include "../hpsc/command.h"
#include "../hpsc/mailbox-link.h"
#include "../hpsc/server.h"

#include "../mailbox/hpsc-mbox.h"

#include "devices.h"

#define MAIN_LOOP_SILENT_ITERS 100

void main_loop()
{
    struct cmd cmd;
    unsigned iter = 0;
    bool verbose;
    rtems_interrupt_level level;

    while (1) {
        verbose = iter++ % MAIN_LOOP_SILENT_ITERS == 0;
        if (verbose)
            printf("RTPS: main loop\n");

        while (!cmd_dequeue(&cmd)) {
            cmd_handle(&cmd);
            verbose = true; // to end log with 'waiting' msg
        }

        rtems_interrupt_local_disable(level); // the check and the WFI must be atomic
        if (!cmd_pending()) {
            if (verbose)
                printf("[%u] Waiting for interrupt...\n", iter);
            asm("wfi"); // ignores PRIMASK set by int_disable
        }
        rtems_interrupt_local_enable(level);
    }
}


static int test_rtps_trch_mailbox()
{
    rtems_status_code sc;
    struct hpsc_mbox mbox_trch;
    struct link *trch_link;
    uint32_t arg[] = { CMD_PING, 42 };
    uint32_t reply[sizeof(arg) / sizeof(arg[0])] = {0};
    int rc;

    sc = hpsc_mbox_probe(&mbox_trch, "TRCH-RTPS Mailbox", MBOX_LSIO__BASE,
                         36, MBOX_LSIO__RTPS_RCV_INT,
                         37, MBOX_LSIO__RTPS_ACK_INT);
    assert(sc == RTEMS_SUCCESSFUL);

    trch_link = mbox_link_connect("RTPS_TRCH_MBOX_TEST_LINK",
                    &mbox_trch, MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
                    /* server */ 0, /* client */ MASTER_ID_RTPS_CPU0);
    if (!trch_link)
        rtems_panic("TRCH link");

    printf("arg len: %u\n", sizeof(arg) / sizeof(arg[0]));
    rc = trch_link->request(trch_link,
                            CMD_TIMEOUT_MS_SEND, arg, sizeof(arg),
                            CMD_TIMEOUT_MS_RECV, reply, sizeof(reply));
    if (rc <= 0)
        rtems_panic("TRCH link request");

    rc = trch_link->disconnect(trch_link);
    if (rc)
        rtems_panic("TRCH link disconnect");

    sc = hpsc_mbox_remove(&mbox_trch);
    if (!trch_link)
        rtems_panic("TRCH link rm");

    return 0;
}

void *POSIX_Init(void *arg)
{
    rtems_status_code sc;
    printf("\n\nRTPS\n");

    struct hpsc_mbox mbox_hpps;
    sc = hpsc_mbox_probe(&mbox_hpps, "HPPS-RTPS Mailbox", MBOX_HPPS_RTPS__BASE,
                         RTPS_IRQ__HR_MBOX_0, MBOX_HPPS_RTPS__RTPS_RCV_INT,
                         RTPS_IRQ__HR_MBOX_1, MBOX_HPPS_RTPS__RTPS_ACK_INT);
    assert(sc == RTEMS_SUCCESSFUL);

    struct link *hpps_link = mbox_link_connect("HPPS_MBOX_LINK", &mbox_hpps,
                    MBOX_HPPS_RTPS__HPPS_RTPS, MBOX_HPPS_RTPS__RTPS_HPPS,
                    /* server */ MASTER_ID_RTPS_CPU0,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        rtems_panic("HPPS link");
    // Never release the link, because we listen on it in main loop

    test_rtps_trch_mailbox();
    printf("TRCH mbox test succeeded\n");

    cmd_handler_register(server_process);

    main_loop();

    return NULL;
}

/* configuration information */

#include <bsp.h>

/* NOTICE: the clock driver is explicitly disabled */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_MAXIMUM_POSIX_THREADS 1

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
