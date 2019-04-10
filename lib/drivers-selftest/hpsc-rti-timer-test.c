#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// drivers
#include <hpsc-rti-timer.h>

#include "hpsc-rti-timer-test.h"

#define INTERVAL_NS 1000000000
#define INTERVAL_US (INTERVAL_NS / 1000)

// Number of intervals to expect the ISR for.
#define NUM_EVENTS 3

static void handle_event(void *arg)
{
    volatile unsigned *events = arg;
    assert(events);
    (*events)++;
    printk("RTI TMR test: events -> %u\n", *events);
}

static int do_test(struct hpsc_rti_timer *tmr, unsigned *events)
{
    uint64_t count;
    uint64_t count2;
    unsigned i;
    assert(tmr);
    assert(events);

    count = hpsc_rti_timer_capture(tmr);
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(1));
    count2 = hpsc_rti_timer_capture(tmr);
    if (count2 <= count) {
        printf("TEST: FAIL: RTI TMR: value did not advance: "
               "0x%08x%08x <= 0x%08x%08x\n",
               (uint32_t)(count2 >> 32), (uint32_t)count2,
               (uint32_t)(count >> 32), (uint32_t)count);
        return HPSC_RTI_TIMER_TEST_NO_ADVANCE;
    }

    if (*events > 0) {
        printf("TEST: FAIL: RTI TMR: unexpected events\n");
        return HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENTS;
    }

    hpsc_rti_timer_configure(tmr, INTERVAL_NS);

    // offset the checks by an epsilon
    rtems_task_wake_after(RTEMS_MICROSECONDS_TO_TICKS(INTERVAL_US) / 10);

    for (i = 1; i <= NUM_EVENTS; ++i) {
#if 1  // TODO: remove this once timing is fixed in RTEMS
        rtems_task_wake_after(RTEMS_MICROSECONDS_TO_TICKS(INTERVAL_US / 8));
#else
        rtems_task_wake_after(RTEMS_MICROSECONDS_TO_TICKS(INTERVAL_US));
#endif
        if (*events != i) {
            printf("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\n",
                   *events, i);
            return HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENT_COUNT;
        }
    }
    return HPSC_RTI_TIMER_TEST_SUCCESS;
}

int hpsc_rti_timer_test_device(struct hpsc_rti_timer *tmr,
                               uint64_t reset_interval_ns)
{
    rtems_status_code sc;
    unsigned events = 0;
    int rc;
    assert(tmr);

    sc = hpsc_rti_timer_start(tmr, handle_event, &events);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_RTI_TIMER_TEST_START;

    rc = do_test(tmr, &events);

    // Since there's no way to disable the RTI timer, the best we can do to
    // reduce the load in the HW emulator, set the interval to max.
    hpsc_rti_timer_configure(tmr, reset_interval_ns);

    sc = hpsc_rti_timer_stop(tmr, handle_event, &events);
    if (sc != RTEMS_SUCCESSFUL)
        rc = HPSC_RTI_TIMER_TEST_STOP;

    return rc;
}

int hpsc_rti_timer_test(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
)
{
    struct hpsc_rti_timer *tmr = NULL;
    rtems_status_code sc;
    int rc;

    sc = hpsc_rti_timer_probe(&tmr, "RTI TMR", base, vec);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_RTI_TIMER_TEST_PROBE;

    rc = hpsc_rti_timer_test_device(tmr, reset_interval_ns);

    sc = hpsc_rti_timer_remove(tmr);
    if (sc != RTEMS_SUCCESSFUL)
        rc = HPSC_RTI_TIMER_TEST_REMOVE;
    return rc;
}
