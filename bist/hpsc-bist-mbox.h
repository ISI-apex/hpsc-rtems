#ifndef HPSC_BIST_MBOX_H
#define HPSC_BIST_MBOX_H

#include <stdint.h>

// drivers
#include <hpsc-mbox.h>

enum hpsc_bist_mbox_rc {
    HPSC_BIST_MBOX_SUCCESS = 0,
    HPSC_BIST_MBOX_PROBE,
    HPSC_BIST_MBOX_REMOVE,
    HPSC_BIST_MBOX_CHAN_CLAIM,
    HPSC_BIST_MBOX_CHAN_RELEASE,
    HPSC_BIST_MBOX_CHAN_WRITE,
    HPSC_BIST_MBOX_CHAN_NO_RECV,
    HPSC_BIST_MBOX_CHAN_NO_ACK,
    HPSC_BIST_MBOX_CHAN_BAD_RECV
};

int hpsc_bist_mbox_chan(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
);

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
);

#endif // HPSC_BIST_MBOX_H
