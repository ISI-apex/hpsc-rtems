#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/shell.h>

#include "test.h"

#define SHELL_TESTS_TOPIC "hpsc-rtps-r52-test"

/******************************************************************************/
// Standalone
/******************************************************************************/

static int shell_test_command(int argc RTEMS_UNUSED, char *argv[] RTEMS_UNUSED)
{
    return test_command();
}
rtems_shell_cmd_t shell_cmd_test_command = {
    "test_command",                            /* name */
    "test_command",                            /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_command,                        /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_cpu_rti_timers(int argc RTEMS_UNUSED,
                                     char *argv[] RTEMS_UNUSED)
{
#if CONFIG_RTI_TIMER
    return test_cpu_rti_timers();
#else 
    fprintf(stderr, "ERROR: CONFIG_RTI_TIMER is not set!\n")
    return -1;
#endif // CONFIG_RTI_TIMER
}
rtems_shell_cmd_t shell_cmd_test_cpu_rti_timers = {
    "test_cpu_rti_timers",                     /* name */
    "test_cpu_rti_timers",                     /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_cpu_rti_timers,                 /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_lsio_sram(int argc RTEMS_UNUSED,
                                char *argv[] RTEMS_UNUSED)
{
    return test_lsio_sram();
}
rtems_shell_cmd_t shell_cmd_test_lsio_sram = {
    "test_lsio_sram",                           /* name */
    "test_lsio_sram",                           /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_lsio_sram,                       /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_lsio_sram_dma(int argc RTEMS_UNUSED,
                                           char *argv[] RTEMS_UNUSED)
{
    return test_lsio_sram_dma();
}
rtems_shell_cmd_t shell_cmd_test_lsio_sram_dma = {
    "test_lsio_sram_dma",                       /* name */
    "test_lsio_sram_dma",                       /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_lsio_sram_dma,                   /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_mbox_lsio_loopback(int argc RTEMS_UNUSED,
                                         char *argv[] RTEMS_UNUSED)
{
#if CONFIG_MBOX_LSIO
    return test_mbox_lsio_loopback();
#else
    fprintf(stderr, "ERROR: CONFIG_MBOX_LSIO is not set!\n");
    return -1;
#endif // CONFIG_MBOX_LSIO
}
rtems_shell_cmd_t shell_cmd_test_mbox_lsio_loopback = {
    "test_mbox_lsio_loopback",                 /* name */
    "test_mbox_lsio_loopback",                 /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_mbox_lsio_loopback,             /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_rtps_mmu(int argc RTEMS_UNUSED, char *argv[] RTEMS_UNUSED)
{
    return test_rtps_mmu(false);
}
rtems_shell_cmd_t shell_cmd_test_rtps_mmu = {
    "test_rtps_mmu",                           /* name */
    "test_rtps_mmu",                           /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_rtps_mmu,                       /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_rtps_mmu_dma(int argc RTEMS_UNUSED,
                                   char *argv[] RTEMS_UNUSED)
{
    return test_rtps_mmu(true);
}
rtems_shell_cmd_t shell_cmd_test_rtps_mmu_dma = {
    "test_rtps_mmu_dma",                       /* name */
    "test_rtps_mmu_dma",                       /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_rtps_mmu_dma,                   /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_shmem(int argc RTEMS_UNUSED, char *argv[] RTEMS_UNUSED)
{
    return test_shmem();
}
rtems_shell_cmd_t shell_cmd_test_shmem = {
    "test_shmem",                              /* name */
    "test_shmem",                              /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_shmem,                          /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

/******************************************************************************/
// Local runtime
/******************************************************************************/

static int shell_test_command_server(int argc RTEMS_UNUSED,
                                     char *argv[] RTEMS_UNUSED)
{
    return test_command_server();
}
rtems_shell_cmd_t shell_cmd_test_command_server = {
    "test_command_server",                     /* name */
    "test_command_server",                     /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_command_server,                 /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_link_shmem(int argc RTEMS_UNUSED,
                                 char *argv[] RTEMS_UNUSED)
{
    return test_link_shmem();
}
rtems_shell_cmd_t shell_cmd_test_link_shmem = {
    "test_link_shmem",                         /* name */
    "test_link_shmem",                         /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_link_shmem,                     /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

/******************************************************************************/
// Externally dependent
/******************************************************************************/

static int shell_test_link_mbox_trch(int argc RTEMS_UNUSED,
                                     char *argv[] RTEMS_UNUSED)
{
#if CONFIG_LINK_MBOX_TRCH_CLIENT
    return test_link_mbox_trch();
#else
    fprintf(stderr, "ERROR: CONFIG_LINK_MBOX_TRCH_CLIENT is not set!\n");
    return -1;
#endif // CONFIG_LINK_MBOX_TRCH_CLIENT
}
rtems_shell_cmd_t shell_cmd_test_link_mbox_trch = {
    "test_link_mbox_trch",                     /* name */
    "test_link_mbox_trch",                     /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_link_mbox_trch,                 /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_link_shmem_trch(int argc RTEMS_UNUSED,
                                      char *argv[] RTEMS_UNUSED)
{
#if CONFIG_LINK_SHMEM_TRCH_CLIENT
    return test_link_shmem_trch();
#else
    fprintf(stderr, "ERROR: CONFIG_LINK_SHMEM_TRCH_CLIENT is not set!\n");
    return -1;
#endif // CONFIG_LINK_SHMEM_TRCH_CLIENT
}
rtems_shell_cmd_t shell_cmd_test_link_shmem_trch = {
    "test_link_shmem_trch",                    /* name */
    "test_link_shmem_trch",                    /* usage */
    SHELL_TESTS_TOPIC,                         /* topic */
    shell_test_link_shmem_trch,                /* command */
    NULL, NULL,                                /* alias, next */
    0, 0, 0                                    /* mode, uid, gid */
};

static int shell_test_mbox_rtps_hpps(int argc RTEMS_UNUSED,
                                         char *argv[] RTEMS_UNUSED)
{
    if (argc <= 1) {
        fprintf(stderr, "ERROR: TEST REQUIRES 2 MBOXES\n");
        return -1;
    }

    unsigned mbox_in = atoi(argv[1]);
    unsigned mbox_out = atoi(argv[2]);

    return test_mbox_rtps_hpps(mbox_in, mbox_out);
}
rtems_shell_cmd_t shell_cmd_test_mbox_rtps_hpps = {
    "test_mbox_rtps_hpps",                          /* name */
    "test_mbox_rtps_hpps",                          /* usage */
    SHELL_TESTS_TOPIC,                              /* topic */
    shell_test_mbox_rtps_hpps,                      /* command */
    NULL, NULL,                                     /* alias, next */
    0, 0, 0                                         /* mode, uid, gid */
};


