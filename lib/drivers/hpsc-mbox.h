#ifndef HPSC_MBOX_H
#define HPSC_MBOX_H

#include <stdint.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

#define HPSC_MBOX_DATA_REGS 16
#define HPSC_MBOX_DATA_SIZE (HPSC_MBOX_DATA_REGS * 4)

#define HPSC_MBOX_CHANNELS 32

/**
 * A mailbox device (IP block instance)
 */
struct hpsc_mbox;

/**
 * Initialize a mailbox IP block
 */
rtems_status_code hpsc_mbox_probe(
    struct hpsc_mbox **mbox,
    const char *info,
    uintptr_t base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b
);

/**
 * Teardown a mailbox IP block
 */
rtems_status_code hpsc_mbox_remove(struct hpsc_mbox *mbox);

/**
 * Claim a mailbox channel
 */
rtems_status_code hpsc_mbox_chan_claim(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest,
    rtems_interrupt_handler cb_a,
    rtems_interrupt_handler cb_b,
    void *cb_arg
);

/**
 * Release a mailbox channel
 */
rtems_status_code hpsc_mbox_chan_release(
    struct hpsc_mbox *mbox,
    unsigned instance
);

/**
 * Write to a mailbox channel (send a message)
 */
size_t hpsc_mbox_chan_write(
    struct hpsc_mbox *mbox,
    unsigned instance,
    void *buf,
    size_t sz
);

/**
 * Read from a mailbox channel (receive a message)
 */
size_t hpsc_mbox_chan_read(
    struct hpsc_mbox *mbox,
    unsigned instance,
    void *buf,
    size_t sz
);

#endif // HPSC_MBOX_H
