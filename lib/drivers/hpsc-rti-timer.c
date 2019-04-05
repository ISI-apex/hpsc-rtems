#define DEBUG 1

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <bsp/irq-generic.h>

#include "gic-trigger.h"
#include "hpsc-rti-timer.h"
#include "regops.h"


#define REG__INTERVAL_LO       0x00
#define REG__INTERVAL_HI       0x04
#define REG__COUNT_LO           0x08
#define REG__COUNT_HI           0x0c

#define REG__CMD_ARM           0x10
#define REG__CMD_FIRE          0x14

enum cmd {
    CMD_CAPTURE = 0,
    CMD_LOAD,
    NUM_CMDS,
};

struct cmd_code {
    uint32_t arm;
    uint32_t fire;
};

// cmd -> (arm, fire)
static const struct cmd_code cmd_codes[] = {
    [CMD_CAPTURE] = { 0xCD01, 0x01CD },
    [CMD_LOAD]    = { 0xCD02, 0x02CD },
};

struct hpsc_rti_timer_cb {
    struct hpsc_rti_timer *tmr;
    hpsc_rti_timer_cb_t *cb;
    void *cb_arg;
};

struct hpsc_rti_timer {
    volatile uint32_t *base;
    const char *name;
    rtems_vector_number vec;
    // TODO: would be nice to support a list of callbacks
    struct hpsc_rti_timer_cb cb;
    bool is_started;
};

static void exec_cmd(struct hpsc_rti_timer *tmr, enum cmd cmd)
{
    // TODO: Arm-Fire is no protection from a stray jump over here...
    // Are we supposed to separate these with data-driven control flow?
    const struct cmd_code *code = &cmd_codes[cmd];
    REGB_WRITE32(tmr->base, REG__CMD_ARM,  code->arm);
    REGB_WRITE32(tmr->base, REG__CMD_FIRE, code->fire);
}

static void hpsc_rti_timer_cb_init(
    struct hpsc_rti_timer_cb *handle,
    struct hpsc_rti_timer *tmr,
    hpsc_rti_timer_cb_t *cb,
    void *cb_arg)
{
    handle->tmr = tmr;
    handle->cb = cb;
    handle->cb_arg = cb_arg;
}

static void hpsc_rti_timer_init(
    struct hpsc_rti_timer *tmr,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number vec)
{
    tmr->base = base;
    tmr->name = name;
    tmr->vec = vec;
    hpsc_rti_timer_cb_init(&tmr->cb, tmr, NULL, NULL);
    tmr->is_started = false;
}

static void hpsc_rti_timer_isr(void *arg)
{
    struct hpsc_rti_timer *tmr = (struct hpsc_rti_timer *)arg;
    assert(tmr);
    DPRINTK("RTI TMR: %s: ISR\n", tmr->name);
    if (tmr->cb.cb)
        tmr->cb.cb(tmr, tmr->cb.cb_arg);
    // Interrupt is edge-triggered, and clears HW de-asserts it on next cycle
}

rtems_status_code hpsc_rti_timer_probe(
    struct hpsc_rti_timer **tmr,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number vec
)
{
    printf("RTI TMR: %s: probe\n", name);
    printf("\tbase: %p\n", base);
    printf("\tvec: %u\n", vec);
    *tmr = malloc(sizeof(struct hpsc_rti_timer));
    if (!*tmr) {
        printf("RTI TMR: malloc failed\n");
        return RTEMS_NO_MEMORY;
    }
    hpsc_rti_timer_init(*tmr, name, base, vec);
    // must set vector to edge-triggered
    gic_trigger_set(vec, GIC_EDGE_TRIGGERED);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_remove(struct hpsc_rti_timer *tmr)
{
    assert(tmr);
    printf("RTI TMR: %s: remove\n", tmr->name);
    if (tmr->cb.cb)
        return RTEMS_RESOURCE_IN_USE;
    if (tmr->is_started)
        return RTEMS_RESOURCE_IN_USE;
    // reset vector to level-sensitive
    gic_trigger_set(tmr->vec, GIC_LEVEL_SENSITIVE);
    free(tmr);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_subscribe(struct hpsc_rti_timer *tmr,
                                           struct hpsc_rti_timer_cb **handle,
                                           hpsc_rti_timer_cb_t *cb,
                                           void *cb_arg)
{
    assert(tmr);
    assert(handle);
    assert(cb);
    DPRINTK("RTI TMR: %s: subscribe\n", tmr->name);
    if (tmr->cb.cb)
        return RTEMS_NOT_OWNER_OF_RESOURCE;
    *handle = &tmr->cb;
    hpsc_rti_timer_cb_init(*handle, tmr, cb, cb_arg);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_unsubscribe(struct hpsc_rti_timer_cb *handle)
{
    assert(handle);
    assert(handle->tmr);
    DPRINTK("RTI TMR: %s: unsubscribe\n", handle->tmr->name);
    hpsc_rti_timer_cb_init(handle, handle->tmr, NULL, NULL);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_start(struct hpsc_rti_timer *tmr)
{
    rtems_status_code sc;
    assert(tmr);
    DPRINTK("RTI TMR: %s: start\n", tmr->name);
    sc = rtems_interrupt_handler_install(tmr->vec, tmr->name,
                                         RTEMS_INTERRUPT_UNIQUE,
                                         hpsc_rti_timer_isr, tmr);
    if (sc == RTEMS_SUCCESSFUL)
        tmr->is_started = true;
    else
        printk("RTI TMR: %s: failed to install interrupt handler\n", tmr->name);
    return sc;
}

rtems_status_code hpsc_rti_timer_stop(struct hpsc_rti_timer *tmr)
{
    rtems_status_code sc;
    assert(tmr);
    DPRINTK("RTI TMR: %s: stop\n", tmr->name);
    sc = rtems_interrupt_handler_remove(tmr->vec, hpsc_rti_timer_isr, tmr);
    if (sc == RTEMS_SUCCESSFUL)
        tmr->is_started = false;
    else
        printk("RTI TMR: %s: failed to remove interrupt handler\n", tmr->name);
    return sc;
}

uint64_t hpsc_rti_timer_capture(struct hpsc_rti_timer *tmr)
{
    assert(tmr);
    exec_cmd(tmr, CMD_CAPTURE);
    uint64_t count = ((uint64_t)REGB_READ32(tmr->base, REG__COUNT_HI) << 32) |
                     REGB_READ32(tmr->base, REG__COUNT_LO);
    DPRINTK("RTI TMR: %s: capture: count -> 0x%08x%08x\n", tmr->name,
            (uint32_t)(count >> 32), (uint32_t)count);
    return count;
}

void hpsc_rti_timer_configure(struct hpsc_rti_timer *tmr, uint64_t interval)
{
    assert(tmr);
    REGB_WRITE32(tmr->base, REG__INTERVAL_HI, interval >> 32);
    REGB_WRITE32(tmr->base, REG__INTERVAL_LO, interval & 0xffffffff);
    DPRINTK("RTI TMR: %s: configure: interval <- 0x%08x%08x\n", tmr->name,
            (uint32_t)(interval >> 32), (uint32_t)interval);
    exec_cmd(tmr, CMD_LOAD);
}
