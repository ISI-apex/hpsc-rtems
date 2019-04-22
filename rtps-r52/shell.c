#include <stdio.h>

#include <rtems.h>
#include <rtems/shell.h>

#include "shell.h"

rtems_status_code shell_create(void)
{
    printf(" =========================\n");
    printf(" Starting shell\n");
    printf(" =========================\n");
    return rtems_shell_init(
        "SHLL",                       /* task name */
        RTEMS_MINIMUM_STACK_SIZE * 4, /* task stack size */
        100,                          /* task priority */
        "/dev/console",               /* device name */
        /* device is currently ignored by the shell if it is not a pty */
        false,                        /* run forever */
        false,                        /* wait for shell to terminate */
        NULL                          /* login check function,
                                         use NULL to disable a login check */
    );
}
