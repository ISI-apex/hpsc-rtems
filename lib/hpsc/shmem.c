#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>

#include "shmem.h"

struct shmem {
    volatile struct hpsc_shmem_region *shm;
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
        s->shm = (volatile struct hpsc_shmem_region *)addr;
    return s;
}

void shmem_close(struct shmem *s)
{
    assert(s);
    free(s);
}

size_t shmem_write(struct shmem *s, const void *msg, size_t sz)
{
    const size_t sz_rem = HPSC_MSG_SIZE - sz;
    assert(s);
    assert(msg);
    assert(sz <= HPSC_MSG_SIZE);
    // don't care about volatile qualifier when writing
    memcpy(RTEMS_DEVOLATILE(void *, s->shm->data), msg, sz);
    if (sz_rem)
        memset(RTEMS_DEVOLATILE(void *, s->shm->data + sz), 0, sz_rem);
    return sz;
}

size_t shmem_read(struct shmem *s, void *msg, size_t sz)
{
    assert(s);
    assert(msg);
    assert(sz >= HPSC_MSG_SIZE);
    mem_vcpy(msg, s->shm->data, HPSC_MSG_SIZE);
    return HPSC_MSG_SIZE;
}

uint32_t shmem_get_status(struct shmem *s)
{
    assert(s);
    return s->shm->status;
}

bool shmem_is_new(struct shmem *s)
{
    assert(s);
    return s->shm->status & HPSC_SHMEM_STATUS_BIT_NEW;
}

bool shmem_is_ack(struct shmem *s)
{
    assert(s);
    return s->shm->status & HPSC_SHMEM_STATUS_BIT_ACK;
}

void shmem_set_new(struct shmem *s, bool val)
{
    assert(s);
    if (val)
        s->shm->status |= HPSC_SHMEM_STATUS_BIT_NEW;
    else
        s->shm->status &= ~HPSC_SHMEM_STATUS_BIT_NEW;
}

void shmem_set_ack(struct shmem *s, bool val)
{
    assert(s);
    if (val)
        s->shm->status |= HPSC_SHMEM_STATUS_BIT_ACK;
    else
        s->shm->status &= ~HPSC_SHMEM_STATUS_BIT_ACK;
}
