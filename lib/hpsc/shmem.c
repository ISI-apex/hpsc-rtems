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

static volatile void *vmem_set(volatile void *s, int c, unsigned n)
{
    volatile uint8_t *bs = s;
    while (n--)
        *bs++ = (unsigned char) c;
    return s;
}

static volatile void *vmem_cpy(volatile void *restrict dest,
                               const void *restrict src, unsigned n)
{
    // assume dest and src are word-aligned
    volatile uint32_t *wd;
    const uint32_t *ws;
    volatile uint8_t *bd;
    const uint8_t *bs;
    for (wd = dest, ws = src; n >= sizeof(*wd); n -= sizeof(*wd))
        *wd++ = *ws++;
    for (bd = (uint8_t *) wd, bs = (uint8_t *) ws; n > 0; n--)
        *bd++ = *bs++;
    return dest;
}

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

#define IS_ALIGNED(p) (((uintptr_t)(const void *)(p) % sizeof(uint32_t)) == 0)

struct shmem *shmem_open(uintptr_t addr)
{
    struct shmem *s = malloc(sizeof(struct shmem));
    assert(IS_ALIGNED(addr));
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
    assert(IS_ALIGNED(msg));
    assert(sz <= HPSC_MSG_SIZE);
    vmem_cpy(s->shm->data, msg, sz);
    if (sz_rem)
        vmem_set(s->shm->data + sz, 0, sz_rem);
    return sz;
}

size_t shmem_read(struct shmem *s, void *msg, size_t sz)
{
    assert(s);
    assert(msg);
    assert(IS_ALIGNED(msg));
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
