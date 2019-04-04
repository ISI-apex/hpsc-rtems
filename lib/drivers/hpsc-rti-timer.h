#ifndef HPSC_RTI_TIMER_H
#define HPSC_RTI_TIMER_H

#include <stdint.h>

#include <rtems.h>

struct hpsc_rti_timer;

typedef void (hpsc_rti_timer_cb_t)(struct hpsc_rti_timer *tmr, void *arg);

rtems_status_code hpsc_rti_timer_probe(
    struct hpsc_rti_timer **tmr,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number vec,
    hpsc_rti_timer_cb_t *cb,
    void *cb_arg
);

rtems_status_code hpsc_rti_timer_remove(struct hpsc_rti_timer *tmr);

uint64_t hpsc_rti_timer_capture(struct hpsc_rti_timer *tmr);

void hpsc_rti_timer_configure(struct hpsc_rti_timer *tmr, uint64_t interval);

#endif // HPSC_RTI_TIMER_H
