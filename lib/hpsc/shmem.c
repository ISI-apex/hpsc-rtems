#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "shmem.h"

struct shmem {
    volatile void *addr;
};

static volatile void *vmem_set(volatile void *s, int c, unsigned n)
{
    volatile uint8_t *bs = s;
    while (n--)
        *bs++ = (unsigned char) c;
    return s;
}

static volatile void *vmem_cpy(volatile void *restrict dest, void *restrict src,
                               unsigned n)
{
    // assume dest and src are word-aligned
    volatile uint32_t *wd;
    uint32_t *ws;
    volatile uint8_t *bd;
    uint8_t *bs;
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
    for (bd = (uint8_t *) wd, bs = (uint8_t *) ws; n > 0; n--)
        *bd++ = *bs++;
    return dest;
}

struct shmem *shmem_open(volatile void *addr)
{
    struct shmem *s = malloc(sizeof(struct shmem));
    if (s)
        s->addr = addr;
    return s;
}

void shmem_close(struct shmem *s)
{
    free(s);
}

size_t shmem_send(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    size_t sz_rem = SHMEM_MSG_SIZE - sz;
    assert(sz <= SHMEM_MSG_SIZE);
    vmem_cpy(shm->data, msg, sz);
    if (sz_rem)
        vmem_set(shm->data + sz, 0, sz_rem);
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_NEW;
    return sz;
}

uint32_t shmem_get_status(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status;
}

bool shmem_is_new(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status & HPSC_SHMEM_STATUS_BIT_NEW;
}

bool shmem_is_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    return shm->status & HPSC_SHMEM_STATUS_BIT_ACK;
}

void shmem_clear_ack(struct shmem *s)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    shm->status &= ~HPSC_SHMEM_STATUS_BIT_ACK;
}

size_t shmem_recv(struct shmem *s, void *msg, size_t sz)
{
    volatile struct hpsc_shmem_region *shm = (volatile struct hpsc_shmem_region *) s->addr;
    assert(sz >= SHMEM_MSG_SIZE);
    mem_vcpy(msg, shm->data, SHMEM_MSG_SIZE);
    shm->status = shm->status & ~HPSC_SHMEM_STATUS_BIT_NEW; // clear new
    shm->status = shm->status | HPSC_SHMEM_STATUS_BIT_ACK; // set ACK
    return SHMEM_MSG_SIZE;
}
