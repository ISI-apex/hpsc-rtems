#include <assert.h>
#include <inttypes.h>

#include <rtems.h>
#include <rtems/bspIo.h>

// drivers
#include <hpsc-rti-timer.h>

#include "hpsc-rti-timer-test.h"

// Number of ticks to wait before checking if RTI timer advanced
#define TEST_ADVANCE_TICKS 1

// Number of intervals to expect the ISR for.
#define TEST_EVENTS_NUM 10
// Timing can be unpredictable, esp. in emulation, so give or take an event
// 10 +/- 1 bounds test timing accuracy to +/-20% (+/-1.9999... events)
// Note: testing 1000 events resulted in skew of ~10
#define TEST_EVENTS_NUM_SKEW 1

#define TEST_EVENTS_INTERVAL_NS 100000000 // 100 ms
#define TEST_EVENTS_INTERVAL_US (TEST_EVENTS_INTERVAL_NS / 1000)

struct rtit_test_ctx {
    struct hpsc_rti_timer *tmr;
    unsigned events;
    rtems_id tid;
};

static void handle_event(void *arg)
{
    struct rtit_test_ctx *ctx = arg;
    assert(ctx);
    ctx->events++;
    printk("RTI TMR test: events -> %u\n", ctx->events);
    if (ctx->events == TEST_EVENTS_NUM)
        rtems_event_send(ctx->tid, RTEMS_EVENT_0);
}

static int do_test_advance(struct rtit_test_ctx *ctx)
{
    uint64_t count;
    uint64_t count2;
    assert(ctx);

    count = hpsc_rti_timer_capture(ctx->tmr);
    rtems_task_wake_after(TEST_ADVANCE_TICKS);
    count2 = hpsc_rti_timer_capture(ctx->tmr);
    if (count2 <= count) {
        printk("TEST: FAIL: RTI TMR: value did not advance: "
               "0x%016llx <= 0x%016llx\n", count2, count);
        return HPSC_RTI_TIMER_TEST_NO_ADVANCE;
    }
    return HPSC_RTI_TIMER_TEST_SUCCESS;
}

static int do_test_events(struct rtit_test_ctx *ctx)
{
    rtems_event_set evset = 0;
    rtems_interval ticks = 
#if 1  // TODO: remove this once timing is fixed in RTEMS
        RTEMS_MICROSECONDS_TO_TICKS(TEST_EVENTS_NUM * TEST_EVENTS_INTERVAL_US / 8);
#else
        RTEMS_MICROSECONDS_TO_TICKS(TEST_EVENTS_NUM * TEST_EVENTS_INTERVAL_US);
#endif
    assert(ctx);

    ctx->events = 0;
    hpsc_rti_timer_configure(ctx->tmr, TEST_EVENTS_INTERVAL_NS);

    // Empirically (in emulation), timeout usually occurs before final ISR
    // Offset extra half interval (best chance of success within skew tolerance)
    ticks += RTEMS_MICROSECONDS_TO_TICKS(TEST_EVENTS_INTERVAL_US / 2);
    rtems_event_receive(RTEMS_EVENT_0, RTEMS_EVENT_ALL, ticks, &evset);

    if (ctx->events < TEST_EVENTS_NUM - TEST_EVENTS_NUM_SKEW ||
        ctx->events > TEST_EVENTS_NUM + TEST_EVENTS_NUM_SKEW) {
        printk("TEST: FAIL: RTI TMR: unexpected event count: %u != %u\n",
               ctx->events, TEST_EVENTS_NUM);
        return HPSC_RTI_TIMER_TEST_UNEXPECTED_EVENT_COUNT;
    } else if (ctx->events != TEST_EVENTS_NUM) {
        printk("TEST: WARN: RTI TMR: event skew (within tolerance): %u != %u\n",
               ctx->events, TEST_EVENTS_NUM);
    }

    return HPSC_RTI_TIMER_TEST_SUCCESS;
}

int hpsc_rti_timer_test_device(struct hpsc_rti_timer *tmr,
                               uint64_t reset_interval_ns)
{
    rtems_status_code sc;
    int rc;
    struct rtit_test_ctx ctx = {
        .tmr = tmr,
        .events = 0,
        .tid = rtems_task_self()
    };
    assert(tmr);

    sc = hpsc_rti_timer_start(tmr, handle_event, &ctx);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_RTI_TIMER_TEST_START;

    rc = do_test_advance(&ctx);
    if (rc == HPSC_RTI_TIMER_TEST_SUCCESS)
        rc = do_test_events(&ctx);

    // Since there's no way to disable the RTI timer, the best we can do to
    // reduce the load in the HW emulator, set the interval to max.
    hpsc_rti_timer_configure(tmr, reset_interval_ns);

    sc = hpsc_rti_timer_stop(tmr, handle_event, &ctx);
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
