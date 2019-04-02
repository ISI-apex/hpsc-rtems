#ifndef HPSC_BIST_RTI_TIMER_H
#define HPSC_BIST_RTI_TIMER_H

#include <stdint.h>

#include <rtems.h>

int hpsc_bist_rti_timer(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
);

#endif // HPSC_BIST_RTI_TIMER_H
