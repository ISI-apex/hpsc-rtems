#ifndef AFFINITY_H
#define AFFINITY_H

#include <rtems.h>

/**
 * Set provided task ID's affinity to use only the provided CPU.
 *
 * @retval RTEMS_SUCCESSFUL Successful operation.
 * @retval RTEMS_INVALID_ID Invalid task identifier.
 *
 * @see rtems_task_set_affinity()
 */
rtems_status_code affinity_pin_to_cpu(rtems_id task_id, int cpu);

/**
 * Set the current task's affinity to use only the provided CPU.
 *
 * @see rtems_task_set_affinity()
 */
void affinity_pin_self_to_cpu(int cpu);

#endif // AFFINITY_H
