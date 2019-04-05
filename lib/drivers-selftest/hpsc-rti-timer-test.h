#ifndef HPSC_RTI_TIMER_TEST_H
#define HPSC_RTI_TIMER_TEST_H

#include <stdint.h>

#include <rtems.h>

enum hpsc_rti_timer_test_rc {
    HPSC_RTI_TIMER_TEST_SUCCESS = 0,
    HPSC_RTI_TIMER_TEST_PROBE,
    HPSC_RTI_TIMER_TEST_REMOVE,
    HPSC_RTI_TIMER_TEST_SUBSCRIBE,
    HPSC_RTI_TIMER_TEST_UNSUBSCRIBE,
    HPSC_RTI_TIMER_TEST_START,
    HPSC_RTI_TIMER_TEST_STOP,
    HPSC_RTI_TIMER_TEST_NO_ADVANCE,
    HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENTS,
    HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENT_COUNT
};

int hpsc_rti_timer_test(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
);

#endif // HPSC_RTI_TIMER_TEST_H
