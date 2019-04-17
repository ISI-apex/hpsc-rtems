#ifndef SHMEM_POLL_H
#define SHMEM_POLL_H

#include <stdbool.h>

#include <rtems.h>
#include <rtems/irq-extension.h>

#include "shmem.h"

struct shmem_poll;

rtems_status_code shmem_poll_task_create(
    struct shmem_poll **sp,
    struct shmem *shm,
    unsigned long poll_us,
    uint32_t status_mask,
    rtems_name tname,
    rtems_interrupt_handler cb,
    void *cb_arg
);

rtems_status_code shmem_poll_task_destroy(struct shmem_poll *sp);

#endif // SHMEM_POLL_H
