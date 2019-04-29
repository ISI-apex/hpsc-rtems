#include <assert.h>
#include <stdint.h>
#include <unistd.h>

// libhpsc
#include <hpsc-msg.h>
#include <link.h>
#include <link-store.h>

// libhpsc-test
#include <hpsc-test.h>

#include "link-names.h"
#include "test.h"

#define WTIMEOUT_TICKS 100
#define RTIMEOUT_TICKS 100

int test_link_mbox_trch()
{
    int rc;
    test_begin("test_link_mbox_trch");
    rc = hpsc_test_link_ping(link_store_get(LINK_NAME__MBOX__TRCH_CLIENT),
                             WTIMEOUT_TICKS, RTIMEOUT_TICKS);
    test_end("test_link_mbox_trch", rc);
    return rc;
}


int test_link_shmem_trch()
{
    int rc;
    test_begin("test_link_shmem_trch");
    rc = hpsc_test_link_ping(link_store_get(LINK_NAME__SHMEM__TRCH_CLIENT),
                             WTIMEOUT_TICKS, RTIMEOUT_TICKS);
    test_end("test_link_shmem_trch", rc);
    return rc;
}
