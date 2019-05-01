#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

#include "shmem.h"
#include "shmem-poll.h"

struct shmem_poll {
    struct shmem *shm;
    rtems_interval poll_ticks;
    rtems_interrupt_handler cb;
    void *cb_arg;
    rtems_id tid;
    bool running;
    uint32_t status_mask;
};

static rtems_task shm_poll_task(rtems_task_argument arg)
{
    struct shmem_poll *sp = (struct shmem_poll *)arg;
    rtems_event_set events;
    uint32_t status;
    // polling for a change in status
    while (1) {
        events = 0;
        rtems_event_receive(RTEMS_EVENT_0, RTEMS_EVENT_ANY, sp->poll_ticks,
                            &events);
        if (events & RTEMS_EVENT_0)
            break; // we've been ordered to exit
        status = shmem_get_status(sp->shm);
        status &= sp->status_mask;
        if (status)
            sp->cb(sp->cb_arg);
    }
    sp->running = false;
    rtems_task_exit();
}

static void shmem_poll_init(
    struct shmem_poll *sp,
    struct shmem *shm,
    rtems_interval poll_ticks,
    uint32_t status_mask,
    rtems_id task_id,
    rtems_interrupt_handler cb,
    void *cb_arg
)
{
    sp->shm = shm;
    sp->poll_ticks = poll_ticks;
    sp->status_mask = status_mask;
    sp->cb = cb;
    sp->cb_arg = cb_arg;
    sp->tid = task_id;
    sp->running = true;
}

rtems_status_code shmem_poll_task_start(
    struct shmem_poll **sp,
    struct shmem *shm,
    rtems_interval poll_ticks,
    uint32_t status_mask,
    rtems_id task_id,
    rtems_interrupt_handler cb,
    void *cb_arg
)
{
    rtems_status_code sc;
    assert(sp);
    assert(shm);
    assert(poll_ticks);
    assert(cb);
    *sp = malloc(sizeof(struct shmem_poll));
    if (!*sp)
        return RTEMS_NO_MEMORY;
    shmem_poll_init(*sp, shm, poll_ticks, status_mask, task_id, cb, cb_arg);
    sc = rtems_task_start(task_id, shm_poll_task, (rtems_task_argument) *sp);
    if (sc != RTEMS_SUCCESSFUL)
        goto free_sp;
    return RTEMS_SUCCESSFUL;

free_sp:
    free(*sp);
    *sp = NULL;
    return sc;
}

rtems_status_code shmem_poll_task_destroy(struct shmem_poll *sp)
{
    rtems_status_code sc;
    assert(sp);
    sc = rtems_event_send(sp->tid, RTEMS_EVENT_0);
    if (sc == RTEMS_SUCCESSFUL) {
        assert(sp->tid != rtems_task_self());
        while (sp->running) // wait for task to complete
            rtems_task_wake_after(RTEMS_YIELD_PROCESSOR);
        free(sp);
    }
    return sc;
}
