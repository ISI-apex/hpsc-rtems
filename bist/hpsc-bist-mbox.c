#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <rtems.h>

// drivers
#include <hpsc-mbox.h>

#include "hpsc-bist-mbox.h"

#define TEST_MSG "HPSC BIST TEST MSG"

struct hpsc_bist {
    struct hpsc_mbox_chan *chan;
    int recv_status;
    bool is_recv;
    bool is_ack;
};

static void cb_recv(void *arg)
{
    size_t sz;
    uint8_t buf[HPSC_MBOX_DATA_SIZE] = { 0 };
    struct hpsc_bist *bist = (struct hpsc_bist *)arg;
    assert(bist);
    assert(bist->chan);
    assert(!bist->is_recv);

    sz = hpsc_mbox_chan_read(bist->chan, buf, sizeof(buf));
    if (sz == HPSC_MBOX_DATA_SIZE)
        bist->recv_status = strncmp((const char *)buf, TEST_MSG, sizeof(buf)) ?
            HPSC_BIST_MBOX_CHAN_BAD_RECV : HPSC_BIST_MBOX_SUCCESS;
    else
        bist->recv_status = HPSC_BIST_MBOX_CHAN_BAD_RECV;
    bist->is_recv = true;
}

static void cb_ack(void *arg)
{
    struct hpsc_bist *bist = (struct hpsc_bist *)arg;
    assert(bist);
    assert(!bist->is_ack);
    bist->is_ack = true;
}

static int test_loopback(struct hpsc_bist *bist)
{
    struct timespec ts;
    size_t sz = hpsc_mbox_chan_write(bist->chan, TEST_MSG, sizeof(TEST_MSG));
    if (sz < sizeof(TEST_MSG))
        return HPSC_BIST_MBOX_CHAN_WRITE;
    // TODO: wait a short period - may need to handle being interrupted...
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
    if (!bist->is_recv)
        return HPSC_BIST_MBOX_CHAN_NO_RECV;
    if (!bist->is_ack)
        return HPSC_BIST_MBOX_CHAN_NO_ACK;
    return HPSC_BIST_MBOX_SUCCESS;
}

int hpsc_bist_mbox_chan(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
)
{
    int rc;
    struct hpsc_bist bist = {
        .chan = NULL,
        .is_recv = false,
        .recv_status = HPSC_BIST_MBOX_CHAN_NO_RECV,
        .is_ack = false
    };
    bist.chan = hpsc_mbox_chan_claim(mbox, instance, owner, src, dest,
                                     cb_recv, cb_ack, &bist);
    if (!bist.chan)
        return HPSC_BIST_MBOX_CHAN_CLAIM;

    rc = test_loopback(&bist);

    if (hpsc_mbox_chan_release(bist.chan))
        return HPSC_BIST_MBOX_CHAN_RELEASE;
    return rc;
}

int hpsc_bist_mbox(
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

    sc = hpsc_mbox_probe(&mbox, "HPSC BIST Mailbox", base,
                         int_a, int_idx_a, int_b, int_idx_b);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_BIST_MBOX_PROBE;

    rc = hpsc_bist_mbox_chan(mbox, instance, owner, src, dest);

    sc = hpsc_mbox_remove(mbox);
    if (sc != RTEMS_SUCCESSFUL)
        return HPSC_BIST_MBOX_REMOVE;
    return rc;
}
