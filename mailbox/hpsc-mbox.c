#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <bsp.h>
#include <bsp/fatal.h>
#include <bsp/irq-generic.h>

#include "hpsc-mbox.h"

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
    if (sc != RTEMS_SUCCESSFUL)
        bsp_fatal(BSP_FATAL_INTERRUPT_INITIALIZATION);
    sc = rtems_interrupt_handler_install(int_b, info, RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_mbox_isr_b, &mbox->int_b);
    if (sc != RTEMS_SUCCESSFUL)
        bsp_fatal(BSP_FATAL_INTERRUPT_INITIALIZATION);

    return sc;
}

rtems_status_code hpsc_mbox_remove(
    struct hpsc_mbox *mbox
)
{
    rtems_status_code sc;
    printf("hpsc_mbox_remove: %s\n", mbox->info);
    sc = rtems_interrupt_handler_remove(mbox->int_a.n, hpsc_mbox_isr_a,
                                        &mbox->int_a);
    if (sc != RTEMS_SUCCESSFUL)
        bsp_fatal(BSP_FATAL_INTERRUPT_INITIALIZATION);
    sc = rtems_interrupt_handler_remove(mbox->int_b.n, hpsc_mbox_isr_b,
                                        &mbox->int_b);
    if (sc != RTEMS_SUCCESSFUL)
        bsp_fatal(BSP_FATAL_INTERRUPT_INITIALIZATION);
    return sc;
}
