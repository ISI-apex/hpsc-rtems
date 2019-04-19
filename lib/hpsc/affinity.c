#include <assert.h>
#include <sched.h>

#include <rtems.h>

rtems_status_code affinity_pin_to_cpu(rtems_id task_id, int cpu)
{
    rtems_status_code sc;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sc = rtems_task_set_affinity(task_id, sizeof(cpuset), &cpuset);
    assert(sc != RTEMS_INVALID_ADDRESS); // cpuset is NULL (it's not)
    if (sc == RTEMS_INVALID_NUMBER) // invalid processor affinity set
        rtems_panic("affinity_pin_to_cpu: bad cpu value: %d", cpu);
    return sc; // either RTEMS_SUCCESSFUL or RTEMS_INVALID_ID
}

void affinity_pin_self_to_cpu(int cpu)
{
    rtems_status_code sc = affinity_pin_to_cpu(RTEMS_SELF, cpu);
    assert(sc != RTEMS_INVALID_ID); // invalid task id (but we used RTEMS_SELF)
    assert(sc == RTEMS_SUCCESSFUL);
}
