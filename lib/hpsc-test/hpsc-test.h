#ifndef HPSC_TEST_H
#define HPSC_TEST_H

#include <rtems.h>

// libhpsc
#include <link.h>

// the following tests have no dependencies
int hpsc_test_command(void);
int hpsc_test_shmem(void);

// the following tests require "command" to be configured with a server to
// respond to PING requests
int hpsc_test_command_server(void);
// the link may be local (loopback) or remote
int hpsc_test_link_ping(struct link *link, rtems_interval wtimeout_ticks,
                        rtems_interval rtimeout_ticks);
int hpsc_test_link_shmem(rtems_interval wtimeout_ticks,
                         rtems_interval rtimeout_ticks);

#endif // HPSC_TEST_H
