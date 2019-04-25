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
#include <link.h>
#include <link-mbox.h>
#include <link-shmem.h>
#include <link-store.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

#include "devices.h"
#include "gic.h"
#include "link-names.h"
#include "server.h"
#include "shutdown.h"
#include "test.h"
#include "watchdog.h"

#define CMD_TIMEOUT_TICKS 1000
#define SHMEM_POLL_TICKS 10

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

#if CONFIG_MBOX_HPPS_RTPS
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
    dev_add_mbox(DEV_ID_MBOX_HPPS_RTPS, mbox_hpps);
#endif // CONFIG_MBOX_HPPS_RTPS

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

static void standalone_tests(void)
{
#if TEST_COMMAND
    if (test_command())
        rtems_panic("Command test");
#endif // TEST_COMMAND

#if TEST_MBOX_LSIO_LOOPBACK
#if !CONFIG_MBOX_LSIO
    #warning Ignoring TEST_MBOX_LSIO_LOOPBACK - requires CONFIG_MBOX_LSIO
#else
    if (test_mbox_lsio_loopback())
        rtems_panic("RTPS LSIO mbox loopback test");
#endif // CONFIG_MBOX_LSIO
#endif //TEST_MBOX_LSIO_LOOPBACK

#if TEST_RTI_TIMER
#if !CONFIG_RTI_TIMER
    #warning Ignoring TEST_RTI_TIMER - requires CONFIG_RTI_TIMER
#else
    if (test_cpu_rti_timers())
        rtems_panic("RTI Timer test");
#endif // CONFIG_RTI_TIMER
#endif // TEST_RTI_TIMER

#if TEST_SHMEM
    if (test_shmem())
        rtems_panic("shmem test");
#endif // TEST_SHMEM
}

static void runtime_tests(void)
{
#if TEST_COMMAND_SERVER
    if (test_command_server())
        rtems_panic("Command server test");
#endif // TEST_COMMAND_SERVER

#if TEST_LINK_SHMEM
    if (test_link_shmem())
        rtems_panic("Shmem link test");
#endif // TEST_LINK_SHMEM
}

static void external_tests(void)
{
#if TEST_MBOX_LINK_TRCH
#if !CONFIG_MBOX_LSIO
    #warning Ignoring TEST_MBOX_LINK_TRCH - requires CONFIG_MBOX_LSIO
#else
    if (test_link_mbox_trch())
        rtems_panic("RTPS->TRCH mailbox test");
#endif // CONFIG_MBOX_LSIO
#endif // TEST_MBOX_LINK_TRCH

#if TEST_SHMEM_LINK_TRCH
#if !CONFIG_LINK_SHMEM_TRCH_CLIENT
    #warning Ignoring TEST_SHMEM_LINK_TRCH - requires CONFIG_LINK_SHMEM_TRCH_CLIENT
#else
    if (test_link_shmem_trch())
        rtems_panic("RTPS->TRCH shmem test");
#endif // CONFIG_LINK_SHMEM_TRCH_CLIENT
#endif // TEST_SHMEM_LINK_TRCH
}

static void init_client_links(void)
{
#if CONFIG_LINK_MBOX_TRCH_CLIENT
#if !CONFIG_MBOX_LSIO
    #warning Ignoring CONFIG_LINK_MBOX_TRCH_CLIENT - requires CONFIG_MBOX_LSIO
#else
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    assert(mbox_lsio);
    struct link *tmc_link = link_mbox_connect(LINK_NAME__MBOX__TRCH_CLIENT,
        mbox_lsio, MBOX_LSIO__TRCH_RTPS, MBOX_LSIO__RTPS_TRCH, 
        /* server */ 0, /* client */ MASTER_ID_RTPS_CPU0);
    if (!tmc_link)
        rtems_panic(LINK_NAME__MBOX__TRCH_CLIENT);
    link_store_append(tmc_link);
#endif // CONFIG_MBOX_LSIO
#endif // CONFIG_LINK_MBOX_TRCH_CLIENT

#if CONFIG_LINK_SHMEM_TRCH_CLIENT
    rtems_status_code tsc_sc;
    rtems_id tsc_tid_recv;
    rtems_id tsc_tid_ack;
    rtems_name tsc_tn_recv = rtems_build_name('T', 'S', 'C', 'R');
    rtems_name tsc_tn_ack = rtems_build_name('T', 'S', 'C', 'A');
    tsc_sc = rtems_task_create(
        tsc_tn_recv, 1, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, &tsc_tid_recv);
    assert(tsc_sc == RTEMS_SUCCESSFUL);
    tsc_sc = rtems_task_create(
        tsc_tn_ack, 1, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, &tsc_tid_ack);
    assert(tsc_sc == RTEMS_SUCCESSFUL);
    struct link *tsc_link = link_shmem_connect(LINK_NAME__SHMEM__TRCH_CLIENT,
        (volatile void *) RTPS_R52_SHM_ADDR__RTPS_TRCH_SEND,
        (volatile void *) RTPS_R52_SHM_ADDR__TRCH_RTPS_REPLY,
        /* is_server */ false, SHMEM_POLL_TICKS, tsc_tid_recv, tsc_tid_ack);
    if (!tsc_link)
        rtems_panic(LINK_NAME__SHMEM__TRCH_CLIENT);
    link_store_append(tsc_link);
#endif // CONFIG_LINK_SHMEM_TRCH_CLIENT
}

static void init_server_links()
{
#if CONFIG_LINK_SHMEM_TRCH_SERVER
    rtems_status_code tss_sc;
    rtems_id tss_tid_recv;
    rtems_id tss_tid_ack;
    rtems_name tss_tn_recv = rtems_build_name('T', 'S', 'S', 'R');
    rtems_name tss_tn_ack = rtems_build_name('T', 'S', 'S', 'A');
    tss_sc = rtems_task_create(
        tss_tn_recv, 1, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, &tss_tid_recv);
    assert(tss_sc == RTEMS_SUCCESSFUL);
    tss_sc = rtems_task_create(
        tss_tn_ack, 1, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, &tss_tid_ack);
    assert(tss_sc == RTEMS_SUCCESSFUL);
    struct link *tss_link = link_shmem_connect(LINK_NAME__SHMEM__TRCH_SERVER,
        (volatile void *) RTPS_R52_SHM_ADDR__TRCH_RTPS_SEND,
        (volatile void *) RTPS_R52_SHM_ADDR__RTPS_TRCH_REPLY,
        /* is_server */ true, SHMEM_POLL_TICKS, tss_tid_recv, tss_tid_ack);
    if (!tss_link)
        rtems_panic(LINK_NAME__SHMEM__TRCH_SERVER);
    link_store_append(tss_link);
#endif // CONFIG_LINK_SHMEM_TRCH_SERVER

#if CONFIG_LINK_MBOX_HPPS_SERVER
#if !CONFIG_MBOX_HPPS_RTPS
    #warning Ignoring CONFIG_LINK_MBOX_HPPS_SERVER - requires CONFIG_MBOX_HPPS_RTPS
#else
    struct hpsc_mbox *mbox_hpps = dev_get_mbox(DEV_ID_MBOX_HPPS_RTPS);
    assert(mbox_hpps);
    struct link *hms_link = link_mbox_connect(LINK_NAME__MBOX__HPPS_SERVER,
        mbox_hpps, MBOX_HPPS_RTPS__HPPS_RTPS, MBOX_HPPS_RTPS__RTPS_HPPS,
        /* server */ MASTER_ID_RTPS_CPU0, /* client */ MASTER_ID_HPPS_CPU0);
    if (!hms_link)
        rtems_panic(LINK_NAME__MBOX__HPPS_SERVER);
    link_store_append(hms_link);
#endif // CONFIG_MBOX_HPPS_RTPS
#endif // CONFIG_LINK_MBOX_HPPS_SERVER
}

static void early_tasks(void)
{
    rtems_name task_name;
    rtems_id task_id;
    rtems_status_code sc;

    // command queue handler task
    task_name = rtems_build_name('C','M','D','H');
    sc = rtems_task_create(
        task_name, 1, RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    if (sc != RTEMS_SUCCESSFUL) 
        rtems_panic("command handler task create");
    sc = cmd_handle_task_start(task_id, server_process, CMD_TIMEOUT_TICKS);
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
    rtems_status_code sc_shell;
    printf(" =========================\n");
    printf(" Starting shell\n");
    printf(" =========================\n");
    sc_shell = rtems_shell_init(
        "SHLL",                       /* task name */
        RTEMS_MINIMUM_STACK_SIZE * 4, /* task stack size */
        100,                          /* task priority */
        "/dev/console",               /* device name */
        /* device is currently ignored by the shell if it is not a pty */
        false,                        /* run forever */
        false,                        /* wait for shell to terminate */
        NULL                          /* login check function,
                                         use NULL to disable a login check */
    );
    if (sc_shell != RTEMS_SUCCESSFUL)
        rtems_panic("shell");
#endif // CONFIG_SHELL
}

void *POSIX_Init(void *arg)
{
    // device drivers already initialized
    printf("\n\nRTPS\n");

    // run boot tests
    standalone_tests();

    // start early tasks
    early_tasks();

    // run tests that require early tasks
    runtime_tests();

    // initialize links
    init_client_links();
    init_server_links();

    // start basic tasks
    init_tasks();

    // run tests that require external interaction
    external_tests();

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
