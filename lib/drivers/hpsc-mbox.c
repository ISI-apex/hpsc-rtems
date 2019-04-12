#define DEBUG 0

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

#include "debug.h"
#include "hpsc-mbox.h"
#include "regops.h"

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
    rtems_interrupt_handler cb;
    void *arg;
};

struct hpsc_mbox_chan {
    // static fields
    struct hpsc_mbox *mbox;
    volatile uint32_t *base;
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
                                rtems_interrupt_handler cb_a,
                                rtems_interrupt_handler cb_b,
                                void *cb_arg)
{
    chan->base = (volatile uint32_t *)((uint8_t *)chan->mbox->base +
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

void hpsc_mbox_chan_config_read(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t *owner,
    uint32_t *src,
    uint32_t *dest
)
{
    uint32_t val;
    assert(mbox);
    assert(instance < HPSC_MBOX_CHANNELS);

    DPRINTK("MBOX: %s: %u: read config\n", mbox->info, instance);
    val = REGB_READ32(mbox->chans[instance].base, REG_CONFIG);
    if (owner)
        *owner = (val & REG_CONFIG__OWNER__MASK) >> REG_CONFIG__OWNER__SHIFT;
    if (src)
        *src =  (val & REG_CONFIG__SRC__MASK) >> REG_CONFIG__SRC__SHIFT;
    if (dest)
        *dest = (val & REG_CONFIG__DEST__MASK) >> REG_CONFIG__DEST__SHIFT;
}

rtems_status_code hpsc_mbox_chan_config_write(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest
)
{
    uint32_t config;
    uint32_t val;
    assert(mbox);
    assert(instance < HPSC_MBOX_CHANNELS);

    config = REG_CONFIG__UNSECURE |
             ((owner << REG_CONFIG__OWNER__SHIFT) & REG_CONFIG__OWNER__MASK) |
             ((src << REG_CONFIG__SRC__SHIFT)     & REG_CONFIG__SRC__MASK) |
             ((dest  << REG_CONFIG__DEST__SHIFT)  & REG_CONFIG__DEST__MASK);
    DPRINTK("MBOX: %s: %u: write config\n", mbox->info, instance);
    REGB_WRITE32(mbox->chans[instance].base, REG_CONFIG, config);
    val = REGB_READ32(mbox->chans[instance].base, REG_CONFIG);
    if (val != config) {
        printk("hpsc_mbox_chan_config_write: failed to write chan %u for %x: "
               "already owned by %x\n", instance, owner,
               (val & REG_CONFIG__OWNER__MASK) >> REG_CONFIG__OWNER__SHIFT);
        return RTEMS_NOT_OWNER_OF_RESOURCE;
    }
    return RTEMS_SUCCESSFUL;
}

void hpsc_mbox_chan_reset(struct hpsc_mbox *mbox, unsigned instance)
{
    assert(mbox);
    assert(instance < HPSC_MBOX_CHANNELS);
    // clearing owner also clears destination (resets the instance)
    DPRINTK("MBOX: %s: %u: reset config\n", mbox->info, instance);
    REGB_WRITE32(mbox->chans[instance].base, REG_CONFIG, 0);
}

rtems_status_code hpsc_mbox_chan_claim(
    struct hpsc_mbox *mbox,
    unsigned instance,
    uint32_t owner,
    uint32_t src,
    uint32_t dest,
    rtems_interrupt_handler cb_a,
    rtems_interrupt_handler cb_b,
    void *cb_arg
)
{
    uint32_t val = 0;
    uint32_t src_hw;
    uint32_t dest_hw;
    struct hpsc_mbox_chan *chan;
    rtems_status_code sc;
    assert(mbox);
    assert(instance < HPSC_MBOX_CHANNELS);

    // NOTE: we don't verify that channels aren't already claimed
    DPRINTK("MBOX: %s: %u: claim\n", mbox->info, instance);
    chan = &mbox->chans[instance];
    hpsc_mbox_chan_init(chan, owner, src, dest, cb_a, cb_b, cb_arg);

    if (chan->owner) {
        sc = hpsc_mbox_chan_config_write(mbox, instance, owner, src, dest);
        if (sc != RTEMS_SUCCESSFUL)
            goto cleanup;
    } else { // not owner, just check the value in registers against the requested value
        hpsc_mbox_chan_config_read(mbox, instance, NULL, &src_hw, &dest_hw);
        if (cb_b && src && src_hw != src) {
            printk("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "src mismatch: %x (expected %x)\n",
                   chan->instance, src, src_hw);
            sc = RTEMS_UNSATISFIED;
            goto cleanup;
        }
        if (cb_a && dest && dest_hw != src) {
            printk("hpsc_mbox_chan_claim: failed to claim mailbox %u: "
                   "dest mismatch: %x (expected %x)\n",
                   chan->instance, dest, dest_hw);
            sc = RTEMS_UNSATISFIED;
            goto cleanup;
        }
    }

    if (chan->int_a.cb)
        val |= HPSC_MBOX_INT_A(chan->mbox->int_a.idx);
    if (chan->int_b.cb)
        val |= HPSC_MBOX_INT_B(chan->mbox->int_b.idx);
    DPRINTK("MBOX: %s: %u: enable interrupts\n", mbox->info, instance);
    REGB_SET32(chan->base, REG_INT_ENABLE, val);

    return RTEMS_SUCCESSFUL;
cleanup:
    hpsc_mbox_chan_destroy(chan);
    return sc;
}

rtems_status_code hpsc_mbox_chan_release(
    struct hpsc_mbox *mbox,
    unsigned instance
)
{
    struct hpsc_mbox_chan *chan;
    uint32_t val;
    assert(instance < HPSC_MBOX_CHANNELS);
    chan = &mbox->chans[instance];
    DPRINTK("MBOX: %s: %u: release\n", mbox->info, instance);
    val = HPSC_MBOX_INT_A(mbox->int_a.idx) |
          HPSC_MBOX_INT_B(mbox->int_b.idx);
    REGB_CLEAR32(chan->base, REG_INT_ENABLE, val);
    if (chan->owner) {
        // We are the OWNER, so we can release
        hpsc_mbox_chan_reset(mbox, instance);
    }
    hpsc_mbox_chan_destroy(chan);
    return RTEMS_SUCCESSFUL;
}

size_t hpsc_mbox_chan_write(
    struct hpsc_mbox *mbox,
    unsigned instance,
    void *buf,
    size_t sz
)
{
    struct hpsc_mbox_chan *chan;
    uint32_t *msg = buf;
    size_t len;
    size_t i;
    assert(instance < HPSC_MBOX_CHANNELS);
    assert(buf);
    assert(sz <= HPSC_MBOX_DATA_SIZE);
    chan = &mbox->chans[instance];

    len = sz / sizeof(uint32_t);
    if (sz % sizeof(uint32_t))
        len++;

    DPRINTK("MBOX: %s: %u: write\n", mbox->info, instance);
    for (i = 0; i < len; ++i)
        REGB_WRITE32(chan->base, REG_DATA + (i * sizeof(uint32_t)), msg[i]);
    // zero out any remaining registers
    for (; i < HPSC_MBOX_DATA_REGS; i++)
        REGB_WRITE32(chan->base, REG_DATA + (i * sizeof(uint32_t)), 0);

    printk("MBOX: %s: %u: raise int A\n", mbox->info, instance);
    REGB_WRITE32(chan->base, REG_EVENT_SET, HPSC_MBOX_EVENT_A);

    return sz;
}

size_t hpsc_mbox_chan_read(
    struct hpsc_mbox *mbox,
    unsigned instance,
    void *buf,
    size_t sz
)
{
    struct hpsc_mbox_chan *chan;
    uint32_t *msg = buf;
    size_t len;
    size_t i;
    assert(instance < HPSC_MBOX_CHANNELS);
    assert(buf);
    // assert(sz >= HPSC_MBOX_DATA_SIZE); // not a strict requirement
    chan = &mbox->chans[instance];

    len = sz / sizeof(uint32_t);
    if (sz % sizeof(uint32_t))
        len++;

    DPRINTK("MBOX: %s: %u: read\n", mbox->info, instance);
    for (i = 0; i < len && i < HPSC_MBOX_DATA_REGS; i++)
        msg[i] = REGB_READ32(chan->base, REG_DATA + (i * sizeof(uint32_t)));

    // ACK
    printk("MBOX: %s: %u: raise int B\n", mbox->info, instance);
    REGB_WRITE32(chan->base, REG_EVENT_SET, HPSC_MBOX_EVENT_B);

    return i * sizeof(uint32_t);
}

static void hpsc_mbox_chan_isr_a(struct hpsc_mbox_chan *chan)
{
    assert(chan);
    // Clear the event first
    printk("MBOX: %s: %u: clear int A\n", chan->mbox->info, chan->instance);
    REGB_WRITE32(chan->base, REG_EVENT_CLEAR, HPSC_MBOX_EVENT_A);
    // issue callback
    if (chan->int_a.cb)
        chan->int_a.cb(chan->int_a.arg);
}

static void hpsc_mbox_chan_isr_b(struct hpsc_mbox_chan *chan)
{
    assert(chan);
    // Clear the event first
    printk("MBOX: %s: %u: clear int B\n", chan->mbox->info, chan->instance);
    REGB_WRITE32(chan->base, REG_EVENT_CLEAR, HPSC_MBOX_EVENT_B);
    // issue callback
    if (chan->int_b.cb)
        chan->int_b.cb(chan->int_b.arg);
}

static bool hpsc_mbox_chan_is_subscribed(struct hpsc_mbox_chan *chan,
                                         unsigned event,
                                         unsigned interrupt)
{
    uint32_t val;
    if (!chan->active)
        return false;
    DPRINTK("MBOX: %s: %u: check event subscription: %u\n",
            chan->mbox->info, chan->instance, event);
    // Are we 'signed up' for this event (A) from this channel?
    // Two criteria: (1) Cause is set, and (2) Mapped to our IRQ
    val = REGB_READ32(chan->base, REG_EVENT_CAUSE);
    if (!(val & event))
        return false; // this mailbox didn't raise the interrupt
    val = REGB_READ32(chan->base, REG_INT_ENABLE);
    if (!(val & interrupt))
        return false; // this mailbox has an event but it's not ours
    return true;
}

static void hpsc_mbox_isr(struct hpsc_mbox *mbox, unsigned event,
                          unsigned interrupt,
                          void (*cb)(struct hpsc_mbox_chan *))
{
    struct hpsc_mbox_chan *chan;
    size_t i;
    bool handled = false;
    assert(mbox);
    assert(cb);
    for (i = 0; i < HPSC_MBOX_CHANNELS; ++i) {
        chan = &mbox->chans[i];
        if (!hpsc_mbox_chan_is_subscribed(chan, event, interrupt))
            continue;
        handled = true;
        cb(chan);
   }
   assert(handled);
}

static void hpsc_mbox_isr_a(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    assert(info);
    DPRINTK("MBOX: %s: ISR A\n", info->mbox->info);
    hpsc_mbox_isr(info->mbox, HPSC_MBOX_EVENT_A,
                  HPSC_MBOX_INT_A(info->mbox->int_a.idx), hpsc_mbox_chan_isr_a);
}
static void hpsc_mbox_isr_b(void *arg)
{
    struct hpsc_mbox_irq_info *info = (struct hpsc_mbox_irq_info *)arg;
    assert(info);
    DPRINTK("MBOX: %s: ISR B\n", info->mbox->info);
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
    printf("\tbase: %p\n", base);
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
    struct hpsc_mbox **mbox,
    const char *info,
    volatile void *base,
    rtems_vector_number int_a,
    unsigned int_idx_a,
    rtems_vector_number int_b,
    unsigned int_idx_b
)
{
    rtems_status_code sc;
    assert(mbox);
    assert(info);
    assert(base);

    printf("MBOX: %s: probe\n", info);
    *mbox = malloc(sizeof(struct hpsc_mbox));
    if (!*mbox) {
        printf("hpsc_mbox_probe: malloc failed\n");
        return RTEMS_NO_MEMORY;
    }

    // init struct
    hpsc_mbox_init(*mbox, info, base, int_a, int_idx_a, int_b, int_idx_b);

    // setup interrupt handlers
    sc = rtems_interrupt_handler_install(int_a, info, RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_mbox_isr_a, &(*mbox)->int_a);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_probe: failed to install interrupt handler A\n");
        goto free_mbox;
    }
    sc = rtems_interrupt_handler_install(int_b, info, RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_mbox_isr_b, &(*mbox)->int_b);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("hpsc_mbox_probe: failed to install interrupt handler B\n");
        goto fail_isr_b;
    }

    return sc;
fail_isr_b:
    rtems_interrupt_handler_remove((*mbox)->int_a.n, hpsc_mbox_isr_a,
                                   &(*mbox)->int_a);
free_mbox:
    free(*mbox);
    return sc;
}

rtems_status_code hpsc_mbox_remove(struct hpsc_mbox *mbox)
{
    rtems_status_code sc = RTEMS_SUCCESSFUL;
    rtems_status_code sc_tmp;
    size_t i;
    assert(mbox);

    printf("MBOX: %s: remove\n", mbox->info);

    for (i = 0; i < RTEMS_ARRAY_SIZE(mbox->chans); i++)
        hpsc_mbox_chan_release(mbox, i);

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
    free(mbox);
    return sc;
}
