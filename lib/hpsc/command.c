#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>
#include <rtems/bspIo.h>

#include "command.h"
#include "link.h"
#include "hpsc-msg.h"

#define CMDQ_LEN 4

struct cmd_handled_ctx {
    cmd_handled_t *cb;
    void *cb_arg;
};

struct cmd_handler_ctx {
    rtems_id tid;
    cmd_handler_t *cb;
    rtems_interval timeout_ticks;
    bool running;
};

struct cmdq_item {
    struct cmd cmd;
    struct cmd_handled_ctx handled;
};

struct cmdq {
    struct cmdq_item q[CMDQ_LEN];
    pthread_spinlock_t lock; // no init is necessary
    size_t head;
    size_t tail;
};

static struct cmdq cmdq = { 0 };

static struct cmd_handler_ctx cmd_handler = {
    .tid = RTEMS_ID_NONE,
    .cb = NULL,
    // timeout prevents hanging when remotes fail
    .timeout_ticks = 0,
    .running = false
};

static struct cmd_handled_ctx cmd_handled = {
    .cb = NULL,
    .cb_arg = NULL
};


static void lock(pthread_spinlock_t *lock)
{
    int err = pthread_spin_lock(lock);
    assert(!err);
}

static void unlock(pthread_spinlock_t *lock)
{
    int err = pthread_spin_unlock(lock);
    assert(!err);
}

static size_t cmdq_size_unsafe(void)
{
    return (cmdq.head >= cmdq.tail) ? (cmdq.head - cmdq.tail) :
        (CMDQ_LEN - cmdq.tail + cmdq.head);
}

static int cmd_enqueue_cb_unsafe(struct cmd *cmd, cmd_handled_t *cb,
                                 void *cb_arg)
{
    assert(cmd);
    if (cmdq.head + 1 % CMDQ_LEN == cmdq.tail) {
        printk("command: enqueue failed: queue full\n");
        return 1;
    }
    cmdq.head = (cmdq.head + 1) % CMDQ_LEN;
    memcpy(&cmdq.q[cmdq.head].cmd, cmd, sizeof(struct cmd));
    cmdq.q[cmdq.head].handled.cb = cb;
    cmdq.q[cmdq.head].handled.cb_arg = cb_arg;
    printk("command: enqueue (tail %u head %u): cmd %u arg %u...\n",
           cmdq.tail, cmdq.head,
           cmdq.q[cmdq.head].cmd.msg[0],
           cmdq.q[cmdq.head].cmd.msg[HPSC_MSG_PAYLOAD_OFFSET]);
    return 0;
}

static int cmd_dequeue_unsafe(struct cmd *cmd, cmd_handled_t **cb,
                              void **cb_arg)
{
    assert(cmd);
    assert(cb);
    assert(cb_arg);
    if (cmdq.head == cmdq.tail)
        return 1;
    cmdq.tail = (cmdq.tail + 1) % CMDQ_LEN;
    memcpy(cmd, &cmdq.q[cmdq.tail].cmd, sizeof(struct cmd));
    *cb = cmdq.q[cmdq.tail].handled.cb;
    *cb_arg = cmdq.q[cmdq.tail].handled.cb_arg;
    printk("command: dequeue (tail %u head %u): cmd %u arg %u...\n",
           cmdq.tail, cmdq.head,
           cmdq.q[cmdq.tail].cmd.msg[0],
           cmdq.q[cmdq.tail].cmd.msg[HPSC_MSG_PAYLOAD_OFFSET]);
    return 0;
}

static void cmd_handle(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg)
{
    HPSC_MSG_DEFINE(reply);
    ssize_t reply_sz;
    size_t rc;
    cmd_status status = CMD_STATUS_SUCCESS;
    assert(cmd);
    assert(cmd_handler.cb);

    printk("command: handle: cmd %u arg %u...\n",
           cmd->msg[0], cmd->msg[HPSC_MSG_PAYLOAD_OFFSET]);

    reply_sz = cmd_handler.cb(cmd, reply, sizeof(reply));
    if (reply_sz < 0) {
        printk("ERROR: command: handle: server failed to process request\n");
        status = CMD_STATUS_HANDLER_FAILED;
        goto out;
    }
    if (!reply_sz) {
        printk("command: handle: server did not produce a reply\n");
        goto out;
    }

    printk("command: handle: %s: reply %u arg %u...\n", cmd->link->name,
           reply[0], reply[HPSC_MSG_PAYLOAD_OFFSET]);
    rc = link_request_send(cmd->link, reply, sizeof(reply),
                           cmd_handler.timeout_ticks);
    if (!rc) {
        printk("command: handle: %s: failed to send reply\n", cmd->link->name);
        status = CMD_STATUS_REPLY_FAILED;
    }

out:
    if (cb)
        cb(cb_arg, status);
    if (cmd_handled.cb)
        cmd_handled.cb(cmd_handled.cb_arg, status);
}

void cmd_handled_register_cb(cmd_handled_t *cb, void *cb_arg)
{
    cmd_handled.cb = cb;
    cmd_handled.cb_arg = cb_arg;
}

void cmd_handled_unregister_cb(void)
{
    cmd_handled.cb = NULL;
    cmd_handled.cb_arg = NULL;
}

int cmd_enqueue_cb(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg)
{
    int rc;
    lock(&cmdq.lock);
    rc = cmd_enqueue_cb_unsafe(cmd, cb, cb_arg);
    unlock(&cmdq.lock);
    if (!rc && cmd_handler.tid != RTEMS_ID_NONE)
        rc = rtems_event_send(cmd_handler.tid, RTEMS_EVENT_0) != RTEMS_SUCCESSFUL;
    return rc;
}

int cmd_enqueue(struct cmd *cmd)
{
    return cmd_enqueue_cb(cmd, NULL, NULL);
}

static int cmd_dequeue(struct cmd *cmd, cmd_handled_t **cb, void **cb_arg)
{
    int rc;
    lock(&cmdq.lock);
    rc = cmd_dequeue_unsafe(cmd, cb, cb_arg);
    unlock(&cmdq.lock);
    return rc;
}

static size_t cmd_flush(void)
{
    struct cmd cmd;
    cmd_handled_t *cb;
    void *cb_arg;
    size_t i = 0;
    while (!cmd_dequeue(&cmd, &cb, &cb_arg)) {
        cmd_handle(&cmd, cb, cb_arg);
        i++;
    }
    return i;
}

size_t cmd_drop_all(void)
{
    size_t qsize;
    lock(&cmdq.lock);
    qsize = cmdq_size_unsafe();
    cmdq.tail = cmdq.head;
    unlock(&cmdq.lock);
    return qsize;
}

static rtems_task cmd_handle_task(rtems_task_argument ignored)
{
    rtems_event_set events;
    size_t i = 0;
    while (1) {
        printk("[%zu] Waiting for command...\n", i);
        i += cmd_flush();
        events = 0;
        // RTEMS_EVENT_0: process a command
        // RTEMS_EVENT_1: exit handler task
        rtems_event_receive(RTEMS_EVENT_0 | RTEMS_EVENT_1, RTEMS_EVENT_ANY,
                            RTEMS_NO_TIMEOUT, &events);
        if (events & RTEMS_EVENT_1)
            // any queued events won't be processed until handler is restarted
            break;
    }
    cmd_handler.running = false;
    rtems_task_exit();
}

static void cmd_handler_set(rtems_id tid, cmd_handler_t cb,
                            rtems_interval timeout_ticks, bool running)
{
    cmd_handler.tid = tid;
    cmd_handler.cb = cb;
    cmd_handler.timeout_ticks = timeout_ticks;
    cmd_handler.running = running;
}

rtems_status_code cmd_handle_task_start(rtems_id task_id, cmd_handler_t cb,
                                        rtems_interval timeout_ticks)
{
    rtems_status_code sc;
    assert(!cmd_handler.running);
    assert(task_id != RTEMS_ID_NONE);
    assert(cb);
    cmd_handler_set(task_id, cb, timeout_ticks, true);
    sc = rtems_task_start(cmd_handler.tid, cmd_handle_task, 1);
    if (sc != RTEMS_SUCCESSFUL)
        cmd_handler_set(RTEMS_ID_NONE, NULL, 0, false);
    return sc;
}

rtems_status_code cmd_handle_task_destroy(void)
{
    rtems_status_code sc = RTEMS_NOT_DEFINED;
    if (cmd_handler.running) {
        sc = rtems_event_send(cmd_handler.tid, RTEMS_EVENT_1);
        if (sc == RTEMS_SUCCESSFUL) {
            while (cmd_handler.running); // wait for task to finish
            cmd_handler_set(RTEMS_ID_NONE, NULL, 0, false);
        }
    }
    return sc;
}
