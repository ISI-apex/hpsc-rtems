#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// libhpsc
#include <command.h>
#include <hpsc-msg.h>
#include <link.h>
#include <shmem.h>
#include <shmem-link.h>

#include "test.h"

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
                      CMD_TIMEOUT_TICKS_SEND, cmd, sizeof(cmd),
                      CMD_TIMEOUT_TICKS_RECV, reply, sizeof(reply));
    if (sz <= 0)
        rc = 1;
    else if (reply[0] != PONG)
        rc = 1;

    // wait for command handler to finish, o/w we prematurely destroy the link
    while (status == CMD_STATUS_UNKNOWN);
    cmd_handled_unregister_cb();

    return status == CMD_STATUS_SUCCESS ? rc : 1;
}

// test command handler using a loopback shmem-link
int test_command_server()
{
    struct hpsc_shmem_region reg_a = { 0 };
    struct hpsc_shmem_region reg_b = { 0 };
    rtems_name sname_recv = rtems_build_name('T','C','S','R');
    rtems_name sname_ack = rtems_build_name('T','C','S','A');
    rtems_name cname_recv = rtems_build_name('T','C','C','R');
    rtems_name cname_ack = rtems_build_name('T','C','C','A');
    struct link *slink;
    struct link *clink;
    int rc;

    printf("TEST: test_command_server: begin\n");

    slink = shmem_link_connect("Command Test Server Link", &reg_a, &reg_b,
                               true, 1, sname_recv, sname_ack);
    if (!slink)
        return 1;
    clink = shmem_link_connect("Command Test Client Link", &reg_b, &reg_a,
                               false, 1, cname_recv, cname_ack);
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
