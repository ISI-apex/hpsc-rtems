#ifndef HPSC_BIST_RTI_TIMER_H
#define HPSC_BIST_RTI_TIMER_H

#include <stdint.h>

#include <rtems.h>

enum hpsc_bist_rti_timer_rc {
    HPSC_BIST_RTI_TIMER_SUCCESS = 0,
    HPSC_BIST_RTI_TIMER_PROBE,
    HPSC_BIST_RTI_TIMER_REMOVE,
    HPSC_BIST_RTI_TIMER_NO_ADVANCE,
    HPSC_BIST_RTI_TIMER_UNEXPECTED_EVENTS,
    HPSC_BIST_RTI_TIMER_UNEXPECTED_EVENT_COUNT
};

int hpsc_bist_rti_timer(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
);

#endif // HPSC_BIST_RTI_TIMER_H
