#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <rtems.h>

void watchdog_tasks_create(rtems_task_priority priority);

void watchdog_tasks_destroy(void);

#endif // WATCHDOG_H
