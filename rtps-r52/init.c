#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/shell.h>

// plat
#include <hwinfo.h>
#include <mailbox-map.h>
#include <mem-map.h>

// libhpsc
#include <affinity.h>
#include <command.h>
#include <mailbox-link.h>

// drivers
#include <hpsc-mbox.h>

#include "devices.h"
#include "gic.h"
#include "shell.h"
#include "server.h"
#include "shutdown.h"
#include "test.h"
#include "watchdog.h"

static rtems_status_code init_extra_drivers(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void *arg
)
{
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

#if CONFIG_RTI_TIMER
    // TODO: RTITs for correct core or both cores, depending on configuration
    struct hpsc_rti_timer *rtit0 = NULL;
    rtems_status_code rtit0_sc;
    cpu_set_t rtit0_cpuset;
    rtems_vector_number rtit0_vec =
        gic_irq_to_rvn(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI);
    // set CPU affinity
    rtit0_sc =
        rtems_task_get_affinity(RTEMS_SELF, sizeof(rtit0_cpuset), &rtit0_cpuset);
    assert(rtit0_sc == RTEMS_SUCCESSFUL);
    affinity_pin_self_to_cpu(0);
    rtit0_sc = hpsc_rti_timer_probe(&rtit0, "RTPS RTI TMR0",
                                    RTI_TIMER_RTPS_R52_0__RTPS_BASE, rtit0_vec);
    assert(rtit0_sc == RTEMS_SUCCESSFUL);
    dev_add_rtit(DEV_ID_CPU_RTPS_R52_0, rtit0);
    // restore CPU affinity
    rtit0_sc =
        rtems_task_set_affinity(RTEMS_SELF, sizeof(rtit0_cpuset), &rtit0_cpuset);
    assert(rtit0_sc == RTEMS_SUCCESSFUL);
#endif // CONFIG_RTI_TIMER

#if CONFIG_WDT
    // TODO: WDTs for correct core or both cores, depending on configuration
    struct hpsc_wdt *wdt0 = NULL;
    rtems_status_code wdt0_sc;
    cpu_set_t wdt0_cpuset;
    rtems_vector_number wdt0_vec =
        gic_irq_to_rvn(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI);
    // set CPU affinity
    wdt0_sc =
        rtems_task_get_affinity(RTEMS_SELF, sizeof(wdt0_cpuset), &wdt0_cpuset);
    assert(wdt0_sc == RTEMS_SUCCESSFUL);
    affinity_pin_self_to_cpu(0);
    wdt0_sc = hpsc_wdt_probe_target(&wdt0, "RTPS0", WDT_RTPS_R52_0_RTPS_BASE,
                                    wdt0_vec);
    assert(wdt0_sc == RTEMS_SUCCESSFUL);
    dev_add_wdt(DEV_ID_CPU_RTPS_R52_0, wdt0);
    // restore CPU affinity
    wdt0_sc =
        rtems_task_set_affinity(RTEMS_SELF, sizeof(wdt0_cpuset), &wdt0_cpuset);
    assert(wdt0_sc == RTEMS_SUCCESSFUL);
#endif // CONFIG_WDT

    return RTEMS_SUCCESSFUL;
}

static void init_tests(void)
{
#if TEST_RTI_TIMER
#if !CONFIG_RTI_TIMER
    #warning Ignoring TEST_RTI_TIMER - requires CONFIG_RTI_TIMER
#else
    if (test_cpu_rti_timers())
        rtems_panic("RTI Timer test");
#endif // CONFIG_RTI_TIMER
#endif // TEST_RTI_TIMER

#if TEST_MBOX_LSIO_LOOPBACK
#if !CONFIG_MBOX_LSIO
    #warning Ignoring TEST_MBOX_LSIO_LOOPBACK - requires CONFIG_MBOX_LSIO
#else
    if (test_mbox_lsio_loopback())
        rtems_panic("RTPS loopback mailbox");
#endif // CONFIG_MBOX_LSIO
#endif //TEST_MBOX_LSIO_LOOPBACK

#if TEST_MBOX_LSIO_TRCH
#if !CONFIG_MBOX_LSIO
    #warning Ignoring TEST_MBOX_LSIO_LOOPBACK - requires CONFIG_MBOX_LSIO
#else
    if (test_mbox_lsio_trch())
        rtems_panic("RTPS->TRCH mailbox");
#endif // CONFIG_MBOX_LSIO
#endif // TEST_MBOX_LSIO_TRCH

#if TEST_SHMEM
    if (test_shmem())
        rtems_panic("shmem test");
#endif // TEST_SHMEM
}

static void init_client_links(void)
{
    // placeholder
}

static void init_server_links()
{
#if CONFIG_MBOX_LINK_SERVER_HPPS
#if !CONFIG_MBOX_HPPS
    #warning Ignoring CONFIG_MBOX_LINK_SERVER_HPPS - requires CONFIG_MBOX_HPPS
#else
    struct hpsc_mbox *mbox_hpps = dev_get_mbox(DEV_ID_MBOX_HPPS);
    assert(mbox_hpps);
    struct link *hpps_link = mbox_link_connect("HPPS_MBOX_LINK", mbox_hpps,
                    MBOX_HPPS_RTPS__HPPS_RTPS, MBOX_HPPS_RTPS__RTPS_HPPS,
                    /* server */ MASTER_ID_RTPS_CPU0,
                    /* client */ MASTER_ID_HPPS_CPU0);
    if (!hpps_link)
        rtems_panic("HPPS link");
    // Never release the link, because we listen on it in main loop
#endif // CONFIG_MBOX_HPPS
#endif // CONFIG_MBOX_LINK_SERVER_HPPS
}

static void early_tasks(void)
{
    rtems_name task_name;
    rtems_id task_id;
    rtems_status_code sc;

    // command queue handler task
    task_name = rtems_build_name('C','M','D','H');
    sc = rtems_task_create(
        task_name, 1, RTEMS_MINIMUM_STACK_SIZE * 2,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    if (sc != RTEMS_SUCCESSFUL) 
        rtems_panic("command handler task create");
    sc = cmd_handle_task_start(task_id, server_process);
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic("command handler task start");
}

static void init_tasks(void)
{
#if CONFIG_WDT
    if (watchdog_tasks_create() != RTEMS_SUCCESSFUL)
        rtems_panic("watchdogs");
#endif // CONFIG_WDT
}

static void late_tasks(void)
{
#if CONFIG_SHELL
    if (shell_create() != RTEMS_SUCCESSFUL)
        rtems_panic("shell");
#endif // CONFIG_SHELL
}


static void runtime_tests(void)
{
    if (test_command_server())
        rtems_panic("Command server test");
}

void *POSIX_Init(void *arg)
{
    // device drivers already initialized
    printf("\n\nRTPS\n");

    // run boot tests
    init_tests();

    // start early tasks
    early_tasks();

    // initialize links - this order matters
    init_client_links();
    init_server_links();

    // start basic tasks
    init_tasks();

    // run tests that require basic tasks
    runtime_tests();

    // start remaining tasks
    late_tasks();

    // init task is finished
    rtems_task_exit();
}

/* configuration information */

#include <bsp.h>

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_NO_COMMAND_SHUTDOWN // we override
#define CONFIGURE_SHELL_USER_COMMANDS &shutdown_rtps_r52_command
#include <rtems/shellconfig.h>

/* drivers */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS \
  { .initialization_entry = init_extra_drivers }

/* POSIX */
#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_MAXIMUM_POSIX_THREADS          16
#define CONFIGURE_MAXIMUM_POSIX_KEYS             16
#define CONFIGURE_MAXIMUM_POSIX_KEY_VALUE_PAIRS  16

/* filesystem */
#define CONFIGURE_FILESYSTEM_DOSFS
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS   40
#define CONFIGURE_IMFS_MEMFILE_BYTES_PER_BLOCK    512

#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_SWAPOUT_TASK_PRIORITY            10

#define CONFIGURE_MAXIMUM_TASKS                    rtems_resource_unlimited (10)

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
