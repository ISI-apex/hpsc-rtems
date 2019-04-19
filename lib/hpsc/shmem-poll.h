#ifndef SHMEM_POLL_H
#define SHMEM_POLL_H

#include <stdbool.h>
#include <stdint.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

#include "shmem.h"

struct shmem_poll;

rtems_status_code shmem_poll_task_start(
    struct shmem_poll **sp,
    struct shmem *shm,
    rtems_interval poll_ticks,
    uint32_t status_mask,
    rtems_id task_id,
    rtems_interrupt_handler cb,
    void *cb_arg
);

rtems_status_code shmem_poll_task_destroy(struct shmem_poll *sp);

#endif // SHMEM_POLL_H
