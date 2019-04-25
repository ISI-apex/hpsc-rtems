#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <rtems.h>

// libhpsc
#include <command.h>

#include "test.h"

#define CMD_TEST_TIMEOUT_TICKS 10

struct cmd_test_ctx {
    struct cmd cmd;
    rtems_id tid;
    bool handled_cmd;
    cmd_status status;
};

static ssize_t cmd_test_handler(struct cmd *cmd, void *reply, size_t reply_sz)
{
    assert(cmd);
    assert(reply);
    assert(reply_sz);
    return 0; // no reply (there's no link to reply with)
}

static void handled_cmd_cb(void *arg, cmd_status status)
{
    struct cmd_test_ctx *cmdt = arg;
    assert(cmdt);
    cmdt->handled_cmd = true;
    rtems_event_send(cmdt->tid, RTEMS_EVENT_0);
}

static void handled_cb(void *arg, cmd_status status)
{
    struct cmd_test_ctx *cmdt = arg;
    assert(cmdt);
    cmdt->status = status;
    rtems_event_send(cmdt->tid, RTEMS_EVENT_1);
}

static int do_test(rtems_id task_id)
{
    rtems_status_code sc;
    rtems_event_set events = 0;
    int rc = 1;
    struct cmd_test_ctx cmdt = {
        .cmd = {
            .msg = { 0 },
            .link = NULL,
        },
        .tid = rtems_task_self(),
        .handled_cmd = false,
        .status = CMD_STATUS_UNKNOWN
    };

    // register "handled" callback
    cmd_handled_register_cb(handled_cb, &cmdt);

    // start the command handle task
    sc = cmd_handle_task_start(task_id, cmd_test_handler, /* ticks */ 0);
    if (sc != RTEMS_SUCCESSFUL)
        goto unregister;

    // enqueue a command and wait for handler to process it
    rc = cmd_enqueue_cb(&cmdt.cmd, handled_cmd_cb, &cmdt);
    if (rc)
        goto stop_task;
    rtems_event_receive(RTEMS_EVENT_0 | RTEMS_EVENT_1, RTEMS_EVENT_ALL,
                        CMD_TEST_TIMEOUT_TICKS, &events);

    // check results
    if (cmdt.status != CMD_STATUS_SUCCESS || !cmdt.handled_cmd)
        rc = 1;

stop_task:
    sc = cmd_handle_task_destroy();
    if (sc != RTEMS_SUCCESSFUL)
        rc = 1;
unregister:
    cmd_handled_unregister_cb();
    return rc;
}

int test_command()
{
    rtems_status_code sc;
    rtems_id task_id;
    rtems_name task_name = rtems_build_name('C','M','D','H');
    int rc;

    test_begin("test_command");
    // create the command handle task
    sc = rtems_task_create(
        task_name, 1, RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &task_id
    );
    assert(sc == RTEMS_SUCCESSFUL);
    rc = do_test(task_id);
    test_end("test_command", rc);
    return rc;
}
