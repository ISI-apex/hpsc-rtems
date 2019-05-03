#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// driver
#include <hpsc-wdt.h>

// libhpsc
#include <affinity.h>
#include <devices.h>
#include <watchdog-cpu.h>

#include "watchdog.h"

// TODO: get this interval dynamically (e.g., from device tree)
// The WDT has a 1 second first stage timeout by default
#if 1 // TODO: remove this after frequency is fixed in RTEMS
#define WDT_KICK_INTERAL_TICKS RTEMS_MICROSECONDS_TO_TICKS(500000 / 8)
#else
#define WDT_KICK_INTERAL_TICKS RTEMS_MICROSECONDS_TO_TICKS(500000)
#endif


static void watchdog_timeout_isr(void *arg)
{
    struct hpsc_wdt *wdt = (struct hpsc_wdt *)arg;
    assert(wdt);
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // TODO: the WDT task failed to kick - maybe initiate a graceful shutdown
}

static rtems_status_code watchdog_task_create(
    rtems_task_priority priority,
    struct hpsc_wdt *wdt,
    uint32_t cpu
)
{
    rtems_id task_id;
    rtems_name task_name;
    rtems_status_code sc;
    assert(wdt);

    task_name = rtems_build_name('W','D','T',cpu);
    sc = rtems_task_create(
        task_name, priority, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    if (sc != RTEMS_SUCCESSFUL) {
        printf("Failed to create watchdog task: %"PRIu32"\n", cpu);
        return sc;
    }
    // pin watchdog task to its CPU
    affinity_pin_to_cpu(task_id, cpu);
    sc = watchdog_cpu_task_start(wdt, task_id, WDT_KICK_INTERAL_TICKS,
                                 watchdog_timeout_isr, wdt);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("Failed to start watchdog task: %"PRIu32"\n", cpu);
        rtems_task_delete(task_id);
    }
    return sc;
}

rtems_status_code watchdog_tasks_create(rtems_task_priority priority)
{
    cpu_set_t cpuset;
    rtems_status_code sc_aff;
    rtems_status_code sc = RTEMS_SUCCESSFUL;
    struct hpsc_wdt *wdt;
    uint32_t cpu;

    // store CPU affinity before CPU-specific operations
    sc_aff = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc_aff == RTEMS_SUCCESSFUL);

    dev_cpu_for_each(cpu) {
        printf("Create watchdog task: %"PRIu32"\n", cpu);
        // pin so we can get device handle
        affinity_pin_self_to_cpu(cpu);
        wdt = dev_cpu_get_wdt();
        assert(wdt);
        sc = watchdog_task_create(priority, wdt, cpu);
        if (sc != RTEMS_SUCCESSFUL)
            break;
    }

    // restore CPU affinity
    sc_aff = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc_aff == RTEMS_SUCCESSFUL);

    return sc;
}

rtems_status_code watchdog_tasks_destroy(void)
{
    rtems_status_code sc = RTEMS_SUCCESSFUL;
    cpu_set_t cpuset;
    rtems_status_code sc_aff;
    uint32_t cpu;

    // store CPU affinity before CPU-specific operations
    sc_aff = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc_aff == RTEMS_SUCCESSFUL);

    dev_cpu_for_each(cpu) {
        printf("Stop watchdog task: %"PRIu32"\n", cpu);
        affinity_pin_self_to_cpu(cpu);
        sc = watchdog_cpu_task_stop();
        if (sc != RTEMS_SUCCESSFUL) {
            printf("Failed to stop watchdog task: %"PRIu32"\n", cpu);
            break;
        }
    }

    // restore CPU affinity
    sc_aff = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc_aff == RTEMS_SUCCESSFUL);

    return sc;
}
