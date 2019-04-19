#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rtems.h>
#include <rtems/bspIo.h>

#include "command.h"
#include "hpsc-msg.h"

#define CMDQ_LEN 4

struct cmdq_item {
    struct cmd cmd;
    cmd_handled_t *cb;
    void *cb_arg;
};

static size_t cmdq_head = 0;
static size_t cmdq_tail = 0;
static struct cmdq_item cmdq[CMDQ_LEN];
static pthread_spinlock_t cmdq_lock; // no init is necessary

static rtems_id cmdh_task_id = RTEMS_ID_NONE;

static cmd_handler_t *cmd_handler = NULL;

static cmd_handled_t *cmd_handled_cb = NULL;
static void *cmd_handled_cb_arg = NULL;

void cmd_handler_register(cmd_handler_t cb)
{
    assert(!cmd_handler);
    cmd_handler = cb;
}

void cmd_handler_unregister(void)
{
    cmd_handler = NULL;
}

int cmd_enqueue_cb(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg)
{
    int err;
    int rc = 0;
    assert(cmd);
    assert(cmdh_task_id != RTEMS_ID_NONE);

    err = pthread_spin_lock(&cmdq_lock);
    assert(!err);
    if (cmdq_head + 1 % CMDQ_LEN == cmdq_tail) {
        printk("command: enqueue failed: queue full\n");
        rc = 1;
        goto out;
    }
    cmdq_head = (cmdq_head + 1) % CMDQ_LEN;

    memcpy(&cmdq[cmdq_head].cmd, cmd, sizeof(struct cmd));
    cmdq[cmdq_head].cb = cb;
    cmdq[cmdq_head].cb_arg = cb_arg;
    printk("command: enqueue (tail %u head %u): cmd %u arg %u...\n",
           cmdq_tail, cmdq_head,
           cmdq[cmdq_head].cmd.msg[0],
           cmdq[cmdq_head].cmd.msg[HPSC_MSG_PAYLOAD_OFFSET]);

out:
    err = pthread_spin_unlock(&cmdq_lock);
    assert(!err);
    if (!rc)
        rc = rtems_event_send(cmdh_task_id, RTEMS_EVENT_0) != RTEMS_SUCCESSFUL;
    return rc;
}

void cmd_handled_register_cb(cmd_handled_t *cb, void *cb_arg)
{
    cmd_handled_cb = cb;
    cmd_handled_cb_arg = cb_arg;
}

void cmd_handled_unregister_cb(void)
{
    cmd_handled_cb = NULL;
    cmd_handled_cb_arg = NULL;
}

int cmd_enqueue(struct cmd *cmd)
{
    return cmd_enqueue_cb(cmd, NULL, NULL);
}

static int cmd_dequeue(struct cmd *cmd, cmd_handled_t **cb, void **cb_arg)
{
    int err;
    int rc = 0;
    assert(cmd);
    assert(cb);
    assert(cb_arg);

    err = pthread_spin_lock(&cmdq_lock);
    assert(!err);
    if (cmdq_head == cmdq_tail) {
        rc = 1;
        goto out;
    }
    cmdq_tail = (cmdq_tail + 1) % CMDQ_LEN;

    memcpy(cmd, &cmdq[cmdq_tail].cmd, sizeof(struct cmd));
    *cb = cmdq[cmdq_tail].cb;
    *cb_arg = cmdq[cmdq_tail].cb_arg;
    printf("command: dequeue (tail %u head %u): cmd %u arg %u...\n",
           cmdq_tail, cmdq_head,
           cmdq[cmdq_tail].cmd.msg[0],
           cmdq[cmdq_tail].cmd.msg[HPSC_MSG_PAYLOAD_OFFSET]);

out:
    err = pthread_spin_unlock(&cmdq_lock);
    assert(!err);
    return rc;
}

static void cmd_handle(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg)
{
    HPSC_MSG_DEFINE(reply);
    int reply_sz;
    size_t rc;
    cmd_status status = CMD_STATUS_SUCCESS;
    rtems_interval sleep_ticks_rem = CMD_TIMEOUT_TICKS_REPLY;
    assert(cmd);

    printf("command: handle: cmd %u arg %u...\n",
           cmd->msg[0], cmd->msg[HPSC_MSG_PAYLOAD_OFFSET]);

    if (!cmd_handler) {
        printf("command: handle: no handler registered\n");
        status = CMD_STATUS_NO_HANDLER;
        goto out;
    }

    reply_sz = cmd_handler(cmd, reply, sizeof(reply));
    if (reply_sz < 0) {
        printf("ERROR: command: handle: server failed to process request\n");
        status = CMD_STATUS_HANDLER_FAILED;
        goto out;
    }
    if (!reply_sz) {
        printf("command: handle: server did not produce a reply\n");
        goto out;
    }

    printf("command: handle: %s: reply %u arg %u...\n", cmd->link->name,
           reply[0], reply[HPSC_MSG_PAYLOAD_OFFSET]);

    rc = link_send(cmd->link, reply, sizeof(reply));
    if (!rc) {
        printf("command: handle: %s: failed to send reply\n", cmd->link->name);
        status = CMD_STATUS_REPLY_FAILED;
        goto out;
    }
    printf("command: handle: %s: waiting for ACK for our reply\n",
           cmd->link->name);
    do {
        if (link_is_send_acked(cmd->link)) {
            printf("command: handle: %s: ACK for our reply received\n", 
                   cmd->link->name);
            break;
        }
        if (!sleep_ticks_rem) {
            printf("command: handle: %s: timed out waiting for ACK\n",
                   cmd->link->name);
            status = CMD_STATUS_ACK_FAILED;
            break;
        }
        rtems_task_wake_after(CMD_TIMEOUT_TICKS_RECV);
        sleep_ticks_rem -= sleep_ticks_rem < CMD_TIMEOUT_TICKS_RECV ?
                           sleep_ticks_rem : CMD_TIMEOUT_TICKS_RECV;
    } while (1);

out:
    if (cb)
        cb(cb_arg, status);
    if (cmd_handled_cb)
        cmd_handled_cb(cmd_handled_cb_arg, status);
}

static rtems_task cmd_handle_task(rtems_task_argument ignored)
{
    struct cmd cmd;
    cmd_handled_t *cb;
    void *cb_arg;
    rtems_event_set events;
    unsigned i = 0;
    printk("Starting command handler task\n");
    while (1) {
        events = 0;
        printk("[%u] Waiting for command...\n", i);
        // RTEMS_EVENT_0: process a command
        // RTEMS_EVENT_1: exit handler task
        rtems_event_receive(RTEMS_EVENT_0 | RTEMS_EVENT_1, RTEMS_EVENT_ANY,
                            RTEMS_NO_TIMEOUT, &events);
        // process queued events first, even if we're ordered to exit, otherwise
        // commands remain stuck in the queue after event is cleared
        while (!cmd_dequeue(&cmd, &cb, &cb_arg)) {
            cmd_handle(&cmd, cb, cb_arg);
            i++;
        }
        if (events & RTEMS_EVENT_1)
            break;
    }
    printk("Exiting command handler task\n");
    rtems_task_exit();
}

rtems_status_code cmd_handle_task_start(rtems_id task_id)
{
    cmdh_task_id = task_id;
    return rtems_task_start(cmdh_task_id, cmd_handle_task, 1);
}

rtems_status_code cmd_handle_task_destroy(void)
{
    rtems_status_code sc;
    if (cmdh_task_id == RTEMS_ID_NONE)
        return RTEMS_NOT_DEFINED;
    sc = rtems_event_send(cmdh_task_id, RTEMS_EVENT_1);
    cmdh_task_id = RTEMS_ID_NONE;
    return sc;
}
