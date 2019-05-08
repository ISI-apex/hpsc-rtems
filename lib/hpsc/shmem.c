#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>

#include "shmem.h"

#define SHMEM_TO_REGION(s) ((volatile struct hpsc_shmem_region *) (s)->addr)

struct shmem {
    uintptr_t addr;
};

static void *mem_vcpy(void *restrict dest, volatile void *restrict src,
                      unsigned n)
{
    // assume dest and src are word-aligned
    uint32_t *wd;
    volatile uint32_t *ws;
    uint8_t *bd;
    volatile uint8_t *bs;
    for (wd = dest, ws = src; n >= sizeof(*wd); n -= sizeof(*wd))
        *wd++ = *ws++;
    for (bd = (uint8_t *) wd, bs = (volatile uint8_t *) ws; n > 0; n--)
        *bd++ = *bs++;
    return dest;
}

struct shmem *shmem_open(uintptr_t addr)
{
    struct shmem *s = malloc(sizeof(struct shmem));
    if (s)
        s->addr = addr;
    return s;
}

void shmem_close(struct shmem *s)
{
    assert(s);
    free(s);
}

size_t shmem_write(struct shmem *s, const void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm;
    size_t sz_rem = HPSC_MSG_SIZE - sz;
    assert(s);
    assert(msg);
    assert(sz <= HPSC_MSG_SIZE);
    shm = SHMEM_TO_REGION(s);
    // don't care about volatile qualifier when writing
    memcpy(RTEMS_DEVOLATILE(void *, shm->data), msg, sz);
    if (sz_rem)
        memset(RTEMS_DEVOLATILE(void *, shm->data + sz), 0, sz_rem);
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_NEW;
    return sz;
}

uint32_t shmem_get_status(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm;
    assert(s);
    shm = SHMEM_TO_REGION(s);
    return shm->status;
}

bool shmem_is_new(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm;
    assert(s);
    shm = SHMEM_TO_REGION(s);
    return shm->status & HPSC_SHMEM_STATUS_BIT_NEW;
}

bool shmem_is_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm;
    assert(s);
    shm = SHMEM_TO_REGION(s);
    return shm->status & HPSC_SHMEM_STATUS_BIT_ACK;
}

void shmem_clear_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm;
    assert(s);
    shm = SHMEM_TO_REGION(s);
    shm->status &= ~HPSC_SHMEM_STATUS_BIT_ACK;
}

size_t shmem_read(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm;
    assert(s);
    assert(msg);
    assert(sz >= HPSC_MSG_SIZE);
    shm = SHMEM_TO_REGION(s);
    mem_vcpy(msg, shm->data, HPSC_MSG_SIZE);
    shm->status = shm->status & ~HPSC_SHMEM_STATUS_BIT_NEW; // clear new
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_ACK; // set ACK
    return HPSC_MSG_SIZE;
}
