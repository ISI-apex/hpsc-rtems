#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

#include "gic-trigger.h"
#include "hpsc-rti-timer.h"

#ifdef HPSC_RTIT_DEBUG
#define HPSC_RTIT_DBG(...) printk(__VA_ARGS__)
#else
#define HPSC_RTIT_DBG(...)
#endif


struct hpsc_rtit_base {
    uint64_t INTERVAL;
    uint64_t COUNT;
    uint32_t CMD_ARM;
    uint32_t CMD_FIRE;
};

struct hpsc_rti_timer {
    volatile struct hpsc_rtit_base *base;
    const char *name;
    rtems_vector_number vec;
    bool is_started;
};

enum cmd {
    CMD_CAPTURE = 0,
    CMD_LOAD,
    NUM_CMDS,
};

struct cmd_code {
    uint32_t arm;
    uint32_t fire;
};

static const struct cmd_code cmd_codes[] = {
    [CMD_CAPTURE] = { 0xCD01, 0x01CD },
    [CMD_LOAD]    = { 0xCD02, 0x02CD },
};

static void exec_cmd(struct hpsc_rti_timer *tmr, enum cmd cmd)
{
    tmr->base->CMD_ARM = cmd_codes[cmd].arm;
    tmr->base->CMD_FIRE = cmd_codes[cmd].fire;
}

static void hpsc_rti_timer_init(
    struct hpsc_rti_timer *tmr,
    const char *name,
    uintptr_t base,
    rtems_vector_number vec)
{
    tmr->base = (volatile struct hpsc_rtit_base *)base;
    tmr->name = name;
    tmr->vec = vec;
    tmr->is_started = false;
}

rtems_status_code hpsc_rti_timer_probe(
    struct hpsc_rti_timer **tmr,
    const char *name,
    uintptr_t base,
    rtems_vector_number vec
)
{
    HPSC_RTIT_DBG("RTIT: %s: probe\n", name);
    HPSC_RTIT_DBG("\tbase: 0x%"PRIxPTR"\n", base);
    HPSC_RTIT_DBG("\tvec: %u\n", vec);
    *tmr = malloc(sizeof(struct hpsc_rti_timer));
    if (!*tmr)
        return RTEMS_NO_MEMORY;
    hpsc_rti_timer_init(*tmr, name, base, vec);
    // must set vector to edge-triggered
    gic_trigger_set(vec, GIC_EDGE_TRIGGERED);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_remove(struct hpsc_rti_timer *tmr)
{
    assert(tmr);
    HPSC_RTIT_DBG("RTIT: %s: remove\n", tmr->name);
    if (tmr->is_started)
        return RTEMS_RESOURCE_IN_USE;
    // reset vector to level-sensitive
    gic_trigger_set(tmr->vec, GIC_LEVEL_SENSITIVE);
    free(tmr);
    return RTEMS_SUCCESSFUL;
}

rtems_status_code hpsc_rti_timer_start(
    struct hpsc_rti_timer *tmr,
    rtems_interrupt_handler handler,
    void *arg
)
{
    rtems_status_code sc;
    assert(tmr);
    HPSC_RTIT_DBG("RTIT: %s: start\n", tmr->name);
    sc = rtems_interrupt_handler_install(tmr->vec, tmr->name,
                                         RTEMS_INTERRUPT_UNIQUE, handler, arg);
    if (sc == RTEMS_SUCCESSFUL)
        tmr->is_started = true;
    return sc;
}

rtems_status_code hpsc_rti_timer_stop(
    struct hpsc_rti_timer *tmr,
    rtems_interrupt_handler handler,
    void *arg
)
{
    rtems_status_code sc;
    assert(tmr);
    HPSC_RTIT_DBG("RTIT: %s: stop\n", tmr->name);
    sc = rtems_interrupt_handler_remove(tmr->vec, handler, arg);
    if (sc == RTEMS_SUCCESSFUL)
        tmr->is_started = false;
    return sc;
}

uint64_t hpsc_rti_timer_capture(struct hpsc_rti_timer *tmr)
{
    uint64_t count;
    assert(tmr);
    exec_cmd(tmr, CMD_CAPTURE);
    count = tmr->base->COUNT;
    HPSC_RTIT_DBG("RTIT: %s: capture: count -> 0x%016"PRIx64"\n",
                  tmr->name, count);
    return count;
}

void hpsc_rti_timer_configure(struct hpsc_rti_timer *tmr, uint64_t interval)
{
    assert(tmr);
    HPSC_RTIT_DBG("RTIT: %s: configure: interval <- 0x%016"PRIx64"\n",
                  tmr->name, interval);
    tmr->base->INTERVAL = interval;
    exec_cmd(tmr, CMD_LOAD);
}
