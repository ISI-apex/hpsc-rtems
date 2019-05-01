#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <rtems.h>

rtems_status_code watchdog_tasks_create(rtems_task_priority priority);

#endif // WATCHDOG_H
