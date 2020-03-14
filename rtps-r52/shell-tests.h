#ifndef SHELL_TESTS_H
#define SHELL_TESTS_H

#include <rtems.h>
#include <rtems/shell.h>

// Shell test commands wrap functions in test.h

// Standalone
extern rtems_shell_cmd_t shell_cmd_test_command;
extern rtems_shell_cmd_t shell_cmd_test_cpu_rti_timers;
extern rtems_shell_cmd_t shell_cmd_test_lsio_sram;
extern rtems_shell_cmd_t shell_cmd_test_lsio_sram_dma;
extern rtems_shell_cmd_t shell_cmd_test_mbox_lsio_loopback;
extern rtems_shell_cmd_t shell_cmd_test_rtps_mmu;
extern rtems_shell_cmd_t shell_cmd_test_rtps_mmu_dma;
extern rtems_shell_cmd_t shell_cmd_test_shmem;

// Local runtime
extern rtems_shell_cmd_t shell_cmd_test_command_server;
extern rtems_shell_cmd_t shell_cmd_test_link_shmem;

// Externally dependent
extern rtems_shell_cmd_t shell_cmd_test_link_mbox_trch;
extern rtems_shell_cmd_t shell_cmd_test_link_shmem_trch;

extern rtems_shell_cmd_t shell_cmd_test_mbox_rtps_hpps;

#endif // SHELL_TESTS_H
