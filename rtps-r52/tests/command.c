#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// libhpsc
#include <command.h>
#include <hpsc-msg.h>
#include <link.h>

#include "test.h"

struct cmd_test_link {
    bool is_write;
    void *reply;
    size_t reply_sz;
};

struct cmd_test {
    struct cmd cmd;
    cmd_status status;
};

static size_t test_link_write(struct link *link, void *buf, size_t sz)
{
    assert(link);
    assert(link->priv);
    assert(buf);
    assert(sz <= HPSC_MSG_SIZE);
    struct cmd_test_link *tlink = link->priv;
    memcpy(tlink->reply, buf, sz);
    tlink->reply_sz = sz;
    tlink->is_write = true;
    return sz;
}
static size_t test_link_read(struct link *link, void *buf, size_t sz)
{
    assert(0); // should never be called
    return 0;
}
static int test_link_close(struct link *link)
{
    assert(0); // should never be called
    return -1;
}

static void handled_cb(void *arg, cmd_status status)
{
    struct cmd_test *cmdt = arg;
    assert(cmdt);
    cmdt->status = status;
}

static int do_test(struct link *link)
{
    // pretend client sent a PING to server (we can skip the actual exchange)
    // command will be processed asynchronously and reply sent to client
    struct cmd_test cmdt = {
        .cmd = {
            .msg = { PING, 0 },
            .link = link,
        },
        .status = CMD_STATUS_UNKNOWN
    };
    struct cmd_test_link *tlink = link->priv;
    int rc = cmd_enqueue_cb(&cmdt.cmd, handled_cb, &cmdt);
    if (rc)
        return rc;

    // wait for reply
    while (!tlink->is_write);
    // ack the reply
    link_ack(link);
    // reply should be a PONG
    if (!tlink->reply_sz || ((uint8_t *)tlink->reply)[0] != PONG)
        rc = 1;

    // wait for command handler to finish, o/w we prematurely destroy the link
    while(cmdt.status == CMD_STATUS_UNKNOWN);

    return cmdt.status == CMD_STATUS_SUCCESS ? rc : 1;
}

// test command handler using a dummy link implementation
int test_command_server()
{
    HPSC_MSG_DEFINE(reply);
    struct cmd_test_link tlink = {
        .is_write = false,
        .reply = &reply,
        .reply_sz = 0
    };
    struct link link = {
        .name = "Command Test Link",
        .priv = &tlink,
        .write = test_link_write,
        .read = test_link_read,
        .close = test_link_close
    };
    int rc;
    printf("TEST: test_command_server: begin\n");
    rc = do_test(&link);
    printf("TEST: test_command_server: %s\n", rc ? "failed": "success");
    return rc;
}
