#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtems.h>

// libhpsc
#include <command.h>
#include <link.h>
#include <link-shmem.h>
#include <shmem.h>

#include "hpsc-test.h"

static void handled_cb(void *arg, cmd_status status)
{
    cmd_status *s = (cmd_status *)arg;
    *s = status;
}

static int do_test(struct link *slink, struct link *clink,
                   rtems_interval wtimeout_ticks, rtems_interval rtimeout_ticks,
                   rtems_event_set event_wait)
{
    // command is sent by client and received by server
    int rc;
    cmd_status status = CMD_STATUS_UNKNOWN;

    cmd_handled_register_cb(handled_cb, &status);
    rc = hpsc_test_link_ping(clink, wtimeout_ticks, rtimeout_ticks, event_wait);
    // wait for command handler to finish, o/w we prematurely destroy the link
    while (status == CMD_STATUS_UNKNOWN)
        rtems_task_wake_after(RTEMS_YIELD_PROCESSOR);
    cmd_handled_unregister_cb();
    return status == CMD_STATUS_SUCCESS ? rc : 1;
}

static void create_poll_task(rtems_name name, rtems_id *id)
{
    assert(id);
    rtems_status_code sc = rtems_task_create(
        name, 1, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES, id
    );
    if (sc != RTEMS_SUCCESSFUL)
        rtems_panic("create_poll_task: %s", rtems_status_text(sc));
}

// test link-shmem (requires command handler to be configured)
int hpsc_test_link_shmem(rtems_interval wtimeout_ticks,
                         rtems_interval rtimeout_ticks,
                         rtems_event_set event_wait)
{
    struct hpsc_shmem_region reg_a = { 0 };
    struct hpsc_shmem_region reg_b = { 0 };
    rtems_id stid_recv;
    rtems_id stid_ack;
    rtems_id ctid_recv;
    rtems_id ctid_ack;
    struct link *slink;
    struct link *clink;
    int rc;

    create_poll_task(rtems_build_name('T','C','S','R'), &stid_recv);
    create_poll_task(rtems_build_name('T','C','S','A'), &stid_ack);
    slink = link_shmem_connect("Shmem Link Test Server",
                               (uintptr_t) &reg_a, (uintptr_t) &reg_b,
                               true, 1, stid_recv, stid_ack);
    if (!slink) {
        // manually cleanup resources for tasks that may not have been started
        rtems_task_delete(stid_recv);
        rtems_task_delete(stid_ack);
        return 1;
    }
    create_poll_task(rtems_build_name('T','C','C','R'), &ctid_recv);
    create_poll_task(rtems_build_name('T','C','C','A'), &ctid_ack);
    clink = link_shmem_connect("Shmem Link Test Client",
                               (uintptr_t) &reg_b, (uintptr_t) &reg_a,
                               false, 1, ctid_recv, ctid_ack);
    if (!clink) {
        rc = 1;
        // manually cleanup resources for tasks that may not have been started
        rtems_task_delete(ctid_recv);
        rtems_task_delete(ctid_ack);
        goto free_slink;
    }

    rc = do_test(slink, clink, wtimeout_ticks, rtimeout_ticks, event_wait);

    if (link_disconnect(clink))
        rc = 1;
free_slink:
    if (link_disconnect(slink))
        rc = 1;

    return rc;
}
