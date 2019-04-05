#include <assert.h>
#include <stdio.h>

#include <rtems/bspIo.h>

// driver
#include <hpsc-wdt.h>

#include "devices.h"
#include "watchdog.h"


void watchdog_enable(void)
{
    struct hpsc_wdt *wdt;
    dev_id_cpu id;
    unsigned count = 0;
    dev_id_cpu_for_each_wdt(id, wdt) {
        if (wdt) {
            hpsc_wdt_enable(wdt);
            count++;
        }
    }
    assert(count); // we should enable at least one CPU's WDT
}

void watchdog_timeout_isr(struct hpsc_wdt *wdt, void *arg)
{
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // nothing to do: main loop will return from WFI/WFE and kick
}

void watchdog_kick(void)
{
    struct hpsc_wdt *wdt;
    dev_id_cpu id;
    unsigned count = 0;
    dev_id_cpu_for_each_wdt(id, wdt) {
        if (wdt) {
            hpsc_wdt_kick(wdt);
            count++;
        }
    }
    assert(count); // we should kick at least one CPU's WDT
}
