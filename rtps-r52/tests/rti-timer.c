#include <assert.h>
#include <sched.h>
#include <stdio.h>

#include <rtems.h>

// drivers
#include <hpsc-rti-timer.h>

// selftest
#include <hpsc-rti-timer-test.h>

// libhpsc
#include <affinity.h>

// plat
#include <hwinfo.h>

#include "devices.h"
#include "test.h"

int test_cpu_rti_timers()
{
    struct hpsc_rti_timer *tmr = NULL;
    dev_id_cpu id = 0;
    unsigned count = 0;
    enum hpsc_rti_timer_test_rc rc = 0;
    rtems_status_code sc;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    printf("TEST: test_core_rti_timer: begin\n");

    // store current affinity
    sc = rtems_task_get_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);
    printf("Saving CPU affinity count: %d\n", CPU_COUNT(&cpuset));

    dev_id_cpu_for_each_rtit(id, tmr) {
        if (tmr) {
            count++;
            // must first bind to the correct CPU
            printf("Setting CPU affinity to CPU: %u\n", id);
            affinity_pin_self_to_cpu(id);
            rc = hpsc_rti_timer_test_device(tmr, RTI_MAX_COUNT);
            if (rc)
                break;
        }
    }

    // restore original affinity
    printf("Restoring CPU affinity count: %d\n", CPU_COUNT(&cpuset));
    sc = rtems_task_set_affinity(RTEMS_SELF, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);

    assert(count); // need to have tested at least one core's timer
    printf("TEST: test_core_rti_timer: %s\n", rc ? "failed": "success");
    return (int) rc;
}
