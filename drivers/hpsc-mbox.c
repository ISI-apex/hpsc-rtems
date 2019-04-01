#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <bsp.h>
#include <bsp/fatal.h>
#include <bsp/irq-generic.h>

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

struct hpsc_mbox_chan_irq_info {
    hpsc_mbox_chan_irq_cb cb;
    void *arg;
};

struct hpsc_mbox_chan {
    // static fields
    struct hpsc_mbox *mbox;
    volatile void *base;
    unsigned instance;
    // dynamic fields
    struct hpsc_mbox_chan_irq_info int_a;
    struct hpsc_mbox_chan_irq_info int_b;
    uint32_t owner;
    uint32_t src;
    uint32_t dest;
    bool active;
};

struct hpsc_mbox_irq_info {
    struct hpsc_mbox *mbox;
    rtems_vector_number n;
    unsigned idx;
};

struct hpsc_mbox {
    struct hpsc_mbox_chan chans[HPSC_MBOX_CHANNELS];
    const char *info;
    volatile void *base;
    struct hpsc_mbox_irq_info int_a;
    struct hpsc_mbox_irq_info int_b;
};


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
    chan->active = true;
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
    chan->active = false;
}

struct hpsc_mbox_chan *hpsc_mbox_chan_claim(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest,
    hpsc_mbox_chan_irq_cb cb_a,
    hpsc_mbox_chan_irq_cb cb_b,
    void *cb_arg
)
{
    volatile uint32_t *addr;
    uint32_t config;
    uint32_t val;
    uint32_t src_hw;
    uint32_t dest_hw;
    struct hpsc_mbox_chan *chan;

    // NOTE: we don't verify that channels aren't already claimed
    assert(instance < HPSC_MBOX_CHANNELS);
    chan = &mbox->chans[instance];
    hpsc_mbox_chan_init(chan, owner, src, dest, cb_a, cb_b, cb_arg);

    if (chan->owner) {
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        config = REG_CONFIG__UNSECURE |
                 ((chan->owner << REG_CONFIG__OWNER__SHIFT) & REG_CONFIG__OWNER__MASK) |
                 ((chan->src << REG_CONFIG__SRC__SHIFT)     & REG_CONFIG__SRC__MASK) |
                 ((chan->dest  << REG_CONFIG__DEST__SHIFT)  & REG_CONFIG__DEST__MASK);
        val = config;
        printk("hpsc_mbox_chan_claim: config: %p <|- %08x\n", addr, val);
        *addr = val;
        val = *addr;
        printk("hpsc_mbox_chan_claim: config: %p -> %08x\n", addr, val);
        if (val != config) {
            printk("hpsc_mbox_chan_claim: failed to claim mailbox %u for %x: "
                   "already owned by %x\n", chan->instance, chan->owner,
                   (val & REG_CONFIG__OWNER__MASK) >> REG_CONFIG__OWNER__SHIFT);
            goto cleanup;
        }
    } else { // not owner, just check the value in registers against the requested value
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        val = *addr;
        printk("hpsc_mbox_chan_claim: config: %p -> %08x\n", addr, val);
        src_hw =  (val & REG_CONFIG__SRC__MASK) >> REG_CONFIG__SRC__SHIFT;
        dest_hw = (val & REG_CONFIG__DEST__MASK) >> REG_CONFIG__DEST__SHIFT;
        if (cb_b && src && src_hw != src) {
            printk("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "src mismatch: %x (expected %x)\n",
                   chan->instance, src, src_hw);
            goto cleanup;
        }
        if (cb_a && dest && dest_hw != src) {
            printk("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "dest mismatch: %x (expected %x)\n",
                   chan->instance, dest, dest_hw);
            goto cleanup;
        }
    }

    val = 0;
    if (chan->int_a.cb)
        val |= HPSC_MBOX_INT_A(chan->mbox->int_a.idx);
    if (chan->int_b.cb)
        val |= HPSC_MBOX_INT_B(chan->mbox->int_b.idx);

    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_INT_ENABLE);
    printk("hpsc_mbox_chan_claim: int en: %p <|- %08x\n", addr, val);
    *addr |= val;

    return chan;
cleanup:
    hpsc_mbox_chan_destroy(chan);
    return NULL;
}

int hpsc_mbox_chan_release(struct hpsc_mbox_chan *chan)
{
    // We are the OWNER, so we can release
    volatile uint32_t *addr;
    uint32_t val = 0;
    if (chan->owner) {
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_CONFIG);
        printk("hpsc_mbox_chan_release: owner: %p <|- %08x\n", addr, val);
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

    printk("hpsc_mbox_chan_write: msg: ");
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_DATA);
    for (i = 0; i < len; ++i) {
        addr[i] = msg[i];
        printk("%x ", msg[i]);
    }
    printk("\n");
    // zero out any remaining registers
    for (; i < HPSC_MBOX_DATA_REGS; i++)
        addr[i] = 0;

    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_SET);
    val = HPSC_MBOX_EVENT_A;
    printk("hpsc_mbox_chan_write: raise int A: %p <- %08x\n", addr, val);
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
    // assert(sz >= HPSC_MBOX_DATA_SIZE); // doesn't always hold currently

    if (sz % sizeof(uint32_t))
        len++;

    printk("hpsc_mbox_chan_read: msg: ");
    for (i = 0; i < len && i < HPSC_MBOX_DATA_REGS; i++) {
        msg[i] = *data++;
        printk("%x ", msg[i]);
    }
    printk("\n");

    // ACK
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_SET);
    val = HPSC_MBOX_EVENT_B;
    printk("hpsc_mbox_chan_read: set int B: %p <- %08x\n", addr, val);
    *addr = val;

    return i * sizeof(uint32_t);
}

static void hpsc_mbox_chan_isr_a(struct hpsc_mbox_chan *chan)
{
    volatile uint32_t *addr;
    uint32_t val;

    printk("hpsc_mbox_chan_isr_a: base %p instance %u\n", chan->base, chan->instance);

    // Clear the event
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_CLEAR);
    val = HPSC_MBOX_EVENT_A;
    printk("hpsc_mbox_chan_isr_a: clear int A: %p <- %08x\n", addr, val);
    *addr = val;

    if (chan->int_a.cb)
        chan->int_a.cb(chan->int_a.arg);
}

static void hpsc_mbox_chan_isr_b(struct hpsc_mbox_chan *chan)
{
    volatile uint32_t *addr;
    uint32_t val;

    printk("hpsc_mbox_chan_isr_b: base %p instance %u\n", chan->base, chan->instance);

    // Clear the event first
    addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_CLEAR);
    val = HPSC_MBOX_EVENT_B;
    printk("hpsc_mbox_chan_isr_b: clear int B: %p <- %08x\n", addr, val);
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
        if (!chan->active)
            continue;
        // Are we 'signed up' for this event (A) from this mailbox (i)?
        // Two criteria: (1) Cause is set, and (2) Mapped to our IRQ
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_EVENT_CAUSE);
        val = *addr;
        printk("hpsc_mbox_isr: cause: %p -> %08x\n", addr, val);
        if (!(val & event))
            continue; // this mailbox didn't raise the interrupt
        addr = (volatile uint32_t *)((uint8_t *)chan->base + REG_INT_ENABLE);
        val = *addr;
        printk("hpsc_mbox_isr: int enable: %p -> %08x\n", addr, val);
        if (!(val & interrupt))
            continue; // this mailbox has an event but it's not ours

        handled = true;
        cb(chan);
   }
   assert(handled);
}

static void hpsc_mbox_isr_a(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    hpsc_mbox_isr(info->mbox, HPSC_MBOX_EVENT_A,
                  HPSC_MBOX_INT_A(info->mbox->int_a.idx), hpsc_mbox_chan_isr_a);
}
static void hpsc_mbox_isr_b(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    hpsc_mbox_isr(info->mbox, HPSC_MBOX_EVENT_B,
                  HPSC_MBOX_INT_B(info->mbox->int_b.idx), hpsc_mbox_chan_isr_b);
}

static void hpsc_mbox_init(
    struct hpsc_mbox *mbox,
    const char *info,
    volatile void *base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b
)
{
    size_t i;
    printf("hpsc_mbox_init: %s\n", info);
    printf("\tbase: %p\n", (volatile uint32_t *)base);
    printf("\tirq_a: %u\n", int_a);
    printf("\tidx_a: %u\n", int_idx_a);
    printf("\tirq_b: %u\n", int_b);
    printf("\tidx_b: %u\n", int_idx_b);
    mbox->info = info;
    mbox->base = base;
    mbox->int_a.mbox = mbox;
    mbox->int_a.n = int_a;
    mbox->int_a.idx = int_idx_a;
    mbox->int_b.mbox = mbox;
    mbox->int_b.n = int_b;
    mbox->int_b.idx = int_idx_b;
    for (i = 0; i < RTEMS_ARRAY_SIZE(mbox->chans); i++) {
        mbox->chans[i].mbox = mbox;
        mbox->chans[i].instance = i;
        mbox->chans[i].active = false;
        // other fields set when channel is claimed, cleared on release
    }
}

rtems_status_code hpsc_mbox_probe(
    struct hpsc_mbox *mbox,
    const char *info,
    volatile void *base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b
)
{
    rtems_status_code sc;

    // init struct
    hpsc_mbox_init(mbox, info, base, int_a, int_idx_a, int_b, int_idx_b);

    // setup interrupt handlers
    sc = rtems_interrupt_handler_install(int_a, info, RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_mbox_isr_a, &mbox->int_a);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_probe: failed to install interrupt handler A\n");
        return sc;
    }
    sc = rtems_interrupt_handler_install(int_b, info, RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_mbox_isr_b, &mbox->int_b);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_probe: failed to install interrupt handler B\n");
        goto fail_isr_b;
    }

    return sc;
fail_isr_b:
    rtems_interrupt_handler_remove(mbox->int_a.n, hpsc_mbox_isr_a,
                                   &mbox->int_a);
    return sc;
}

rtems_status_code hpsc_mbox_remove(struct hpsc_mbox *mbox)
{
    rtems_status_code sc = RTEMS_SUCCESSFUL;
    rtems_status_code sc_tmp;
    printf("hpsc_mbox_remove: %s\n", mbox->info);

    sc_tmp = rtems_interrupt_handler_remove(mbox->int_a.n, hpsc_mbox_isr_a,
                                            &mbox->int_a);
    if (sc_tmp != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_remove: failed to uninstall interrupt handler A\n");
        sc = sc_tmp;
    }
    sc_tmp = rtems_interrupt_handler_remove(mbox->int_b.n, hpsc_mbox_isr_b,
                                            &mbox->int_b);
    if (sc_tmp != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_remove: failed to uninstall interrupt handler B\n");
        sc = sc_tmp;
    }
    return sc;
}
