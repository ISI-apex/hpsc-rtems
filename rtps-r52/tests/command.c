#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// libhpsc
#include <command.h>
#include <hpsc-msg.h>
#include <link.h>
#include <link-shmem.h>
#include <shmem.h>

#include "test.h"

#define WTIMEOUT_TICKS 10
#define RTIMEOUT_TICKS 10

static void handled_cb(void *arg, cmd_status status)
{
    cmd_status *s = (cmd_status *)arg;
    *s = status;
}

static int do_test(struct link *slink, struct link *clink)
{
    // command is sent by client and received by server
    HPSC_MSG_DEFINE(cmd);
    HPSC_MSG_DEFINE(reply);
    ssize_t sz;
    int rc = 0;
    cmd_status status = CMD_STATUS_UNKNOWN;

    cmd_handled_register_cb(handled_cb, &status);

    hpsc_msg_ping(cmd, sizeof(cmd), NULL, 0);
    sz = link_request(clink,
                      WTIMEOUT_TICKS, cmd, sizeof(cmd),
                      RTIMEOUT_TICKS, reply, sizeof(reply));
    if (sz <= 0)
        rc = 1;
    else if (reply[0] != PONG)
        rc = 1;

    // wait for command handler to finish, o/w we prematurely destroy the link
    while (status == CMD_STATUS_UNKNOWN);
    cmd_handled_unregister_cb();

    return status == CMD_STATUS_SUCCESS ? rc : 1;
}

static void create_poll_task(rtems_name name, rtems_id *id)
{
    assert(id);
    rtems_status_code sc = rtems_task_create(
        name, 1, RTEMS_MINIMUM_STACK_SIZE * 2,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, id
    );
    assert(sc == RTEMS_SUCCESSFUL);
}

// test command handler using a loopback shmem-link
int test_command_server()
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

    printf("TEST: test_command_server: begin\n");

    create_poll_task(rtems_build_name('T','C','S','R'), &stid_recv);
    create_poll_task(rtems_build_name('T','C','S','A'), &stid_ack);
    create_poll_task(rtems_build_name('T','C','C','R'), &ctid_recv);
    create_poll_task(rtems_build_name('T','C','C','A'), &ctid_ack);
    slink = link_shmem_connect("Command Test Server Link", &reg_a, &reg_b,
                               true, 1, stid_recv, stid_ack);
    if (!slink)
        return 1;
    clink = link_shmem_connect("Command Test Client Link", &reg_b, &reg_a,
                               false, 1, ctid_recv, ctid_ack);
    if (!clink) {
        rc = 1;
        goto free_slink;
    }

    rc = do_test(slink, clink);
    printf("TEST: test_command_server: %s\n", rc ? "failed": "success");

    if (link_disconnect(clink))
        rc = 1;
free_slink:
    if (link_disconnect(slink))
        rc = 1;
    return rc;
}
