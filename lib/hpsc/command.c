#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <rtems.h>
#include <rtems/bspIo.h>

#include "command.h"

#define CMD_QUEUE_LEN 4
#define REPLY_SIZE CMD_MSG_SZ

static size_t cmdq_head = 0;
static size_t cmdq_tail = 0;
static struct cmd cmdq[CMD_QUEUE_LEN];

static rtems_id cmdh_task_id = RTEMS_ID_NONE;

static cmd_handler_t *cmd_handler = NULL;

void cmd_handler_register(cmd_handler_t cb)
{
    assert(!cmd_handler);
    cmd_handler = cb;
}

void cmd_handler_unregister(void)
{
    cmd_handler = NULL;
}

int cmd_enqueue(struct cmd *cmd)
{
    assert(cmd);
    assert(cmdh_task_id != RTEMS_ID_NONE);
    if (cmdq_head + 1 % CMD_QUEUE_LEN == cmdq_tail) {
        printk("command: enqueue failed: queue full\n");
        return 1;
    }
    cmdq_head = (cmdq_head + 1) % CMD_QUEUE_LEN;

    memcpy(&cmdq[cmdq_head], cmd, sizeof(struct cmd));
    printk("command: enqueue (tail %u head %u): cmd %u arg %u...\n",
           cmdq_tail, cmdq_head,
           cmdq[cmdq_head].msg[0], cmdq[cmdq_head].msg[CMD_MSG_PAYLOAD_OFFSET]);

    if (rtems_event_send(cmdh_task_id, RTEMS_EVENT_0) == RTEMS_SUCCESSFUL)
        return 0;
    return 1;
    // TODO: SEV (to prevent race between queue check and WFE in main loop)
    // return 0;
}

static int cmd_dequeue(struct cmd *cmd)
{
    assert(cmd);
    if (cmdq_head == cmdq_tail)
        return 1;
    cmdq_tail = (cmdq_tail + 1) % CMD_QUEUE_LEN;

    memcpy(cmd, &cmdq[cmdq_tail], sizeof(struct cmd));
    printf("command: dequeue (tail %u head %u): cmd %u arg %u...\n",
           cmdq_tail, cmdq_head,
           cmdq[cmdq_tail].msg[0], cmdq[cmdq_tail].msg[CMD_MSG_PAYLOAD_OFFSET]);
    return 0;
}

static void ms_to_ts(struct timespec *ts, uint32_t ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000;
}

static void cmd_handle(struct cmd *cmd)
{
    uint8_t reply[REPLY_SIZE] = { 0 };
    int reply_sz;
    size_t rc;
    uint32_t sleep_ms_rem = CMD_TIMEOUT_MS_REPLY;
    struct timespec ts;
    assert(cmd);

    printf("command: handle: cmd %u arg %u...\n",
           cmd->msg[0], cmd->msg[CMD_MSG_PAYLOAD_OFFSET]);

    if (!cmd_handler) {
        printf("command: handle: no handler registered\n");
        return;
    }

    reply_sz = cmd_handler(cmd, reply, sizeof(reply));
    if (reply_sz < 0) {
        printf("ERROR: command: handle: server failed to process request\n");
        return;
    }
    if (!reply_sz) {
        printf("command: handle: server did not produce a reply\n");
        return;
    }

    printf("command: handle: %s: reply %u arg %u...\n", cmd->link->name,
           reply[0], reply[CMD_MSG_PAYLOAD_OFFSET]);

    rc = cmd->link->send(cmd->link, CMD_TIMEOUT_MS_SEND, reply, sizeof(reply));
    if (!rc) {
        printf("command: handle: %s: failed to send reply\n", cmd->link->name);
    } else {
        printf("command: handle: %s: waiting for ACK for our reply\n",
               cmd->link->name);
        ms_to_ts(&ts, CMD_TIMEOUT_MS_RECV);
        do {
            if (cmd->link->is_send_acked(cmd->link)) {
                printf("command: handle: %s: ACK for our reply received\n", 
                       cmd->link->name);
                break;
            }
            if (sleep_ms_rem) {
                nanosleep(&ts, NULL);
                sleep_ms_rem -= sleep_ms_rem < CMD_TIMEOUT_MS_RECV ?
                    sleep_ms_rem : CMD_TIMEOUT_MS_RECV;
            } else {
                printf("command: handle: %s: timed out waiting for ACK\n",
                       cmd->link->name);
                break;
            }
        } while (1);
    }
}

static rtems_task cmd_handle_task(rtems_task_argument ignored)
{
    struct cmd cmd;
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
        while (!cmd_dequeue(&cmd)) {
            cmd_handle(&cmd);
            i++;
        }
        if (events & RTEMS_EVENT_1)
            break;
    }
    printk("Exiting command handler task\n");
    rtems_task_exit();
}

rtems_status_code cmd_handle_task_create(void)
{
    rtems_name task_name = rtems_build_name('C','M','D','H');
    rtems_status_code sc = rtems_task_create(
        task_name, 1, RTEMS_MINIMUM_STACK_SIZE * 2,
        RTEMS_DEFAULT_MODES,
        RTEMS_FLOATING_POINT | RTEMS_DEFAULT_ATTRIBUTES, &cmdh_task_id
    );
    if (sc != RTEMS_SUCCESSFUL) {
        printf("Failed to create command handler task\n");
    } else {
        sc = rtems_task_start(cmdh_task_id, cmd_handle_task, 1);
        if (sc != RTEMS_SUCCESSFUL)
            printf("Failed to start command handler task\n");
    }
    return sc;
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
