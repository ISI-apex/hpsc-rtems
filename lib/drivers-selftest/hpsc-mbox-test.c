#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <rtems.h>

// drivers
#include <hpsc-mbox.h>

#include "hpsc-mbox-test.h"

#define TEST_MSG "HPSC MBOX TEST MSG"

struct hpsc_mbox_test {
    struct hpsc_mbox *mbox;
    unsigned instance;
    int rx_rc;
    bool is_rx;
    bool is_ack;
};

static void cb_recv(void *arg)
{
    size_t sz;
    uint8_t buf[HPSC_MBOX_DATA_SIZE] = { 0 };
    struct hpsc_mbox_test *ctx = (struct hpsc_mbox_test *)arg;
    assert(ctx);
    assert(ctx->mbox);
    assert(!ctx->is_rx);

    sz = hpsc_mbox_chan_read(ctx->mbox, ctx->instance, buf, sizeof(buf));
    if (sz == HPSC_MBOX_DATA_SIZE)
        ctx->rx_rc = strncmp((const char *)buf, TEST_MSG, sizeof(buf)) ?
            HPSC_MBOX_TEST_CHAN_BAD_RECV : HPSC_MBOX_TEST_SUCCESS;
    else
        ctx->rx_rc = HPSC_MBOX_TEST_CHAN_BAD_RECV;
    ctx->is_rx = true;
}

static void cb_ack(void *arg)
{
    struct hpsc_mbox_test *ctx = (struct hpsc_mbox_test *)arg;
    assert(ctx);
    assert(!ctx->is_ack);
    ctx->is_ack = true;
}

static int test_loopback(struct hpsc_mbox_test *ctx)
{
    size_t sz = hpsc_mbox_chan_write(ctx->mbox, ctx->instance, TEST_MSG,
                                     sizeof(TEST_MSG));
    if (sz < sizeof(TEST_MSG))
        return HPSC_MBOX_TEST_CHAN_WRITE;
    // wait a short period
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(10));
    if (!ctx->is_rx)
        return HPSC_MBOX_TEST_CHAN_NO_RECV;
    if (!ctx->is_ack)
        return HPSC_MBOX_TEST_CHAN_NO_ACK;
    return HPSC_MBOX_TEST_SUCCESS;
}

int hpsc_mbox_chan_test(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
)
{
    int rc;
    rtems_status_code sc;
    struct hpsc_mbox_test ctx = {
        .mbox = mbox,
        .instance = instance,
        .is_rx = false,
        .rx_rc = HPSC_MBOX_TEST_CHAN_NO_RECV,
        .is_ack = false
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
    volatile void *base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
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
