#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// drivers
#include <hpsc-rti-timer.h>

#include "hpsc-bist-rti-timer.h"

#define INTERVAL_NS 1000000000

// Number of intervals to expect the ISR for.
#define NUM_EVENTS 3

static void handle_event(struct hpsc_rti_timer *tmr, void *arg)
{
    volatile unsigned *events = arg;
    (*events)++;
    printk("RTI TMR test: events -> %u\n", *events);
}

int hpsc_bist_rti_timer(
    volatile uint32_t *base,
    rtems_vector_number vec,
    uint64_t reset_interval_ns
)
{
    struct hpsc_rti_timer *tmr;
    struct timespec ts;
    rtems_status_code sc;
    uint64_t count;
    uint64_t count2;
    unsigned events = 0;
    unsigned i;
    int rc = 1;

    sc = hpsc_rti_timer_probe(&tmr, "RTI TMR", base, vec,
                              handle_event, &events);
    if (sc != RTEMS_SUCCESSFUL)
        return 1;

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
        goto cleanup;
    }

    if (events > 0) {
        printf("TEST: FAIL: RTI TMR: unexpected events\n");
        goto cleanup;
    }

    hpsc_rti_timer_configure(tmr, INTERVAL_NS);

    // offset the checks by an epsilon
    ts.tv_sec = (INTERVAL_NS / 10) / 1000000000;
    ts.tv_nsec = (INTERVAL_NS / 10) % 1000000000;
    nanosleep(&ts, NULL);
    ts.tv_sec = INTERVAL_NS / 1000000000;
    ts.tv_nsec = INTERVAL_NS % 1000000000;
    for (i = 1; i <= NUM_EVENTS; ++i) {
        nanosleep(&ts, NULL);
        if (events != i) {
            printf("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\n",
                   events, i);
            goto cleanup;
        }
    }
    rc = 0;

cleanup:
    // Since there's no way to disable the RTI timer, the best we can do to
    // reduce the load in the HW emulator, set the interval to max.
    hpsc_rti_timer_configure(tmr, reset_interval_ns);
    return hpsc_rti_timer_remove(tmr) == RTEMS_SUCCESSFUL ? rc : 1;
}
