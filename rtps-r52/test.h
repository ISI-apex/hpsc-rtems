#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#include <rtems.h>

/*
 * There are three test classes:
 * 1) Standalone: only require drivers or library under test
 * 2) Local runtime: require some local tasks to be configured
 * 3) Externally dependent: require interaction with other subsystems/OSes
 */

// Standalone
int test_command(void);
int test_cpu_rti_timers(void);
int test_mbox_lsio_loopback(void);
int test_shmem(void);

// Local runtime
int test_command_server(void);
int test_link_shmem(void);

// Externally dependent
int test_link_mbox_trch(void);
int test_link_shmem_trch(void);

// test utilities

RTEMS_INLINE_ROUTINE void test_begin(const char *name)
{
    printf("TEST: %s: begin\n", name);
}

RTEMS_INLINE_ROUTINE void test_end(const char *name, int rc)
{
    printf("TEST: %s: %s\n", name, rc ? "failed": "success");
}

#endif // TEST_H
