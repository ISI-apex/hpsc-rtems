#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// libhpsc
#include <command.h>
#include <hpsc-msg.h>
#include <shmem.h>
#include <shmem-link.h>

#include "test.h"

struct cmd_test {
    struct cmd cmd;
    cmd_status status;
};

static void handled_cb(void *arg, cmd_status status)
{
    struct cmd_test *cmdt = arg;
    cmdt->status = status;
}

static int do_test(struct link *slink, struct link *clink)
{
    // pretend client sent a PING to server (we can skip the actual exchange)
    // command will be processed asynchronously and reply sent to client
    uint8_t ibuf[SHMEM_MSG_SIZE] = { 0 };
    struct cmd_test cmdt = {
        .cmd = {
            .msg = { PING, 0 },
            .link = slink,
        },
        .status = CMD_STATUS_UNKNOWN
    };
    int rc;

    rc = cmd_enqueue_cb(&cmdt.cmd, handled_cb, &cmdt);
    if (rc)
        return rc;

    // wait for reply
    while (!clink->recv(clink, ibuf, sizeof(ibuf)));
    // reply should be a PONG
    if (ibuf[0] != PONG)
        rc = 1;

    // wait for command handler to finish, o/w we prematurely destroy the link
    while(cmdt.status == CMD_STATUS_UNKNOWN);

    return cmdt.status == CMD_STATUS_SUCCESS ? rc : 1;
}

// test command handler using a loopback shmem-link
int test_command_server()
{
    struct hpsc_shmem_region reg_a = { 0 };
    struct hpsc_shmem_region reg_b = { 0 };
    
    struct link *slink;
    struct link *clink;
    int rc;

    printf("TEST: test_command_server: begin\n");

    slink = shmem_link_connect("Command Test Server Shmem Link", &reg_a, &reg_b);
    if (!slink)
        return 1;
    clink = shmem_link_connect("Command Test Client Shmem Link", &reg_b, &reg_a);
    if (!clink) {
        rc = 1;
        goto free_slink;
    }

    rc = do_test(slink, clink);

    clink->disconnect(clink); // never fails
free_slink:
    slink->disconnect(slink); // never fails
    printf("TEST: test_command_server: %s\n", rc ? "failed": "success");
    return rc;
}
