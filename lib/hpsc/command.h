#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtems.h>

#include "hpsc-msg.h"
#include "link.h"

#define CMD_TIMEOUT_TICKS_SEND 10
#define CMD_TIMEOUT_TICKS_RECV 10
// timeout prevent hangs when remotes fail
#define CMD_TIMEOUT_TICKS_REPLY 1000

typedef enum {
    CMD_STATUS_SUCCESS,
    CMD_STATUS_NO_HANDLER,
    CMD_STATUS_HANDLER_FAILED,
    CMD_STATUS_REPLY_FAILED,
    CMD_STATUS_ACK_FAILED,
    CMD_STATUS_UNKNOWN
} cmd_status;

struct cmd {
    uint8_t msg[HPSC_MSG_SIZE];
    struct link *link;
};

typedef ssize_t (cmd_handler_t)(struct cmd *cmd, void *reply, size_t reply_sz);
typedef void (cmd_handled_t)(void *arg, cmd_status status);

void cmd_handler_register(cmd_handler_t *cb);
void cmd_handler_unregister(void);

void cmd_handled_register_cb(cmd_handled_t *cb, void *cb_arg);
void cmd_handled_unregister_cb(void);

int cmd_enqueue_cb(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg);
int cmd_enqueue(struct cmd *cmd);

rtems_status_code cmd_handle_task_start(rtems_id task_id);
rtems_status_code cmd_handle_task_destroy(void);

#endif // COMMAND_H
