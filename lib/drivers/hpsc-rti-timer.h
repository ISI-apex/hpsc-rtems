#ifndef HPSC_RTI_TIMER_H
#define HPSC_RTI_TIMER_H

#include <stdint.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

/**
 * A RTI Timer device (IP block instance)
 */
struct hpsc_rti_timer;

/**
 * Initialize a RTI Timer IP block
 */
rtems_status_code hpsc_rti_timer_probe(
    struct hpsc_rti_timer **tmr,
    const char *name,
    uintptr_t base,
    rtems_vector_number vec
);

/**
 * Teardown a RTI Timer IP block
 */
rtems_status_code hpsc_rti_timer_remove(struct hpsc_rti_timer *tmr);

/**
 * Start a RTI Timer
 */
rtems_status_code hpsc_rti_timer_start(
    struct hpsc_rti_timer *tmr,
    rtems_interrupt_handler handler,
    void *arg
);

/**
 * Stop a RTI Timer
 */
rtems_status_code hpsc_rti_timer_stop(
    struct hpsc_rti_timer *tmr,
    rtems_interrupt_handler handler,
    void *arg
);

/**
 * Read a RTI Timer
 */
uint64_t hpsc_rti_timer_capture(struct hpsc_rti_timer *tmr);

/**
 * Configure a RTI Timer interval
 */
void hpsc_rti_timer_configure(struct hpsc_rti_timer *tmr, uint64_t interval);

#endif // HPSC_RTI_TIMER_H
