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
    rtems_vector_number int_b
)
{
    size_t i;
    mbox->info = info;
    mbox->base = base;
    mbox->int_a.mbox = mbox;
    mbox->int_a.n = int_a;
    mbox->int_b.mbox = mbox;
    mbox->int_b.n = int_b;
    for (i = 0; i < RTEMS_ARRAY_SIZE(mbox->chans); i++) {
        mbox->chans[i].mbox = mbox;
        mbox->chans[i].instance = i;
        // other fields set when channel is claimed, cleared on release
    }
}

rtems_status_code hpsc_mbox_probe(
    struct hpsc_mbox *mbox,
    const char *info,
    volatile void *base,
    rtems_vector_number int_a,
    rtems_vector_number int_b
)
{
    rtems_status_code sc;

    // init struct
    hpsc_mbox_init(mbox, info, base, int_a, int_b);

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
