#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/capture.h>
#include <rtems/shell.h>
#include <bsp/hpsc-wdt.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>

// libhpsc
#include <affinity.h>
#include <command.h>
#include <devices.h>
#include <link.h>
#include <link-store.h>

#include "shutdown.h"
#include "watchdog.h"


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
    NULL,                                      /* next */
    0,                                         /* mode */
    0,                                         /* uid */
    0                                          /* gid */
};

static bool shutdown_task_visitor(rtems_tcb *tcb, void *arg)
{
    rtems_id id = rtems_capture_task_id(tcb);
    rtems_status_code sc RTEMS_UNUSED;
    // don't stop ourself
    if (rtems_task_self() == id) // RTEMS_SELF doesn't work here
        return false;
    printf("Suspending task: %u\n", rtems_capture_task_name(tcb));
    // deleting tasks causes a crash, so just suspend them instead
    sc = rtems_task_suspend(id);
    // only other option is RTEMS_INVALID_ID, which should never happen
    assert(sc == RTEMS_SUCCESSFUL || sc == RTEMS_ALREADY_SUSPENDED);
    return false;
}

RTEMS_NO_RETURN void shutdown(void)
{
    struct link *link;
    const char *name;
    struct hpsc_rti_timer *rtit;
    struct hpsc_mbox *mbox;
    struct HPSC_WDT_Config *wdt;
    rtems_status_code sc;
    size_t i;
    uint32_t cpu;

    // try to stop gracefully
    printf("Stopping command handler...\n");
    sc = cmd_handle_task_destroy();
    if (sc != RTEMS_SUCCESSFUL)
        printf("Failed to stop command handler\n");

    // NOTE: stop any other tasks with handles on links before continuing

    // disconnect links
    printf("Disconnecting links...\n");
    while ((link = link_store_extract_first())) {
        name = link->name;
        if (link_disconnect(link))
            printf("Failed to disconnect link: %s\n", name);
    }

    // Empty the command queue which may now have stale link references.
    // This is done _after_ destroying links because links may have received and
    // enqueued messages after we stopped the command handler task, and we can't
    // destroy the links _before_ stopping the command handler task.
    printf("Dropping pending commands...\n");
    i = cmd_drop_all();
    printf("Dropped: %zu\n", i);

    // stop kicking watchdogs
    printf("Stopping watchdog tasks...\n");
    watchdog_tasks_destroy();

    // suspend any remaining tasks
    printf("Suspending tasks...\n");
    rtems_task_iterate(shutdown_task_visitor, NULL);

    // shutdown mailboxes
    printf("Removing mailboxes...\n");
    dev_id_mbox_for_each(i, mbox) {
        if (mbox) {
            dev_set_mbox(i, NULL);
            sc = hpsc_mbox_remove(mbox);
            if (sc != RTEMS_SUCCESSFUL)
                rtems_panic("shutdown: hpsc_mbox_remove");
        }
    }

    // From here on we change our CPU affinity, but we don't really need to
    // save/restore it since we're shutting down...
    // TODO: may not be able to bind to all CPUs if task's cpuset disallows it

    // disable timers
    printf("Removing RTI timers...\n");
    dev_cpu_for_each(cpu) {
        affinity_pin_self_to_cpu(cpu);
        rtit = dev_cpu_get_rtit();
        if (rtit) {
            dev_cpu_set_rtit(NULL);
            sc = hpsc_rti_timer_remove(rtit);
            if (sc != RTEMS_SUCCESSFUL)
                rtems_panic("shutdown: hpsc_rti_timer_remove");
        }
    }

    // stop kicking watchdogs
    printf("Removing watchdogs...\n");
    dev_cpu_for_each(cpu) {
        affinity_pin_self_to_cpu(cpu);
        wdt = dev_cpu_get_wdt();
        if (wdt) {
            dev_cpu_set_wdt(NULL);
            wdt_uninit(wdt);
        }
    }

    // RTEMS should enter an infinite loop until TRCH resets us
    printf("Exiting application...\n");
    exit(0);
}
