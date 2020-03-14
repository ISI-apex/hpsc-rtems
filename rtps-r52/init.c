#include <assert.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/shell.h>
#include <bsp/hwinfo.h>
#include <bsp/mpu.h>
#include <bsp/hpsc-wdt.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>

// libhpsc
#include <affinity.h>
#include <command.h>
#include <devices.h>
#include <link.h>
#include <link-mbox.h>
#include <link-shmem.h>
#include <link-store.h>

// plat
#include <mailbox-map.h>
#include <mem-map.h>

#include "gic.h"
#include "link-names.h"
#include "server.h"
#include "shell-tests.h"
#include "shutdown.h"
#include "test.h"
#include "watchdog.h"

// Design note:
// Incompatible CONFIG_* options -> compilation failure.
// Incompatible TEST_* options (e.g., missing CONFIG_* dependencies) -> warning.

#define CMD_TIMEOUT_TICKS 10000
#define SHMEM_POLL_TICKS 100

// lower values are higher priority, in range 1-255
#define TASK_PRI_WDT 1
#define TASK_PRI_SHMEM_POLL_TRCH 10
#define TASK_PRI_CMDH 20
#define TASK_PRI_SHELL 100

#define NAME_MBOX_TRCH "TRCH-RTPS Mailbox"
#define NAME_MBOX_HPPS "HPPS-RTPS Mailbox"

static rtems_status_code init_extra_drivers(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg
)
{
    cpu_set_t cpuset;
    rtems_status_code sc;

#if CONFIG_MBOX_LSIO
    struct hpsc_mbox *mbox_lsio = NULL;
    rtems_vector_number mbox_lsio_vec_a =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + LSIO_MBOX0_INT_EVT0__RTPS_R52_LOCKSTEP_SSW,
                       GIC_IRQ_TYPE_SPI);
    rtems_vector_number mbox_lsio_vec_b =
        gic_irq_to_rvn(RTPS_IRQ__TR_MBOX_0 + LSIO_MBOX0_INT_EVT1__RTPS_R52_LOCKSTEP_SSW,
                       GIC_IRQ_TYPE_SPI);
    sc = hpsc_mbox_probe(&mbox_lsio, NAME_MBOX_TRCH,
                         (uintptr_t) MBOX_LSIO__BASE,
                         mbox_lsio_vec_a,
                         LSIO_MBOX0_INT_EVT0__RTPS_R52_LOCKSTEP_SSW,
                         mbox_lsio_vec_b,
                         LSIO_MBOX0_INT_EVT1__RTPS_R52_LOCKSTEP_SSW);
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic(NAME_MBOX_TRCH);
    dev_set_mbox(DEV_ID_MBOX_LSIO, mbox_lsio);
#endif // CONFIG_MBOX_LSIO

#if CONFIG_MBOX_HPPS_RTPS
    struct hpsc_mbox *mbox_hpps = NULL;
    rtems_vector_number mbox_hpps_vec_a =
        gic_irq_to_rvn(RTPS_IRQ__HR_MBOX_0 + HPPS_MBOX1_INT_EVT0__RTPS_R52_LOCKSTEP_SSW,
                       GIC_IRQ_TYPE_SPI);
    rtems_vector_number mbox_hpps_vec_b =
        gic_irq_to_rvn(RTPS_IRQ__HR_MBOX_0 + HPPS_MBOX1_INT_EVT1__RTPS_R52_LOCKSTEP_SSW,
                       GIC_IRQ_TYPE_SPI);
    sc = hpsc_mbox_probe(&mbox_hpps, NAME_MBOX_HPPS,
                         (uintptr_t) MBOX_HPPS_RTPS__BASE,
                         mbox_hpps_vec_a,
                         HPPS_MBOX1_INT_EVT0__RTPS_R52_LOCKSTEP_SSW,
                         mbox_hpps_vec_b,
                         HPPS_MBOX1_INT_EVT1__RTPS_R52_LOCKSTEP_SSW);
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic(NAME_MBOX_HPPS);
    dev_set_mbox(DEV_ID_MBOX_HPPS_RTPS, mbox_hpps);
#endif // CONFIG_MBOX_HPPS_RTPS

    // store CPU affinity before CPU-specific operations
    sc = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

#if CONFIG_RTI_TIMER
    uintptr_t rtit_bases[] = {
        (uintptr_t) RTI_TIMER_RTPS_R52_0__RTPS_BASE,
        (uintptr_t) RTI_TIMER_RTPS_R52_1__RTPS_BASE
    };
    static const char *rtit_names[] = { "RTPS-R52-RTIT-0", "RTPS-R52-RTIT-1" };
    struct hpsc_rti_timer *rtit;
    uint32_t rtit_cpu;
    rtems_vector_number rtit_vec =
        gic_irq_to_rvn(PPI_IRQ__RTI_TIMER, GIC_IRQ_TYPE_PPI);
    // must set vector to edge-triggered
    gic_trigger_set(rtit_vec, GIC_EDGE_TRIGGERED);
    dev_cpu_for_each(rtit_cpu) {
        assert(rtit_cpu < RTEMS_ARRAY_SIZE(rtit_names));
        affinity_pin_self_to_cpu(rtit_cpu);
        sc = hpsc_rti_timer_probe(&rtit, rtit_names[rtit_cpu],
                                  rtit_bases[rtit_cpu], rtit_vec);
        if (sc != RTEMS_SUCCESSFUL)
            rtems_panic("%s", rtit_names[rtit_cpu]);
        dev_cpu_set_rtit(rtit);
    }
#endif // CONFIG_RTI_TIMER

#if CONFIG_WDT
    static const uintptr_t wdt_bases[] = {
        WDT_RTPS_R52_0_RTPS_BASE,
        WDT_RTPS_R52_1_RTPS_BASE
    };
    static const char *wdt_names[] = { "RTPS-R52-WDT-0", "RTPS-R52-WDT-1" };
    static struct HPSC_WDT_Config wdts[] = { { 0 }, { 0 } };
    uint32_t wdt_cpu;
    rtems_vector_number wdt_vec =
        gic_irq_to_rvn(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI);
    dev_cpu_for_each(wdt_cpu) {
        assert(wdt_cpu < RTEMS_ARRAY_SIZE(wdt_names));
        affinity_pin_self_to_cpu(wdt_cpu);
        wdt_init_target(&wdts[wdt_cpu], wdt_names[wdt_cpu], wdt_bases[wdt_cpu],
                        wdt_vec);
        dev_cpu_set_wdt(&wdts[wdt_cpu]);
    }
#endif // CONFIG_WDT

    // restore CPU affinity
    sc = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

    return RTEMS_SUCCESSFUL;
}

static void standalone_tests(void)
{
#if TEST_COMMAND
    if (test_command())
        rtems_panic("Command test");
#endif // TEST_COMMAND

#if TEST_LSIO_SRAM
    if (test_lsio_sram())
        rtems_panic("LSIO SRAM test");
#endif // TEST_LSIO_SRAM

#if TEST_LSIO_SRAM_DMA
    if (test_lsio_sram_dma())
        rtems_panic("LSIO SRAM w/ DMA test");
#endif // TEST_LSIO_SRAM_DMA

#if TEST_MBOX_LSIO_LOOPBACK
#if !CONFIG_MBOX_LSIO
    #warning Ignoring TEST_MBOX_LSIO_LOOPBACK - requires CONFIG_MBOX_LSIO
#else
    if (test_mbox_lsio_loopback())
        rtems_panic("RTPS LSIO mbox loopback test");
#endif // CONFIG_MBOX_LSIO
#endif // TEST_MBOX_LSIO_LOOPBACK

#if TEST_RTI_TIMER
#if !CONFIG_RTI_TIMER
    #warning Ignoring TEST_RTI_TIMER - requires CONFIG_RTI_TIMER
#else
    if (test_cpu_rti_timers())
        rtems_panic("RTI Timer test");
#endif // CONFIG_RTI_TIMER
#endif // TEST_RTI_TIMER

#if TEST_RTPS_DMA
#if TEST_RTPS_MMU
    if (test_rtps_mmu(true))
        rtems_panic("RTPS MMU+DMA test");
#else
    #warning Ignoring TEST_RTPS_DMA - requires TEST_RTPS_MMU
#endif // TEST_RTPS_MMU
#elif TEST_RTPS_MMU
    if (test_rtps_mmu(false))
        rtems_panic("RTPS MMU test");
#endif // TEST_RTPS_DMA

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
#if !CONFIG_LINK_MBOX_TRCH_CLIENT
    #warning Ignoring TEST_MBOX_LINK_TRCH - requires CONFIG_LINK_MBOX_TRCH_CLIENT
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
    rtems_status_code sc RTEMS_UNUSED;

#if CONFIG_LINK_MBOX_TRCH_CLIENT
#if !CONFIG_MBOX_LSIO
    #error CONFIG_LINK_MBOX_TRCH_CLIENT requires CONFIG_MBOX_LSIO
#endif // CONFIG_MBOX_LSIO
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    assert(mbox_lsio);
    struct link *tmc_link = link_mbox_connect(LINK_NAME__MBOX__TRCH_CLIENT,
        mbox_lsio,
        LSIO_MBOX0_CHAN__TRCH_SSW__RTPS_R52_LOCKSTEP_SSW,
        LSIO_MBOX0_CHAN__RTPS_R52_LOCKSTEP_SSW__TRCH_SSW, 
        /* server */ 0, /* client */ MASTER_ID_RTPS_CPU0);
    if (!tmc_link)
        rtems_panic(LINK_NAME__MBOX__TRCH_CLIENT);
    link_store_append(tmc_link);
#endif // CONFIG_LINK_MBOX_TRCH_CLIENT

#if CONFIG_LINK_SHMEM_TRCH_CLIENT
    rtems_id tsc_tid_recv;
    rtems_id tsc_tid_ack;
    rtems_name tsc_tn_recv = rtems_build_name('T', 'S', 'C', 'R');
    rtems_name tsc_tn_ack = rtems_build_name('T', 'S', 'C', 'A');
    sc = rtems_task_create(
        tsc_tn_recv, TASK_PRI_SHMEM_POLL_TRCH, RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &tsc_tid_recv
    );
    assert(sc == RTEMS_SUCCESSFUL);
    sc = rtems_task_create(
        tsc_tn_ack, TASK_PRI_SHMEM_POLL_TRCH, RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &tsc_tid_ack
    );
    assert(sc == RTEMS_SUCCESSFUL);
    struct link *tsc_link = link_shmem_connect(LINK_NAME__SHMEM__TRCH_CLIENT,
        RTPS_DDR_ADDR__SHM__RTPS_R52_LOCKSTEP_SSW__TRCH_SSW,
        RTPS_DDR_ADDR__SHM__TRCH_SSW__RTPS_R52_LOCKSTEP_SSW,
        /* is_server */ false, SHMEM_POLL_TICKS, tsc_tid_recv, tsc_tid_ack);
    if (!tsc_link)
        rtems_panic(LINK_NAME__SHMEM__TRCH_CLIENT);
    link_store_append(tsc_link);
#endif // CONFIG_LINK_SHMEM_TRCH_CLIENT
}

static void init_server_links(void)
{
    rtems_status_code sc RTEMS_UNUSED;

// TODO: Disabled - ultimately rm once we support bidirectional links
// #if CONFIG_LINK_SHMEM_TRCH_SERVER
//     rtems_id tss_tid_recv;
//     rtems_id tss_tid_ack;
//     rtems_name tss_tn_recv = rtems_build_name('T', 'S', 'S', 'R');
//     rtems_name tss_tn_ack = rtems_build_name('T', 'S', 'S', 'A');
//     sc = rtems_task_create(
//         tss_tn_recv, TASK_PRI_SHMEM_POLL_TRCH, RTEMS_MINIMUM_STACK_SIZE,
//         RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &tss_tid_recv
//     );
//     assert(sc == RTEMS_SUCCESSFUL);
//     sc = rtems_task_create(
//         tss_tn_ack, TASK_PRI_SHMEM_POLL_TRCH, RTEMS_MINIMUM_STACK_SIZE,
//         RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &tss_tid_ack
//     );
//     assert(sc == RTEMS_SUCCESSFUL);
//     struct link *tss_link = link_shmem_connect(LINK_NAME__SHMEM__TRCH_SERVER,
//         RTPS_DDR_ADDR__SHM__TRCH_SSW__RTPS_R52_LOCKSTEP_SSW,
//         RTPS_DDR_ADDR__SHM__RTPS_R52_LOCKSTEP_SSW__TRCH_SSW,
//         /* is_server */ true, SHMEM_POLL_TICKS, tss_tid_recv, tss_tid_ack);
//     if (!tss_link)
//         rtems_panic(LINK_NAME__SHMEM__TRCH_SERVER);
//     link_store_append(tss_link);
// #endif // CONFIG_LINK_SHMEM_TRCH_SERVER

#if CONFIG_LINK_MBOX_HPPS_SERVER
#if !CONFIG_MBOX_HPPS_RTPS
    #error CONFIG_LINK_MBOX_HPPS_SERVER requires CONFIG_MBOX_HPPS_RTPS
#endif // CONFIG_MBOX_HPPS_RTPS
    struct hpsc_mbox *mbox_hpps = dev_get_mbox(DEV_ID_MBOX_HPPS_RTPS);
    assert(mbox_hpps);
    struct link *hms_link = link_mbox_connect(LINK_NAME__MBOX__HPPS_SERVER,
        mbox_hpps,
        HPPS_MBOX1_CHAN__HPPS_SMP_APP__RTPS_R52_LOCKSTEP_SSW,
        HPPS_MBOX1_CHAN__RTPS_R52_LOCKSTEP_SSW__HPPS_SMP_APP,
        /* server */ MASTER_ID_RTPS_CPU0, /* client */ MASTER_ID_HPPS_CPU0);
    if (!hms_link)
        rtems_panic(LINK_NAME__MBOX__HPPS_SERVER);
    link_store_append(hms_link);
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
        task_name, TASK_PRI_CMDH, RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    assert(sc == RTEMS_SUCCESSFUL);
    sc = cmd_handle_task_start(task_id, server_process, CMD_TIMEOUT_TICKS);
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic("command handler task start");
}

static void init_tasks(void)
{
#if CONFIG_WDT
    watchdog_tasks_create(TASK_PRI_WDT);
#endif // CONFIG_WDT
}

static void late_tasks(void)
{
#if CONFIG_SHELL
    printf(" =========================\n");
    printf(" Starting shell\n");
    printf(" =========================\n");
    rtems_status_code sc = rtems_shell_init(
        "SHLL",                       /* task name */
        RTEMS_MINIMUM_STACK_SIZE * 4, /* task stack size */
        TASK_PRI_SHELL,               /* task priority */
        "/dev/console",               /* device name */
        /* device is currently ignored by the shell if it is not a pty */
        false,                        /* run forever */
        false,                        /* wait for shell to terminate */
        NULL                          /* login check function,
                                         use NULL to disable a login check */
    );
    if (sc != RTEMS_SUCCESSFUL)
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
// TODO: test_command requires changing state in lib/hpsc/command.c, which by
//       runtime is already reserved by the application. State would have be
//       moved out of static structs in command.c and managed by application
//       before we could enable this test at runtime.
#define CONFIGURE_SHELL_USER_COMMANDS \
    /* functionality commands */ \
    &shutdown_rtps_r52_command, \
    /* standalone tests */ \
    /* &shell_cmd_test_command, */ \
    &shell_cmd_test_cpu_rti_timers, \
    &shell_cmd_test_lsio_sram, \
    &shell_cmd_test_lsio_sram_dma, \
    &shell_cmd_test_mbox_lsio_loopback, \
    &shell_cmd_test_rtps_mmu, \
    &shell_cmd_test_rtps_mmu_dma, \
    &shell_cmd_test_shmem, \
    /* runtime tests */ \
    &shell_cmd_test_command_server, \
    &shell_cmd_test_link_shmem, \
    /* externally-dependent tests */ \
    &shell_cmd_test_link_mbox_trch, \
    &shell_cmd_test_link_shmem_trch, \
    &shell_cmd_test_mbox_rtps_hpps
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

/* for MMU */
#define CONFIGURE_MAXIMUM_REGIONS                   1

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
