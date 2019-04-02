#ifndef HPSC_MBOX_H
#define HPSC_MBOX_H

#include <stdbool.h>
#include <stdint.h>

#include <rtems.h>

#define HPSC_MBOX_DATA_REGS 16
#define HPSC_MBOX_DATA_SIZE (HPSC_MBOX_DATA_REGS * 4)

#define HPSC_MBOX_CHANNELS 32

typedef void (*hpsc_mbox_chan_irq_cb)(void *);

/**
 * A mailbox device (IP block instance)
 */
struct hpsc_mbox;

/**
 * A channel (up at HPSC_MBOX_CHANNELS instances per device)
 */
struct hpsc_mbox_chan;

/**
 * Initialize a mailbox IP block
 */
rtems_status_code hpsc_mbox_probe(
    struct hpsc_mbox **mbox,
    const char *info,
    volatile void *base,
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
 * Read a mailbox channel configuration.
 */
void hpsc_mbox_chan_config_read(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t *owner,
    uint32_t *src,
    uint32_t *dest
);

/**
 * Write a mailbox channel configuration.
 * This is performed automatically by hpsc_mbox_chan_claim when owner is set,
 * and should not usually be used directly.
 */
int hpsc_mbox_chan_config_write(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
);

/**
 * Reset (release) a mailbox channel configuration.
 * This is performed automatically by hpsc_mbox_chan_release when owner is set,
 * and should not usually be used directly.
 */
void hpsc_mbox_chan_reset(struct hpsc_mbox *mbox, unsigned instance);

/**
 * Claim a mailbox channel
 */
struct hpsc_mbox_chan *hpsc_mbox_chan_claim(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest,
    hpsc_mbox_chan_irq_cb cb_a,
    hpsc_mbox_chan_irq_cb cb_b,
    void *cb_arg
);

/**
 * Release a mailbox channel
 */
int hpsc_mbox_chan_release(struct hpsc_mbox_chan *chan);

/**
 * Write to a mailbox channel (send a message)
 */
size_t hpsc_mbox_chan_write(struct hpsc_mbox_chan *chan, void *buf, size_t sz);

/**
 * Read from a mailbox channel (receive a message)
 */
size_t hpsc_mbox_chan_read(struct hpsc_mbox_chan *chan, void *buf, size_t sz);

#endif // HPSC_MBOX_H
