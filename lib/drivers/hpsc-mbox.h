#ifndef HPSC_MBOX_H
#define HPSC_MBOX_H

#include <stdint.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

#define HPSC_MBOX_DATA_REGS 16
#define HPSC_MBOX_DATA_SIZE (HPSC_MBOX_DATA_REGS * 4)

#define HPSC_MBOX_CHANNELS 32

// Users must synchronize channel claims and releases between tasks/threads.
// E.g., if a higher priority task preempts a lower priority task, and both are
// opening or closing the same channel, the tasks may deadlock.

/**
 * A mailbox device (IP block instance)
 */
struct hpsc_mbox;

/**
 * Initialize a mailbox IP block.
 * May not be called from an interrupt context.
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
 * Teardown a mailbox IP block.
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_mbox_remove(struct hpsc_mbox *mbox);

/**
 * Claim a mailbox channel
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_mbox_chan_claim(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint8_t owner,
    uint8_t src,
    uint8_t dest,
    rtems_interrupt_handler cb_a,
    rtems_interrupt_handler cb_b,
    void *cb_arg
);

/**
 * Release a mailbox channel
 * May not be called from an interrupt context.
 */
rtems_status_code hpsc_mbox_chan_release(
    struct hpsc_mbox *mbox,
    unsigned instance
);

/* 
 * Data read/write is kept separate from event set/clear b/c correct event
 * management ordering cannot be guaranteed by mbox_send/mbox_read and the ISRs
 * in the driver, without a lot of assumptions about callback routine behavior
 * and how mailboxes are used in general---and it's not pretty.
 * Higher-level abstractions can make event handling opaque, if desired.
 *
 * In the case of polled operation (not currently supported), event handling can
 * be performed at any time.
 * In the case where events drive IRQs (this design), events should be cleared
 * before ISRs complete, o/w the IRQ remains active and the ISR runs again.
 *
 * The driver _could_ enforce correct event handling by performing the read in
 * the driver's RCV ISR and passing the data pointer to the `cb_a` routine.
 * This requires either that `cb_a` handles the message before returning, or
 * that it copies the message somewhere more persistent, in which case there is
 * a wasteful data copy. Instead, we allow the next level up to read directly
 * into whatever buffer is desired, but require that it manage the events.
 */

/*
 * A note on correct usage:
 * The RCV event _must_ be cleared _before_ ACK is set, otherwise the sender may
 * receive the ACK and send a new message before the RCV event is actually
 * cleared, in which case the second message is missed.
 */

/**
 * Write to a mailbox channel (send a message)
 */
size_t hpsc_mbox_chan_write(
    struct hpsc_mbox *mbox,
    unsigned instance,
    const void *buf,
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

/**
 * Set the RCV event (after write)
 */
void hpsc_mbox_chan_event_set_rcv(struct hpsc_mbox *mbox, unsigned instance);

/**
 * Set the ACK event (after read, after clearing RCV)
 */
void hpsc_mbox_chan_event_set_ack(struct hpsc_mbox *mbox, unsigned instance);

/**
 * Clear the RCV event (after read, before setting ACK)
 */
void hpsc_mbox_chan_event_clear_rcv(struct hpsc_mbox *mbox, unsigned instance);

/**
 * Clear the ACK event
 */
void hpsc_mbox_chan_event_clear_ack(struct hpsc_mbox *mbox, unsigned instance);

#endif // HPSC_MBOX_H
