#ifndef TEST_H
#define TEST_H

#include <stdbool.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

/*
 * There are three test classes:
 * 1) Standalone: only require drivers or library under test
 * 2) Local runtime: require some local tasks to be configured
 * 3) Externally dependent: require interaction with other subsystems/OSes
 */

// Standalone
int test_command(void);
int test_cpu_rti_timers(void);
int test_lsio_sram_syscfg(void);
int test_lsio_sram_dma_syscfg(void);
int test_mbox_lsio_loopback(void);
int test_rtps_dma(void); // wrapped by test_rtps_mmu
int test_rtps_mmu(bool do_dma_test);
int test_shmem(void);

// Local runtime
int test_command_server(void);
int test_link_shmem(void);

// Externally dependent
int test_link_mbox_trch(void);
int test_link_shmem_trch(void);

int test_mbox_rtps_hpps(unsigned mbox_in, unsigned mbox_out);

// test utilities

RTEMS_INLINE_ROUTINE void test_begin(const char *name)
{
    printk("TEST: %s: begin\n", name);
}

RTEMS_INLINE_ROUTINE void test_end(const char *name, int rc)
{
    printk("TEST: %s: %s\n", name, rc ? "failed": "success");
}

#define TEST_OR_SET_RC(val, r, msg) \
    if (!(val)) { \
        r = 1; \
        printk(msg); \
    }

#define TEST_OR_SET_RC_GOTO(val, r, msg, lbl) \
    if (!(val)) { \
        r = 1; \
        printk(msg); \
        goto lbl; \
    }

#define TEST_SC_OR_SET_RC(s, r, msg) \
    if ((s) != RTEMS_SUCCESSFUL) { \
        printk("TEST_SC: %s\n", rtems_status_text(sc)); \
        TEST_OR_SET_RC(0, r, msg) \
    }

#define TEST_SC_OR_SET_RC_GOTO(s, r, msg, lbl) \
    if ((s) != RTEMS_SUCCESSFUL) { \
        printk("TEST_SC: %s\n", rtems_status_text(sc)); \
        TEST_OR_SET_RC_GOTO(0, r, msg, lbl) \
    }

#endif // TEST_H
