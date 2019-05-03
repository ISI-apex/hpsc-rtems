#ifndef WATCHDOG_CPU_H
#define WATCHDOG_CPU_H

#include <rtems.h>
#include <rtems/irq-extension.h>

// drivers
#include <hpsc-wdt.h>

/**
 * Start the watchdog kicker task for the current CPU.
 * The caller is responsible for pinning the task to the appropriate CPU.
 */
rtems_status_code watchdog_cpu_task_start(
    struct hpsc_wdt *wdt,
    rtems_id task_id,
    rtems_interval ticks,
    rtems_interrupt_handler cb,
    void *cb_arg
);

/**
 * Stop the watchdog kicker task for the current CPU.
 */
rtems_status_code watchdog_cpu_task_stop(void);

#endif // WATCHDOG_CPU_H
