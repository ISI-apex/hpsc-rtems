#ifndef SHMEM_H
#define SHMEM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hpsc-msg.h"

// All subsystems must understand this structure and its protocol
#define HPSC_SHMEM_STATUS_BIT_NEW 0x01
#define HPSC_SHMEM_STATUS_BIT_ACK 0x02
struct hpsc_shmem_region {
    uint8_t data[HPSC_MSG_SIZE];
    uint32_t status;
};

#define HPSC_SHMEM_REGION_SZ sizeof(struct hpsc_shmem_region)

struct shmem;

/**
 * Open a shared memory region.
 */
struct shmem *shmem_open(volatile void *addr);

/**
 * Close a shared memory region.
 */
void shmem_close(struct shmem *s);

/**
 * Write data to the shared memory region.
 * Automatically sets the NEW status bit.
 * Returns the number of bytes written
 */
size_t shmem_write(struct shmem *s, void *msg, size_t sz);

/**
 * Read data from the shared memory region.
 * Automatically clears the NEW status bit and sets the ACK status bit.
 * Returns the number of bytes read
 */
size_t shmem_read(struct shmem *s, void *msg, size_t sz);

/**
 * Read the entire status field so it can be parsed directly.
 */
uint32_t shmem_get_status(struct shmem *s);

/**
 * Returns true if the NEW status bit is set, false otherwise.
 */
bool shmem_is_new(struct shmem *s);

/**
 * Returns true if the ACK status bit is set, false otherwise.
 */
bool shmem_is_ack(struct shmem *s);

/**
 * Clears the ACK status bit.
 */
void shmem_clear_ack(struct shmem *s);

#endif // SHMEM_H
