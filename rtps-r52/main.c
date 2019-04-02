#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>

// plat
#include <hwinfo.h>
#include <mailbox-map.h>
#include <mem-map.h>

// libhpsc
#include <command.h>
#include <mailbox-link.h>
#include <server.h>

// mailbox driver
#include <hpsc-mbox.h>

#include "devices.h"
#include "gic.h"
#include "test.h"
#include "watchdog.h"

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

#if CONFIG_WDT
        // Kicking from here is insufficient, because we sleep. There are two
        // ways to complete:
        //     (A) have TRCH disable the watchdog in response to the WFI output
        //     signal from the core,
        //     (B) have a scheduler (with a tick interval shorter than the
        //     watchdog timeout interval) and kick from the scheuduler tick, or
        //     (C) kick on return from WFI/WFI (which could be as a result of
        //     either first stage timeout IRQ or the system timer tick IRQ).
        // At this time, we can do either (B) or (C): (B) has the disadvantage
        // that what is being monitored is the systick ISR, and not the main
        // loop proper (so if any ISRs starve the main loop, that won't be
        // detected), and (C) has the disadvantage that if the main loop
        // performs long actions, those actions need to kick. We go with (C).
        watchdog_kick();
#endif // CONFIG_WDT

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

void *POSIX_Init(void *arg)
{
    printf("\n\nRTPS\n");

#if CONFIG_MBOX_LSIO
    struct hpsc_mbox *mbox_lsio = NULL;
    rtems_status_code mbox_lsio_sc;
    rtems_vector_number mbox_lsio_vec_a =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_RCV_INT,
                       GIC_IRQ_TYPE_SPI);
    rtems_vector_number mbox_lsio_vec_b =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + MBOX_LSIO__RTPS_ACK_INT,
                       GIC_IRQ_TYPE_SPI);
    mbox_lsio_sc = hpsc_mbox_probe(&mbox_lsio, "TRCH-RTPS Mailbox",
                                   MBOX_LSIO__BASE,
                                   mbox_lsio_vec_a, MBOX_LSIO__RTPS_RCV_INT,
                                   mbox_lsio_vec_b, MBOX_LSIO__RTPS_ACK_INT);
    assert(mbox_lsio_sc == RTEMS_SUCCESSFUL);
    dev_add_mbox(DEV_ID_MBOX_LSIO, mbox_lsio);
#endif // CONFIG_MBOX_LSIO

#if CONFIG_MBOX_HPPS
    struct hpsc_mbox *mbox_hpps = NULL;
    rtems_status_code mbox_hpps_sc;
    rtems_vector_number mbox_hpps_vec_a =
        gic_irq_to_rvn(RTPS_IRQ__HR_MBOX_0 + MBOX_HPPS_RTPS__RTPS_RCV_INT,
                       GIC_IRQ_TYPE_SPI);
    rtems_vector_number mbox_hpps_vec_b =
        gic_irq_to_rvn(RTPS_IRQ__HR_MBOX_0 + MBOX_HPPS_RTPS__RTPS_ACK_INT,
                       GIC_IRQ_TYPE_SPI);
    mbox_hpps_sc = hpsc_mbox_probe(&mbox_hpps, "HPPS-RTPS Mailbox",
                                   MBOX_HPPS_RTPS__BASE,
                                   mbox_hpps_vec_a, MBOX_HPPS_RTPS__RTPS_RCV_INT,
                                   mbox_hpps_vec_b, MBOX_HPPS_RTPS__RTPS_ACK_INT);
    assert(mbox_hpps_sc == RTEMS_SUCCESSFUL);
    dev_add_mbox(DEV_ID_MBOX_HPPS, mbox_hpps);
#endif // CONFIG_MBOX_HPPS

#if CONFIG_HPPS_RTPS_MAILBOX
    struct link *hpps_link = mbox_link_connect("HPPS_MBOX_LINK", mbox_hpps,
                    MBOX_HPPS_RTPS__HPPS_RTPS, MBOX_HPPS_RTPS__RTPS_HPPS,
                    /* server */ MASTER_ID_RTPS_CPU0,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        rtems_panic("HPPS link");
    // Never release the link, because we listen on it in main loop
#endif // CONFIG_HPPS_RTPS_MAILBOX

#if TEST_RTPS_TRCH_MAILBOX
    if (test_rtps_trch_mailbox())
        rtems_panic("RTPS->TRCH mailbox");
#endif // TEST_RTPS_TRCH_MAILBOX

#if TEST_SHMEM
    if (test_shmem())
        rtems_panic("shmem test");
#endif // TEST_SHMEM

#if CONFIG_WDT
    watchdog_init();
#endif // CONFIG_WDT

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
