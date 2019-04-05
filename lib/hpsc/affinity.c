#include <assert.h>
#include <sched.h>

#include <rtems.h>

void affinity_pin_to_cpu(rtems_id task_id, int cpu)
{
    rtems_status_code sc;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sc = rtems_task_set_affinity(task_id, sizeof(cpuset), &cpuset);
    assert(sc == RTEMS_SUCCESSFUL);
}

void affinity_pin_self_to_cpu(int cpu)
{
    affinity_pin_to_cpu(RTEMS_SELF, cpu);
}
