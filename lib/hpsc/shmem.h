#ifndef SHMEM_H
#define SHMEM_H

#include <stdint.h>
#include <unistd.h>

#include "hpsc-msg.h"

#define SHMEM_MSG_SIZE HPSC_MSG_SIZE

// All subsystems must understand this structure and its protocol
struct hpsc_shmem_region {
    uint8_t data[SHMEM_MSG_SIZE];
    uint32_t is_new;
};

#define HPSC_SHMEM_REGION_SZ sizeof(struct hpsc_shmem_region)

struct shmem;

struct shmem *shmem_open(void *addr);

void shmem_close(struct shmem *s);

// returns the number of bytes written
size_t shmem_send(struct shmem *s, void *msg, size_t sz);

uint32_t shmem_get_status(struct shmem *s);

// returns 0the number of bytes read
size_t shmem_recv(struct shmem *s, void *msg, size_t sz);

#endif // SHMEM_H
