#include <assert.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// driver
#include <hpsc-wdt.h>

// plat
#include <hwinfo.h>

#include "devices.h"
#include "gic.h"
#include "watchdog.h"


// TODO: WDTs for correct core or both cores, depending on configuration

static void handle_timeout(struct hpsc_wdt *wdt, void *arg)
{
    hpsc_wdt_timeout_clear(wdt, 0);
    printk("watchdog: expired\n");
    // nothing to do: main loop will return from WFI/WFE and kick
}

int watchdog_init(void)
{
    struct hpsc_wdt *wdt0 = NULL;
    rtems_status_code sc;
    rtems_vector_number vec = gic_irq_to_rvn(PPI_IRQ__WDT, GIC_IRQ_TYPE_PPI);
    sc = hpsc_wdt_probe_target(&wdt0, "RTPS0", WDT_RTPS_R52_0_RTPS_BASE, vec,
                               handle_timeout, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return 1;
    assert(wdt0);

    dev_add_wdt(DEV_ID_CPU_RTPS_R52_0, wdt0);
    hpsc_wdt_enable(wdt0);
    return 0;
}

void watchdog_deinit(void)
{
    struct hpsc_wdt *wdt0 = dev_get_wdt(DEV_ID_CPU_RTPS_R52_0);
    assert(wdt0);
    hpsc_wdt_remove(wdt0);
    dev_remove_wdt(DEV_ID_CPU_RTPS_R52_0);
}

void watchdog_kick(void)
{
    struct hpsc_wdt *wdt0 = dev_get_wdt(DEV_ID_CPU_RTPS_R52_0);
    assert(wdt0);
    hpsc_wdt_kick(wdt0);
}
