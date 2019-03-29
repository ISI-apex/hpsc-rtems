#ifndef HPSC_MBOX_H
#define HPSC_MBOX_H

#include <stdint.h>

#include <rtems.h>

#define HPSC_MBOX_DATA_REGS 16
#define HPSC_MBOX_DATA_SIZE (HPSC_MBOX_DATA_REGS * 4)

#define HPSC_MBOX_CHANNELS 32

typedef void (*hpsc_mbox_chan_irq_cb)(void *);

struct hpsc_mbox;

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
};

struct hpsc_mbox_irq_info {
    struct hpsc_mbox *mbox;
    rtems_vector_number n;
};

struct hpsc_mbox {
    struct hpsc_mbox_chan chans[HPSC_MBOX_CHANNELS];
    const char *info;
    volatile void *base;
    struct hpsc_mbox_irq_info int_a;
    struct hpsc_mbox_irq_info int_b;
};

rtems_status_code hpsc_mbox_probe(
    struct hpsc_mbox *mbox,
    const char *info,
    volatile void *base,
    rtems_vector_number int_a,
    rtems_vector_number int_b
);

rtems_status_code hpsc_mbox_remove(
    struct hpsc_mbox *mbox
);

int hpsc_mbox_chan_claim(struct hpsc_mbox_chan *chan,
                         uint32_t owner, uint32_t src, uint32_t dest,
                         hpsc_mbox_chan_irq_cb cb_a, hpsc_mbox_chan_irq_cb cb_b,
                         void *cb_arg);
int hpsc_mbox_chan_release(struct hpsc_mbox_chan *chan);

size_t hpsc_mbox_chan_write(struct hpsc_mbox_chan *chan, void *buf, size_t sz);
size_t hpsc_mbox_chan_read(struct hpsc_mbox_chan *chan, void *buf, size_t sz);

void hpsc_mbox_isr_a(void *arg);
void hpsc_mbox_isr_b(void *arg);

#endif // HPSC_MBOX_H
