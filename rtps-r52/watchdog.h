#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <rtems.h>

// driver
#include <hpsc-wdt.h>

rtems_status_code watchdog_tasks_create(void);

void watchdog_timeout_isr(struct hpsc_wdt *wdt, void *arg);

#endif // WATCHDOG_H
