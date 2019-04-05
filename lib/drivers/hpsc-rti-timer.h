#ifndef HPSC_RTI_TIMER_H
#define HPSC_RTI_TIMER_H

#include <stdint.h>

#include <rtems.h>

/**
 * A RTI Timer device (IP block instance)
 */
struct hpsc_rti_timer;

/**
 * A RTI Timer callback handle
 */
struct hpsc_rti_timer_cb;

typedef void (hpsc_rti_timer_cb_t)(struct hpsc_rti_timer *tmr, void *arg);

/**
 * Initialize a RTI Timer IP block
 */
rtems_status_code hpsc_rti_timer_probe(
    struct hpsc_rti_timer **tmr,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number vec
);

/**
 * Teardown a RTI Timer IP block
 */
rtems_status_code hpsc_rti_timer_remove(struct hpsc_rti_timer *tmr);

/**
 * Subscribe to a RTI Timer
 * Currently, there can only be one subscriber
 */
rtems_status_code hpsc_rti_timer_subscribe(struct hpsc_rti_timer *tmr,
                                           struct hpsc_rti_timer_cb **handle,
                                           hpsc_rti_timer_cb_t *cb,
                                           void *cb_arg);

/**
 * Unsubscribe from a RTI Timer
 */
rtems_status_code hpsc_rti_timer_unsubscribe(struct hpsc_rti_timer_cb *handle);

/**
 * Start a RTI Timer
 */
rtems_status_code hpsc_rti_timer_start(struct hpsc_rti_timer *tmr);

/**
 * Stop a RTI Timer
 */
rtems_status_code hpsc_rti_timer_stop(struct hpsc_rti_timer *tmr);

/**
 * Read a RTI Timer
 */
uint64_t hpsc_rti_timer_capture(struct hpsc_rti_timer *tmr);

/**
 * Configure a RTI Timer interval
 */
void hpsc_rti_timer_configure(struct hpsc_rti_timer *tmr, uint64_t interval);

#endif // HPSC_RTI_TIMER_H
