#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <rtems.h>

// drivers
#include <hpsc-mbox.h>

#include "hpsc-mbox-test.h"

#define TEST_MSG "HPSC MBOX TEST MSG"
#define TEST_TIMEOUT_TICKS 1000

#define MBOX_EVENT_RECV RTEMS_EVENT_0
#define MBOX_EVENT_ACK  RTEMS_EVENT_1

struct hpsc_mbox_test {
    struct hpsc_mbox *mbox;
    unsigned instance;
    rtems_id tid;
    int rx_rc;
    int ack_rc;
};

static void cb_recv(void *arg)
{
    size_t sz;
    uint8_t buf[HPSC_MBOX_DATA_SIZE] = { 0 };
    struct hpsc_mbox_test *ctx = (struct hpsc_mbox_test *)arg;
    assert(ctx);
    assert(ctx->mbox);
    if (ctx->rx_rc == HPSC_MBOX_TEST_CHAN_NO_RECV) {
        sz = hpsc_mbox_chan_read(ctx->mbox, ctx->instance, buf, sizeof(buf));
        hpsc_mbox_chan_event_clear_rcv(ctx->mbox, ctx->instance);
        hpsc_mbox_chan_event_set_ack(ctx->mbox, ctx->instance);
        if (sz == HPSC_MBOX_DATA_SIZE)
            ctx->rx_rc = strncmp((const char *)buf, TEST_MSG, sizeof(buf)) ?
                HPSC_MBOX_TEST_CHAN_BAD_RECV : HPSC_MBOX_TEST_SUCCESS;
        else
            ctx->rx_rc = HPSC_MBOX_TEST_CHAN_BAD_RECV;
    } else {
        ctx->rx_rc = HPSC_MBOX_TEST_CHAN_MULTIPLE_RECV;
    }
    rtems_event_send(ctx->tid, MBOX_EVENT_RECV);
}

static void cb_ack(void *arg)
{
    struct hpsc_mbox_test *ctx = (struct hpsc_mbox_test *)arg;
    assert(ctx);
    hpsc_mbox_chan_event_clear_ack(ctx->mbox, ctx->instance);
    if (ctx->ack_rc == HPSC_MBOX_TEST_CHAN_NO_ACK)
        ctx->ack_rc = HPSC_MBOX_TEST_SUCCESS;
    else
        ctx->ack_rc = HPSC_MBOX_TEST_CHAN_MULTIPLE_ACK;
    rtems_event_send(ctx->tid, MBOX_EVENT_ACK);
}

static int test_loopback(struct hpsc_mbox_test *ctx)
{
    rtems_event_set events = 0;
    size_t sz = hpsc_mbox_chan_write(ctx->mbox, ctx->instance, TEST_MSG,
                                     sizeof(TEST_MSG));
    hpsc_mbox_chan_event_set_rcv(ctx->mbox, ctx->instance);
    if (sz < sizeof(TEST_MSG))
        return HPSC_MBOX_TEST_CHAN_WRITE;
    // wait for test to complete or timeout
    rtems_event_receive(MBOX_EVENT_RECV | MBOX_EVENT_ACK, RTEMS_EVENT_ALL,
                        TEST_TIMEOUT_TICKS, &events);
    if (ctx->rx_rc != HPSC_MBOX_TEST_SUCCESS)
        return ctx->rx_rc;
    if (ctx->ack_rc != HPSC_MBOX_TEST_SUCCESS)
        return ctx->ack_rc;
    return HPSC_MBOX_TEST_SUCCESS;
}

int hpsc_mbox_chan_test(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint8_t owner,
    uint8_t src,
    uint8_t dest
)
{
    int rc;
    rtems_status_code sc;
    struct hpsc_mbox_test ctx = {
        .mbox = mbox,
        .instance = instance,
        .tid = rtems_task_self(),
        .rx_rc = HPSC_MBOX_TEST_CHAN_NO_RECV,
        .ack_rc = HPSC_MBOX_TEST_CHAN_NO_ACK
    };
    sc = hpsc_mbox_chan_claim(mbox, instance, owner, src, dest, cb_recv, cb_ack,
                              &ctx);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_MBOX_TEST_CHAN_CLAIM;

    rc = test_loopback(&ctx);

    sc = hpsc_mbox_chan_release(mbox, instance);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_MBOX_TEST_CHAN_RELEASE;
    return rc;
}

int hpsc_mbox_test(
    uintptr_t base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b,
    unsigned instance,
    uint8_t owner,
    uint8_t src,
    uint8_t dest
)
{
    struct hpsc_mbox *mbox;
    rtems_status_code sc;
    int rc;

    sc = hpsc_mbox_probe(&mbox, "HPSC TEST MAILBOX", base,
                         int_a, int_idx_a, int_b, int_idx_b);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_MBOX_TEST_PROBE;

    rc = hpsc_mbox_chan_test(mbox, instance, owner, src, dest);

    sc = hpsc_mbox_remove(mbox);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_MBOX_TEST_REMOVE;
    return rc;
}
