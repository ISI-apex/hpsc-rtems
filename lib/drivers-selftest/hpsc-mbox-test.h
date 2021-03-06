#ifndef HPSC_MBOX_TEST_H
#define HPSC_MBOX_TEST_H

#include <stdint.h>

#include <rtems.h>

// drivers
#include <hpsc-mbox.h>

enum hpsc_mbox_test_rc {
    HPSC_MBOX_TEST_SUCCESS = 0,
    HPSC_MBOX_TEST_PROBE,
    HPSC_MBOX_TEST_REMOVE,
    HPSC_MBOX_TEST_CHAN_CLAIM,
    HPSC_MBOX_TEST_CHAN_RELEASE,
    HPSC_MBOX_TEST_CHAN_WRITE,
    HPSC_MBOX_TEST_CHAN_NO_RECV,
    HPSC_MBOX_TEST_CHAN_NO_ACK,
    HPSC_MBOX_TEST_CHAN_BAD_RECV,
    HPSC_MBOX_TEST_CHAN_MULTIPLE_RECV,
    HPSC_MBOX_TEST_CHAN_MULTIPLE_ACK
};

int hpsc_mbox_chan_test(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint8_t owner,
    uint8_t src,
    uint8_t dest
);

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
);

#endif // HPSC_MBOX_TEST_H
