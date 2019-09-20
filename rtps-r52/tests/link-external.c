// libhpsc
#include <link-store.h>

// libhpsc-test
#include <hpsc-test.h>

#include "link-names.h"
#include "test.h"

#define WTIMEOUT_TICKS 1000
#define RTIMEOUT_TICKS 1000
// assumes calling task isn't using this event
#define LINK_EVENT_WAIT RTEMS_EVENT_0

int test_link_mbox_trch(void)
{
    int rc;
    test_begin("test_link_mbox_trch");
    rc = hpsc_test_link_ping(link_store_get(LINK_NAME__MBOX__TRCH_CLIENT),
                             WTIMEOUT_TICKS, RTIMEOUT_TICKS, LINK_EVENT_WAIT);
    test_end("test_link_mbox_trch", rc);
    return rc;
}

int test_link_shmem_trch(void)
{
    int rc;
    test_begin("test_link_shmem_trch");
    rc = hpsc_test_link_ping(link_store_get(LINK_NAME__SHMEM__TRCH_CLIENT),
                             WTIMEOUT_TICKS, RTIMEOUT_TICKS, LINK_EVENT_WAIT);
    test_end("test_link_shmem_trch", rc);
    return rc;
}
