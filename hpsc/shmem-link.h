#ifndef SHMEM_LINK_H
#define SHMEM_LINK_H

#include "link.h"

struct link *shmem_link_connect(const char *name, void *addr_out, void *addr_in);

#endif // SHMEM_LINK_H
