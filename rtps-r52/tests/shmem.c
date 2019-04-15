#include <stdint.h>
#include <stdio.h>

// libhpsc
#include <shmem.h>

#include "test.h"

static char *msg = "Test Message";
static char buf[HPSC_SHMEM_REGION_SZ] = {0};
// a dummy shared memory region
static char shmem_reg[HPSC_SHMEM_REGION_SZ] = {0};

int test_shmem()
{
    size_t sz;
    int ret = 0;
    uint32_t status;
    struct shmem *shm = shmem_open(&shmem_reg);
    if (!shm)
        return 1;
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\n");
        ret = 1;
        goto out;
    }
    sz = shmem_send(shm, msg, sizeof(msg));
    if (sz != sizeof(msg)) {
        printf("ERROR: TEST: shmem: send failed\n");
        ret = 1;
        goto out;
    }
    status = shmem_get_status(shm);
    if (!status) {
        printf("ERROR: TEST: shmem: intermediate status failed\n");
        ret = 1;
        goto out;
    }
    sz = shmem_recv(shm, buf, sizeof(buf));
    if (sz != SHMEM_MSG_SIZE) {
        printf("ERROR: TEST: shmem: recv failed\n");
        ret = 1;
        goto out;
    }
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: final status failed\n");
        ret = 1;
        goto out;
    }
    printf("TEST: shmem: success\n");

out:
    shmem_close(shm);
    return ret;
}
