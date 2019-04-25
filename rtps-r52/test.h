#ifndef TEST_H
#define TEST_H

#include <stdio.h>

/*
 * There are three test classes:
 * 1) Standalone: only require drivers or library under test
 * 2) Local runtime: require some local tasks to be configured
 * 3) Externally dependent: require interaction with other subsystems/OSes
 */

// Standalone
int test_command();
int test_cpu_rti_timers();
int test_mbox_lsio_loopback();
int test_shmem();

// Local runtime
int test_command_server();
int test_link_shmem();

// Externally dependent
int test_link_mbox_trch();
int test_link_shmem_trch();

// test utilities

static inline void test_begin(const char *name)
{
    printf("TEST: %s: begin\n", name);
}

static inline void test_end(const char *name, int rc)
{
    printf("TEST: %s: %s\n", name, rc ? "failed": "success");
}

#endif // TEST_H
