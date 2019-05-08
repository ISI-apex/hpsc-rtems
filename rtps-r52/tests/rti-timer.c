#include <assert.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>

#include <rtems.h>

// drivers
#include <hpsc-rti-timer.h>

// selftest
#include <hpsc-rti-timer-test.h>

// libhpsc
#include <affinity.h>
#include <devices.h>

// plat
#include <hwinfo.h>

#include "test.h"

int test_cpu_rti_timers(void)
{
    struct hpsc_rti_timer *tmr;
    uint32_t id;
    enum hpsc_rti_timer_test_rc rc = 0;
    rtems_status_code sc RTEMS_UNUSED;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    test_begin("test_cpu_rti_timers");

    // store current affinity
    sc = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);
    printf("Saving CPU affinity count: %d\n", CPU_COUNT(&cpuset));

    dev_cpu_for_each(id) {
        // must first bind to the correct CPU
        affinity_pin_self_to_cpu(id);
        tmr = dev_cpu_get_rtit();
        assert(tmr);
        printf("Setting CPU affinity to CPU: %"PRIu32"\n", id);
        rc = hpsc_rti_timer_test_device(tmr, RTI_MAX_COUNT);
        if (rc)
            break;
    }

    // restore original affinity
    printf("Restoring CPU affinity count: %d\n", CPU_COUNT(&cpuset));
    sc = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

    test_end("test_cpu_rti_timers", rc);
    return (int) rc;
}
