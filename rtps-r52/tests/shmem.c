#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// libhpsc
#include <link.h>
#include <link-store.h>
#include <shmem.h>

#include "link-names.h"
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
    // no flags should be set at this point
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\n");
        ret = 1;
        goto out;
    }
    sz = shmem_write(shm, msg, sizeof(msg));
    if (sz != sizeof(msg)) {
        printf("ERROR: TEST: shmem: write failed\n");
        ret = 1;
        goto out;
    }
    // NEW flag should be set
    if (!shmem_is_new(shm)) {
        printf("ERROR: TEST: shmem: intermediate status failed\n");
        ret = 1;
        goto out;
    }
    sz = shmem_read(shm, buf, sizeof(buf));
    if (sz != HPSC_MSG_SIZE) {
        printf("ERROR: TEST: shmem: read failed\n");
        ret = 1;
        goto out;
    }
    // NEW flag should be off, ACK flag should be set
    if (shmem_is_new(shm) || !shmem_is_ack(shm)) {
        printf("ERROR: TEST: shmem: final status failed\n");
        ret = 1;
        goto out;
    }
    shmem_clear_ack(shm);
    // no flags should be set anymore
    status = shmem_get_status(shm);
    if (status) {
        printf("ERROR: TEST: shmem: initial status failed\n");
        ret = 1;
        goto out;
    }
    printf("TEST: shmem: success\n");

out:
    shmem_close(shm);
    return ret;
}

#define WTIMEOUT_TICKS 100
#define RTIMEOUT_TICKS 100

int test_link_shmem_trch()
{
    HPSC_MSG_DEFINE(arg);
    HPSC_MSG_DEFINE(reply);
    uint32_t payload = 42;
    ssize_t rc;
    struct link *trch_link = link_store_get(LINK_NAME__SHMEM__TRCH_CLIENT);
    assert(trch_link);

    hpsc_msg_ping(arg, sizeof(arg), &payload, sizeof(payload));
    rc = link_request(trch_link,
                      WTIMEOUT_TICKS, arg, sizeof(arg),
                      RTIMEOUT_TICKS, reply, sizeof(reply));
    rc = rc <= 0 ? -1 : 0;
    printf("TEST: link_shmem_trch: %s\n", rc ? "failed" : "success");

    return (int) rc;
}
