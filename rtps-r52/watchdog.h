#ifndef WATCHDOG_H
#define WATCHDOG_H

// driver
#include <hpsc-wdt.h>

void watchdog_enable(void);
void watchdog_timeout_isr(struct hpsc_wdt *wdt, void *arg);
void watchdog_kick(void);

#endif // WATCHDOG_H
