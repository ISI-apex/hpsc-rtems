#ifndef LINK_SHMEM_H
#define LINK_SHMEM_H

#include <stdbool.h>

#include <rtems.h>

#include "link.h"

struct link *link_shmem_connect(
    const char* name,
    volatile void *addr_out,
    volatile void *addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_id tid_recv,
    rtems_id tid_ack
);

#endif // LINK_SHMEM_H
