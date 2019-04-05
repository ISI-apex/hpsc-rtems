#ifndef AFFINITY_H
#define AFFINITY_H

#include <rtems.h>

void affinity_pin_to_cpu(rtems_id task_id, int cpu);

void affinity_pin_self_to_cpu(int cpu);

#endif // AFFINITY_H
