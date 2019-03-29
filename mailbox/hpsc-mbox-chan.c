#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hpsc-mbox.h"

#define REG_CONFIG              0x00
#define REG_EVENT_CAUSE         0x04
#define REG_EVENT_CLEAR         0x04
#define REG_EVENT_STATUS        0x08
#define REG_EVENT_SET           0x08
#define REG_INT_ENABLE          0x0C
#define REG_DATA                0x10

#define REG_CONFIG__UNSECURE      0x1
#define REG_CONFIG__OWNER__SHIFT  8
#define REG_CONFIG__OWNER__MASK   0x0000ff00
#define REG_CONFIG__SRC__SHIFT    16
#define REG_CONFIG__SRC__MASK     0x00ff0000
#define REG_CONFIG__DEST__SHIFT   24
#define REG_CONFIG__DEST__MASK    0xff000000

#define HPSC_MBOX_EVENT_A 0x1
#define HPSC_MBOX_EVENT_B 0x2
#define HPSC_MBOX_EVENT_C 0x4

#define HPSC_MBOX_INT_A(idx) (1 << (2 * (idx)))      // rcv (map event A to int 'idx')
#define HPSC_MBOX_INT_B(idx) (1 << (2 * (idx) + 1))  // ack (map event B to int 'idx')

#define HPSC_MBOX_EVENTS 2
#define HPSC_MBOX_INTS   16
#define HPSC_MBOX_INSTANCE_REGION (REG_DATA + HPSC_MBOX_DATA_SIZE)


static void hpsc_mbox_chan_init(struct hpsc_mbox_chan *chan,
                                uint32_t owner, uint32_t src, uint32_t dest,
                                hpsc_mbox_chan_irq_cb cb_a,
                                hpsc_mbox_chan_irq_cb cb_b,
                                void *cb_arg)
{
    chan->base = (volatile void *)((uint8_t *)chan->mbox->base +
                                   chan->instance * HPSC_MBOX_INSTANCE_REGION);
    chan->int_a.cb = cb_a;
    chan->int_a.arg = cb_arg;
    chan->int_b.cb = cb_b;
    chan->int_b.arg = cb_arg;
    chan->owner = owner;
    chan->src = src;
    chan->dest = dest;
}

static void hpsc_mbox_chan_destroy(struct hpsc_mbox_chan *chan)
{
    chan->int_a.cb = NULL;
    chan->int_a.arg = NULL;
    chan->int_b.cb = NULL;
    chan->int_b.arg = NULL;
    chan->owner = 0;
    chan->src = 0;
    chan->dest = 0;
}

int hpsc_mbox_chan_claim(struct hpsc_mbox_chan *chan,
                         uint32_t owner, uint32_t src, uint32_t dest,
                         hpsc_mbox_chan_irq_cb cb_a, hpsc_mbox_chan_irq_cb cb_b,
                         void *cb_arg)
{
    volatile uint32_t *addr;
    uint32_t config;
    uint32_t val;
    uint32_t src_hw;
    uint32_t dest_hw;

    hpsc_mbox_chan_init(chan, owner, src, dest, cb_a, cb_b, cb_arg);

    if (chan->owner) {
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        config = REG_CONFIG__UNSECURE |
                 ((chan->owner << REG_CONFIG__OWNER__SHIFT) & REG_CONFIG__OWNER__MASK) |
                 ((chan->src << REG_CONFIG__SRC__SHIFT)     & REG_CONFIG__SRC__MASK) |
                 ((chan->dest  << REG_CONFIG__DEST__SHIFT)  & REG_CONFIG__DEST__MASK);
        val = config;
        printf("hpsc_mbox_chan_claim: config: %p <|- %08x\n", addr, val);
        *addr = val;
        val = *addr;
        printf("hpsc_mbox_chan_claim: config: %p -> %08x\n", addr, val);
        if (val != config) {
            printf("hpsc_mbox_chan_claim: failed to claim mailbox %u for %x: "
                   "already owned by %x\n", chan->instance, chan->owner,
                   (val & REG_CONFIG__OWNER__MASK) >> REG_CONFIG__OWNER__SHIFT);
            goto cleanup;
        }
    } else { // not owner, just check the value in registers against the requested value
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        val = *addr;
        printf("hpsc_mbox_chan_claim: config: %p -> %08x\n", addr, val);
        src_hw =  (val & REG_CONFIG__SRC__MASK) >> REG_CONFIG__SRC__SHIFT;
        dest_hw = (val & REG_CONFIG__DEST__MASK) >> REG_CONFIG__DEST__SHIFT;
        if (cb_b && src && src_hw != src) {
            printf("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "src mismatch: %x (expected %x)\n",
                   chan->instance, src, src_hw);
            goto cleanup;
        }
        if (cb_a && dest && dest_hw != src) {
            printf("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "dest mismatch: %x (expected %x)\n",
                   chan->instance, dest, dest_hw);
            goto cleanup;
        }
    }

    val = 0;
    if (chan->int_a.cb)
        val |= HPSC_MBOX_INT_A(chan->mbox->int_a.n);
    if (chan->int_b.cb)
        val |= HPSC_MBOX_INT_B(chan->mbox->int_b.n);

    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_INT_ENABLE);
    printf("mbox_claim: int en: %p <|- %08x\n", addr, val);
    *addr |= val;

    return 0;
cleanup:
    hpsc_mbox_chan_destroy(chan);
    return -1;
}

int hpsc_mbox_chan_release(struct hpsc_mbox_chan *chan)
{
    // We are the OWNER, so we can release
    volatile uint32_t *addr;
    uint32_t val = 0;
    if (chan->owner) {
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        printf("hpsc_mbox_chan_release: owner: %p <|- %08x\n", addr, val);
        *addr = val;
        // clearing owner also clears destination (resets the instance)
    }
    hpsc_mbox_chan_destroy(chan);
    return 0;
}

size_t hpsc_mbox_chan_write(struct hpsc_mbox_chan *chan, void *buf, size_t sz)
{
    unsigned i;
    uint32_t *msg = buf;
    unsigned len = sz / sizeof(uint32_t);
    volatile uint32_t *addr;
    uint32_t val;

    assert(sz <= HPSC_MBOX_DATA_SIZE);
    if (sz % sizeof(uint32_t))
        len++;

    printf("hpsc_mbox_chan_write: msg: ");
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_DATA);
    for (i = 0; i < len; ++i) {
        addr[i] = msg[i];
        printf("%x ", msg[i]);
    }
    printf("\n");
    // zero out any remaining registers
    for (; i < HPSC_MBOX_DATA_REGS; i++)
        addr[i] = 0;

    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_SET);
    val = HPSC_MBOX_EVENT_A;
    printf("hpsc_mbox_chan_write: raise int A: %p <- %08x\n", addr, val);
    *addr = val;

    return sz;
}

size_t hpsc_mbox_chan_read(struct hpsc_mbox_chan *chan, void *buf, size_t sz)
{
    uint32_t *msg = buf;
    volatile uint32_t *data = (volatile uint32_t *)((uint8_t *)chan->base + REG_DATA);
    size_t len = sz / sizeof(uint32_t);
    volatile uint32_t *addr;
    uint32_t val;
    size_t i;
    assert(sz >= HPSC_MBOX_DATA_SIZE);

    if (sz % sizeof(uint32_t))
        len++;

    printf("hpsc_mbox_chan_read: msg: ");
    for (i = 0; i < len && i < HPSC_MBOX_DATA_REGS; i++) {
        msg[i] = *data++;
        printf("%x ", msg[i]);
    }
    printf("\n");

    // ACK
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_SET);
    val = HPSC_MBOX_EVENT_B;
    printf("hpsc_mbox_chan_read: set int B: %p <- %08x\n", addr, val);
    *addr = val;

    return i * sizeof(uint32_t);
}

static void hpsc_mbox_chan_isr_a(struct hpsc_mbox_chan *chan)
{
    volatile uint32_t *addr;
    uint32_t val;

    printf("hpsc_mbox_chan_isr_a: base %p instance %u\n", chan->base, chan->instance);

    // Clear the event
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_CLEAR);
    val = HPSC_MBOX_EVENT_A;
    printf("hpsc_mbox_chan_isr_a: clear int A: %p <- %08x\n", addr, val);
    *addr = val;

    if (chan->int_a.cb)
        chan->int_a.cb(chan->int_a.arg);
}

static void hpsc_mbox_chan_isr_b(struct hpsc_mbox_chan *chan)
{
    volatile uint32_t *addr;
    uint32_t val;

    printf("hpsc_mbox_chan_isr_b: base %p instance %u\n", chan->base, chan->instance);

    // Clear the event first
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_CLEAR);
    val = HPSC_MBOX_EVENT_B;
    printf("hpsc_mbox_chan_isr_b: clear int B: %p <- %08x\n", addr, val);
    *addr = val;

    if (chan->int_b.cb)
        chan->int_b.cb(chan->int_b.arg);
}

static void hpsc_mbox_isr(struct hpsc_mbox *mbox, unsigned event,
                         unsigned interrupt,
                         void (*cb)(struct hpsc_mbox_chan *))
{
    struct hpsc_mbox_chan *chan;
    volatile uint32_t *addr;
    uint32_t val;
    unsigned i;
    bool handled = false;

    for (i = 0; i < HPSC_MBOX_CHANNELS; ++i) {
        chan = &mbox->chans[i];
        // Are we 'signed up' for this event (A) from this mailbox (i)?
        // Two criteria: (1) Cause is set, and (2) Mapped to our IRQ
        addr = (volatile uint32_t *)((uint8_t *)mbox->base + REG_EVENT_CAUSE);
        val = *addr;
        printf("hpsc_mbox_isr: cause: %p -> %08x\n", addr, val);
        if (!(val & event))
            continue; // this mailbox didn't raise the interrupt
        addr = (volatile uint32_t *)((uint8_t *)mbox->base + REG_INT_ENABLE);
        val = *addr;
        printf("hpsc_mbox_isr: int enable: %p -> %08x\n", addr, val);
        if (!(val & interrupt))
            continue; // this mailbox has an event but it's not ours

        handled = true;
        cb(chan);
   }
   assert(handled);
}

void hpsc_mbox_isr_a(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    hpsc_mbox_isr(info->mbox, HPSC_MBOX_EVENT_A,
                  HPSC_MBOX_INT_A(info->mbox->int_a.n), hpsc_mbox_chan_isr_a);
}
void hpsc_mbox_isr_b(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    hpsc_mbox_isr(info->mbox, HPSC_MBOX_EVENT_B,
                  HPSC_MBOX_INT_B(info->mbox->int_b.n), hpsc_mbox_chan_isr_b);
}
