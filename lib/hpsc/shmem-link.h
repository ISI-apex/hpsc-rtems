#ifndef SHMEM_LINK_H
#define SHMEM_LINK_H

#include <stdbool.h>

#include <rtems.h>

#include "link.h"

struct link *shmem_link_connect(
    const char* name,
    volatile void *addr_out,
    volatile void *addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_id tid_recv,
    rtems_id tid_ack
);

#endif // SHMEM_LINK_H
