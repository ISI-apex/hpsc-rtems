#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// drivers
#include <hpsc-rti-timer.h>

#include "hpsc-rti-timer-test.h"

#define INTERVAL_NS 1000000000

// Number of intervals to expect the ISR for.
#define NUM_EVENTS 3

static void handle_event(struct hpsc_rti_timer *tmr, void *arg)
{
    volatile unsigned *events = arg;
    (*events)++;
    printk("RTI TMR test: events -> %u\n", *events);
}

int hpsc_rti_timer_test(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
)
{
    struct hpsc_rti_timer *tmr = NULL;
    struct hpsc_rti_timer_cb *tmr_handler = NULL;
    struct timespec ts;
    rtems_status_code sc;
    uint64_t count;
    uint64_t count2;
    unsigned events = 0;
    unsigned i;
    int rc;

    sc = hpsc_rti_timer_probe(&tmr, "RTI TMR", base, vec);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_RTI_TIMER_TEST_PROBE;

    sc = hpsc_rti_timer_subscribe(tmr, &tmr_handler, handle_event, &events);
    if (sc != RTEMS_SUCCESSFUL) {
        rc = HPSC_RTI_TIMER_TEST_SUBSCRIBE;
        goto remove;
    }

    sc = hpsc_rti_timer_start(tmr);
    if (sc != RTEMS_SUCCESSFUL) {
        rc = HPSC_RTI_TIMER_TEST_START;
        goto unsubscribe;
    }

    count = hpsc_rti_timer_capture(tmr);
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000; // 1 ms
    nanosleep(&ts, NULL);
    count2 = hpsc_rti_timer_capture(tmr);
    if (count2 <= count) {
        printf("TEST: FAIL: RTI TMR: value did not advance: "
               "0x%08x%08x <= 0x%08x%08x\n",
               (uint32_t)(count2 >> 32), (uint32_t)count2,
               (uint32_t)(count >> 32), (uint32_t)count);
        rc = HPSC_RTI_TIMER_TEST_NO_ADVANCE;
        goto cleanup;
    }

    if (events > 0) {
        printf("TEST: FAIL: RTI TMR: unexpected events\n");
        rc = HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENTS;
        goto cleanup;
    }

    hpsc_rti_timer_configure(tmr, INTERVAL_NS);

    // offset the checks by an epsilon
    ts.tv_sec = (INTERVAL_NS / 10) / 1000000000;
    ts.tv_nsec = (INTERVAL_NS / 10) % 1000000000;
    nanosleep(&ts, NULL);
#if 1 // TODO: remove this once timing is fixed in RTEMS
    ts.tv_sec = (INTERVAL_NS / 8) / 1000000000;
    ts.tv_nsec = (INTERVAL_NS / 8) % 1000000000;
#else
    ts.tv_sec = INTERVAL_NS / 1000000000;
    ts.tv_nsec = INTERVAL_NS % 1000000000;
#endif
    for (i = 1; i <= NUM_EVENTS; ++i) {
        nanosleep(&ts, NULL);
        if (events != i) {
            printf("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\n",
                   events, i);
            rc = HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENT_COUNT;
            goto cleanup;
        }
    }
    rc = HPSC_RTI_TIMER_TEST_SUCCESS;

cleanup:
    // Since there's no way to disable the RTI timer, the best we can do to
    // reduce the load in the HW emulator, set the interval to max.
    hpsc_rti_timer_configure(tmr, reset_interval_ns);

    sc = hpsc_rti_timer_stop(tmr);
    if (sc != RTEMS_SUCCESSFUL)
        rc = HPSC_RTI_TIMER_TEST_STOP;

unsubscribe:
    sc = hpsc_rti_timer_unsubscribe(tmr_handler);
    if (sc != RTEMS_SUCCESSFUL)
        rc = HPSC_RTI_TIMER_TEST_UNSUBSCRIBE;

remove:
    sc = hpsc_rti_timer_remove(tmr);
    if (sc != RTEMS_SUCCESSFUL)
        rc = HPSC_RTI_TIMER_TEST_REMOVE;
    return rc;
}
