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
#define WDT_KICK_INTERAL_TICKS RTEMS_MICROSECONDS_TO_TICKS(500000)


static void watchdog_timeout_isr(void *arg)
{
    struct hpsc_wdt *wdt = (struct hpsc_wdt *)arg;
    assert(wdt);
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // TODO: the WDT task failed to kick - maybe initiate a graceful shutdown
}

static void watchdog_task_create(
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
    assert(sc == RTEMS_SUCCESSFUL);
    // pin watchdog task to its CPU
    sc = affinity_pin_to_cpu(task_id, cpu);
    assert(sc == RTEMS_SUCCESSFUL);
    sc = watchdog_cpu_task_start(wdt, task_id, WDT_KICK_INTERAL_TICKS,
                                 watchdog_timeout_isr, wdt);
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic("watchdog_task_create: watchdog_cpu_task_start");
}

void watchdog_tasks_create(rtems_task_priority priority)
{
    cpu_set_t cpuset;
    rtems_status_code sc RTEMS_UNUSED;
    uint32_t cpu;

    // store CPU affinity before CPU-specific operations
    sc = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

    dev_cpu_for_each(cpu) {
        printf("Watchdog task: start: %"PRIu32"\n", cpu);
        // pin so we can get device handle
        affinity_pin_self_to_cpu(cpu);
        watchdog_task_create(priority, dev_cpu_get_wdt(), cpu);
    }

    // restore CPU affinity
    sc = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);
}

void watchdog_tasks_destroy(void)
{
    cpu_set_t cpuset;
    rtems_status_code sc RTEMS_UNUSED;
    uint32_t cpu;

    // store CPU affinity before CPU-specific operations
    sc = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

    dev_cpu_for_each(cpu) {
        printf("Watchdog task: stop: %"PRIu32"\n", cpu);
        affinity_pin_self_to_cpu(cpu);
        sc = watchdog_cpu_task_stop();
        // RTEMS_NOT_DEFINED means no task was running
        if (sc != RTEMS_SUCCESSFUL && sc != RTEMS_NOT_DEFINED)
            rtems_panic("watchdog_tasks_destroy: watchdog_cpu_task_stop");
    }

    // restore CPU affinity
    sc = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);
}
