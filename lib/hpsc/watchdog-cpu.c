#include <assert.h>
#include <stdbool.h>

#include <rtems.h>
#include <rtems/score/percpudata.h>

// drivers
#include <hpsc-wdt.h>

#include "watchdog-cpu.h"

#define WDT_TASK_EXIT RTEMS_EVENT_0

struct watchdog_task_ctx {
    struct hpsc_wdt *wdt;
    rtems_id tid;
    rtems_interval ticks;
    bool running;
};

static PER_CPU_DATA_ITEM(struct watchdog_task_ctx, tasks) = { 0 };

static rtems_task watchdog_task(rtems_task_argument arg)
{
    struct watchdog_task_ctx *ctx = (struct watchdog_task_ctx *)arg;
    rtems_event_set events;
    assert(ctx);
    while (1) {
        hpsc_wdt_kick(ctx->wdt);
        rtems_event_receive(WDT_TASK_EXIT, RTEMS_EVENT_ANY, ctx->ticks,
                            &events);
        if (events & WDT_TASK_EXIT)
            break;
    }
    ctx->running = false;
    rtems_task_exit();
}

#define PER_CPU_WATCHDOG_TASK_CTX \
    PER_CPU_DATA_GET(_Per_CPU_Get_by_index(rtems_get_current_processor()), \
                     struct watchdog_task_ctx, tasks)

rtems_status_code watchdog_cpu_task_start(
    struct hpsc_wdt *wdt,
    rtems_id task_id,
    rtems_interval ticks,
    rtems_interrupt_handler cb,
    void *cb_arg
)
{
    struct watchdog_task_ctx *ctx;
    rtems_status_code sc;
    assert(wdt);
    assert(task_id != RTEMS_ID_NONE);
    assert(cb);

    ctx = PER_CPU_WATCHDOG_TASK_CTX;
    assert(ctx);
    assert(!ctx->running);

    ctx->wdt = wdt;
    ctx->tid = task_id;
    ctx->ticks = ticks;
    ctx->running = true;

    // once enabled, the WDT can't be stopped
    sc = hpsc_wdt_enable(wdt, cb, cb_arg);
    if (sc == RTEMS_SUCCESSFUL) 
        sc = rtems_task_start(task_id, watchdog_task, (rtems_task_argument)ctx);
    if (sc != RTEMS_SUCCESSFUL)
        ctx->running = false;

    return sc;
}

rtems_status_code watchdog_cpu_task_stop(void)
{
    struct watchdog_task_ctx *ctx;
    rtems_status_code sc = RTEMS_NOT_DEFINED;

    ctx = PER_CPU_WATCHDOG_TASK_CTX;
    assert(ctx);

    if (ctx->running) {
        sc = rtems_event_send(ctx->tid, WDT_TASK_EXIT);
        if (sc == RTEMS_SUCCESSFUL) {
            while (ctx->running) // wait for task to finish
                rtems_task_wake_after(RTEMS_YIELD_PROCESSOR);
        }
    }

    return sc;
}
