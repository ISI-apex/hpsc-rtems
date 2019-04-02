#include <assert.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// driver
#include <hpsc-wdt.h>

// plat
#include <hwinfo.h>

#include "gic.h"
#include "watchdog.h"

static struct hpsc_wdt *wdt = NULL;

// TODO: watchdogs for both cores
//struct hpsc_wdt *wdts[NUM_CORES]; // must be in global scope since needed by global ISR

static void handle_timeout(struct hpsc_wdt *wdt, void *arg)
{
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // nothing to do: main loop will return from WFI/WFE and kick
}

int watchdog_init(void)
{
    rtems_status_code sc;
    rtems_vector_number vec = gic_irq_to_rvn(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI);
    sc = hpsc_wdt_probe_target(&wdt, "RTPS0", WDT_RTPS_R52_0_RTPS_BASE, vec,
                               handle_timeout, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return 1;

    hpsc_wdt_enable(wdt);
    return 0;
}

void watchdog_deinit(void)
{
    assert(wdt);
    hpsc_wdt_remove(wdt);
    wdt = NULL;
}

void watchdog_kick(void)
{
    assert(wdt);
    hpsc_wdt_kick(wdt);
}
