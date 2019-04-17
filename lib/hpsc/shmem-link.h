#ifndef SHMEM_LINK_H
#define SHMEM_LINK_H

#include <stdbool.h>

#include <rtems.h>

#include "link.h"

struct link *shmem_link_connect(
    const char* name,
    void *addr_out,
    void *addr_in,
    bool is_server,
    unsigned long poll_us,
    rtems_name tname_recv,
    rtems_name tname_ack
);

#endif // SHMEM_LINK_H
