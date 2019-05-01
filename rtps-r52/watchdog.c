#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <rtems/bspIo.h>

// driver
#include <hpsc-wdt.h>

// libhpsc
#include <affinity.h>
#include <devices.h>

#include "watchdog.h"

// TODO: get this interval dynamically (e.g., from device tree)
// The WDT has a 1 second first stage timeout by default
#define WDT_KICK_INTERVAL_US 500000


static void watchdog_timeout_isr(void *arg)
{
    struct hpsc_wdt *wdt = (struct hpsc_wdt *)arg;
    assert(wdt);
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // TODO: the WDT task failed to kick - maybe initiate a graceful shutdown
}

static rtems_task watchdog_task(rtems_task_argument arg)
{
    rtems_interval ticks = RTEMS_MICROSECONDS_TO_TICKS(WDT_KICK_INTERVAL_US);
    unsigned cpu = (unsigned) arg;
    struct hpsc_wdt *wdt;

    // must run from the WDT's CPU
    affinity_pin_self_to_cpu(cpu);

    wdt = dev_cpu_get_wdt();
    assert(wdt);

    // once enabled, the WDT can't be stopped
    hpsc_wdt_enable(wdt, watchdog_timeout_isr, wdt);

    while (1) {
        // get WDT every time, in case it's removed (still a race condition)
        wdt = dev_cpu_get_wdt();
        if (wdt)
            hpsc_wdt_kick(wdt);
        else
            // not much we can do except to keep trying
            printf("watchdog_task: no WDT device found for cpu: %u\n", cpu);
#if 1 // TODO: remove this after frequency is fixed in RTEMS
        rtems_task_wake_after(ticks / 8);
#else
        rtems_task_wake_after(ticks);
#endif
    }
}

rtems_status_code watchdog_tasks_create(void)
{
    rtems_id task_id;
    rtems_name task_name;
    rtems_status_code sc = RTEMS_SUCCESSFUL;
    unsigned cpu;

    dev_cpu_for_each(cpu) {
        printf("Create watchdog task: %u\n", cpu);
        task_name = rtems_build_name('W','D','T',cpu);
        sc = rtems_task_create(
            task_name, 1, RTEMS_MINIMUM_STACK_SIZE,
            RTEMS_DEFAULT_MODES,
            RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &task_id
        );
        if (sc != RTEMS_SUCCESSFUL) {
            printf("Failed to create watchdog task: %u\n", cpu);
            break;
        }
        sc = rtems_task_start(task_id, watchdog_task, cpu);
        if (sc != RTEMS_SUCCESSFUL) {
            printf("Failed to start watchdog task: %u\n", cpu);
            rtems_task_delete(task_id);
            break;
        }
    }

    return sc;
}
