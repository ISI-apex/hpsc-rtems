#include <rtems.h>

// libhpsc-test
#include <hpsc-test.h>

#include "test.h"

int test_command()
{
    int rc;
    test_begin("test_command");
    rc = hpsc_test_command();
    test_end("test_command", rc);
    return rc;
}

int test_shmem()
{
    int rc;
    test_begin("test_shmem");
    rc = hpsc_test_shmem();
    test_end("test_shmem", rc);
    return rc;
}

int test_command_server()
{
    int rc;
    test_begin("test_command_server");
    rc = hpsc_test_command_server();
    test_end("test_command_server", rc);
    return rc;
}
int test_link_shmem()
{
    int rc;
    rtems_interval wtimeout_ticks = 10;
    rtems_interval rtimeout_ticks = 10;
    test_begin("test_link_shmem");
    rc = hpsc_test_link_shmem(wtimeout_ticks, rtimeout_ticks);
    test_end("test_link_shmem", rc);
    return rc;
}
