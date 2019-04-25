#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/capture.h>
#include <rtems/shell.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

// libhpsc
#include <command.h>
#include <link-store.h>

#include "devices.h"
#include "shutdown.h"


static int shutdown_rtps_r52(int argc RTEMS_UNUSED, char *argv[] RTEMS_UNUSED)
{
    fprintf(stdout, "System shutting down at user request\n");
    shutdown();
    return 0;
}

rtems_shell_cmd_t shutdown_rtps_r52_command = {
    "shutdown",                                /* name */
    "shutdown",                                /* usage */
    "hpsc-rtps-r52",                           /* topic */
    shutdown_rtps_r52,                         /* command */
    NULL,                                      /* alias */
    NULL                                       /* next */
};

static bool shutdown_task_visitor(rtems_tcb *tcb, void *arg)
{
    rtems_id id = rtems_capture_task_id(tcb);
    rtems_status_code sc;
    // don't stop ourself
    if (rtems_task_self() == id) // RTEMS_SELF doesn't work here
        return false;
    printf("Deleting task: %u\n", rtems_capture_task_name(tcb));
    // deleting tasks causes a crash, so just suspend them instead
    if (rtems_task_is_suspended(id) == RTEMS_SUCCESSFUL) {
        sc = rtems_task_suspend(id);
        assert(sc == RTEMS_SUCCESSFUL);
    }
    return false;
}

void shutdown(void)
{
    struct link *link;
    const char *name;
    struct hpsc_rti_timer *rtit;
    struct hpsc_mbox *mbox;
    struct hpsc_wdt *wdt;
    rtems_status_code sc;
    int rc;
    size_t i;

    // TODO: Send a message to TRCH

    // try to stop gracefully
    printf("Stopping command handler...\n");
    sc = cmd_handle_task_destroy();
    if (sc != RTEMS_SUCCESSFUL)
        printf("Failed to stop command handler\n");

    // disconnect links
    printf("Disconnecting links...\n");
    while ((link = link_store_extract_first())) {
        name = link->name;
        rc = link_disconnect(link);
        if (rc)
            printf("Failed to disconnect link: %s\n", name);
    }

    // we destroyed links, so empty the command queue which references them
    printf("Dropping pending commands...\n");
    i = cmd_drop_all();
    printf("Dropped: %zu\n", i);

    // stop running tasks
    printf("Suspending tasks...\n");
    rtems_task_iterate(shutdown_task_visitor, NULL);

    // disable timers
    printf("Disabling RTI timers...\n");
    dev_id_cpu_for_each_rtit(i, rtit) {
        if (rtit) {
            dev_remove_rtit(i);
            sc = hpsc_rti_timer_remove(rtit);
            assert(sc == RTEMS_SUCCESSFUL);
        }
    }

    // shutdown mailboxes
    printf("Removing mailboxes...\n");
    dev_id_mbox_for_each(i, mbox) {
        if (mbox) {
            dev_remove_mbox(i);
            sc = hpsc_mbox_remove(mbox);
            assert(sc == RTEMS_SUCCESSFUL);
        }
    }

    // stop kicking watchdogs
    printf("Removing watchdogs...\n");
    dev_id_cpu_for_each_wdt(i, wdt) {
        if (wdt) {
            dev_remove_wdt(i);
            sc = hpsc_wdt_remove(wdt);
            assert(sc == RTEMS_SUCCESSFUL);
        }
    }

    // RTEMS should enter an infinite loop until TRCH resets us
    printf("Exiting application...\n");
    exit(0);
}
