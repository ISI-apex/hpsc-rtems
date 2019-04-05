#include <stdbool.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/shell.h>

#include "shell.h"

static rtems_task shell_task(rtems_task_argument ignored)
{
    printf(" =========================\n");
    printf(" Starting shell\n");
    printf(" =========================\n");
    rtems_shell_init(
        "SHLL",                       /* task name */
        RTEMS_MINIMUM_STACK_SIZE * 4, /* task stack size */
        100,                          /* task priority */
        "/dev/console",               /* device name */
        /* device is currently ignored by the shell if it is not a pty */
        false,                        /* run forever */
        true,                         /* wait for shell to terminate */
        NULL       /* login check function,
        use NULL to disable a login check */
    );
}

rtems_status_code shell_create(void)
{
    rtems_id task_id;
    rtems_name task_name = rtems_build_name('S','H','L','L');
    rtems_status_code sc;

    sc = rtems_task_create(
        task_name, 1, RTEMS_MINIMUM_STACK_SIZE * 2,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    if (sc != RTEMS_SUCCESSFUL) {
        printf("Failed to create shell task\n");
        return sc;
    }
    sc = rtems_task_start(task_id, shell_task, 1);
    if (sc != RTEMS_SUCCESSFUL)
        printf("Failed to start shell task\n");
    return sc;
}
