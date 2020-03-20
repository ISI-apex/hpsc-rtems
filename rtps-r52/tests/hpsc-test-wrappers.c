// libhpsc-test
#include <hpsc-test.h>

#include "test.h"

int test_command(void)
{
    int rc;
    test_begin("test_command");
    rc = hpsc_test_command();
    test_end("test_command", rc);
    return rc;
}

int test_shmem(void)
{
    int rc;
    test_begin("test_shmem");
    rc = hpsc_test_shmem();
    test_end("test_shmem", rc);
    return rc;
}

int test_command_server(void)
{
    int rc;
    test_begin("test_command_server");
    rc = hpsc_test_command_server();
    test_end("test_command_server", rc);
    return rc;
}

#define SHMEM_WTIMEOUT_TICKS 5000
#define SHMEM_RTIMEOUT_TICKS 5000
// assumes calling task isn't using this event
#define SHMEM_EVENT_WAIT     RTEMS_EVENT_0
int test_link_shmem(void)
{
    int rc;
    test_begin("test_link_shmem");
    rc = hpsc_test_link_shmem(SHMEM_WTIMEOUT_TICKS, SHMEM_RTIMEOUT_TICKS,
                              SHMEM_EVENT_WAIT);
    test_end("test_link_shmem", rc);
    return rc;
}
