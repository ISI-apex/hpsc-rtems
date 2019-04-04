#ifndef HPSC_WDT_H
#define HPSC_WDT_H

#include <stdint.h>
#include <stdbool.h>

#include <rtems.h>

struct hpsc_wdt;

typedef void (*hpsc_wdt_cb_t)(struct hpsc_wdt *wdt, void *arg);

// If fewer stages than the timer HW supports are passed here,
// then the interrupts of latter stages are ignored (but they
// still happen, since all stages are always active in HW).
//
// Note: clk_freq_hz and max_div are fixed hardware characteristics of the
// board (but not of the IP block), i.e. what would be defined by the device
// tree node, hence they are not hardcoded in the driver. To divide the
// frequency, you would change the argument to hpsc_wdt_configure, not here.
rtems_status_code hpsc_wdt_probe_monitor(
    struct hpsc_wdt **wdt,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number intr_vec,
    hpsc_wdt_cb_t cb,
    void *cb_arg,
    uint32_t clk_freq_hz,
    unsigned max_div
);

rtems_status_code hpsc_wdt_probe_target(
    struct hpsc_wdt **wdt,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number intr_vec,
    hpsc_wdt_cb_t cb,
    void *cb_arg
);

rtems_status_code hpsc_wdt_remove(struct hpsc_wdt *wdt);

int hpsc_wdt_configure(
    struct hpsc_wdt *wdt,
    unsigned freq,
    unsigned num_stages,
    uint64_t *timeouts
);

uint64_t hpsc_wdt_count(struct hpsc_wdt *wdt, unsigned stage);

uint64_t hpsc_wdt_timeout(struct hpsc_wdt *wdt, unsigned stage);

void hpsc_wdt_timeout_clear(struct hpsc_wdt *wdt, unsigned stage);

bool hpsc_wdt_is_enabled(struct hpsc_wdt *wdt);

void hpsc_wdt_enable(struct hpsc_wdt *wdt);

void hpsc_wdt_disable(struct hpsc_wdt *wdt);

void hpsc_wdt_kick(struct hpsc_wdt *wdt);

#endif // HPSC_WDT_H
