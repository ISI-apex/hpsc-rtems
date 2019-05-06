#ifndef HPSC_WDT_H
#define HPSC_WDT_H

#include <stdint.h>
#include <stdbool.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

struct hpsc_wdt;

// If fewer stages than the timer HW supports are passed here,
// then the interrupts of latter stages are ignored (but they
// still happen, since all stages are always active in HW).
//
// Note: clk_freq_hz and max_div are fixed hardware characteristics of the
// board (but not of the IP block), i.e. what would be defined by the device
// tree node, hence they are not hardcoded in the driver. To divide the
// frequency, you would change the argument to hpsc_wdt_configure, not here.

/**
 * Initialize a WDT IP block from a monitoring CPU.
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_wdt_probe_monitor(
    struct hpsc_wdt **wdt,
    const char *name,
    uintptr_t base,
    rtems_vector_number intr_vec,
    uint32_t clk_freq_hz,
    unsigned max_div
);

/**
 * Initialize a WDT IP block from the target CPU.
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_wdt_probe_target(
    struct hpsc_wdt **wdt,
    const char *name,
    uintptr_t base,
    rtems_vector_number intr_vec
);

/**
 * Teardown a WDT IP block
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_wdt_remove(struct hpsc_wdt *wdt);

/**
 * Configure a WDT.
 */
rtems_status_code hpsc_wdt_configure(
    struct hpsc_wdt *wdt,
    unsigned freq,
    unsigned num_stages,
    uint64_t *timeouts
);

/**
 * Read a WDT count.
 */
uint64_t hpsc_wdt_count(struct hpsc_wdt *wdt, unsigned stage);

/**
 * Read a WDT timeout.
 */
uint64_t hpsc_wdt_timeout(struct hpsc_wdt *wdt, unsigned stage);

/**
 * Clear a WDT timeout.
 */
void hpsc_wdt_timeout_clear(struct hpsc_wdt *wdt, unsigned stage);

/**
 * Check if WDT is enabled.
 */
bool hpsc_wdt_is_enabled(struct hpsc_wdt *wdt);

/**
 * Register a WDT ISR (does not affect WDT enabled/disabled state).
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_wdt_handler_install(
    struct hpsc_wdt *wdt,
    rtems_interrupt_handler cb,
    void *cb_arg
);

/**
 * Unregister a WDT ISR (does not affect WDT enabled/disabled state).
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_wdt_handler_remove(
    struct hpsc_wdt *wdt,
    rtems_interrupt_handler cb,
    void *cb_arg
);

/**
 * Enable a WDT.
 */
void hpsc_wdt_enable(struct hpsc_wdt *wdt);

/**
 * Disable a WDT (may only be disabled by a monitor).
 */
void hpsc_wdt_disable(struct hpsc_wdt *wdt);

/**
 * Kick a WDT.
 */
void hpsc_wdt_kick(struct hpsc_wdt *wdt);

#endif // HPSC_WDT_H
