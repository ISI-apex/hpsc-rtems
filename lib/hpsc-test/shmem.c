#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// libhpsc
#include <shmem.h>

#include "hpsc-test.h"

static int do_test(struct shmem *shm)
{
    const char *msg = "Test Message";
    char buf[HPSC_SHMEM_REGION_SZ] = {0};
    size_t sz;
    uint32_t status = shmem_get_status(shm);
    // no flags should be set at this point
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\n");
        return 1;
    }
    sz = shmem_write(shm, msg, sizeof(msg));
    if (sz != sizeof(msg)) {
        printf("ERROR: TEST: shmem: write failed\n");
        return 1;
    }
    // NEW flag should be set
    if (!shmem_is_new(shm)) {
        printf("ERROR: TEST: shmem: intermediate status failed\n");
        return 1;
    }
    sz = shmem_read(shm, buf, sizeof(buf));
    if (sz != HPSC_MSG_SIZE) {
        printf("ERROR: TEST: shmem: read failed\n");
        return 1;
    }
    // NEW flag should be off, ACK flag should be set
    if (shmem_is_new(shm) || !shmem_is_ack(shm)) {
        printf("ERROR: TEST: shmem: final status failed\n");
        return 1;
    }
    shmem_clear_ack(shm);
    // no flags should be set anymore
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\n");
        return 1;
    }
    return 0;
}

int hpsc_test_shmem(void)
{
    // a dummy shared memory region
    uint8_t shmem_reg[HPSC_SHMEM_REGION_SZ] = {0};
    struct shmem *shm;
    int rc;

    shm = shmem_open((uintptr_t) &shmem_reg);
    if (!shm)
        return 1;
    rc = do_test(shm);
    shmem_close(shm);
    return rc;
}
