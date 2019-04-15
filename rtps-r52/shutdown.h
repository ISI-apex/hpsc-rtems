#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include <rtems.h>
#include <rtems/shell.h>

void shutdown(void);

extern rtems_shell_cmd_t shutdown_rtps_r52_command;

#endif // SHUTDOWN_H
