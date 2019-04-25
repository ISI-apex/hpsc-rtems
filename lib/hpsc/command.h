#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtems.h>

#include "hpsc-msg.h"
#include "link.h"

typedef enum {
    CMD_STATUS_SUCCESS,
    CMD_STATUS_HANDLER_FAILED,
    CMD_STATUS_REPLY_FAILED,
    CMD_STATUS_UNKNOWN
} cmd_status;

struct cmd {
    uint8_t msg[HPSC_MSG_SIZE];
    struct link *link;
};

typedef ssize_t (cmd_handler_t)(struct cmd *cmd, void *reply, size_t reply_sz);
typedef void (cmd_handled_t)(void *arg, cmd_status status);

/**
 * Register a callback handler to be run after every queued command is handled.
 * This operation is not synchronized with cmd_handle_task, so don't use after
 * cmd_handle_task is started unless the command queue is empty and guaranteed 
 * not to enqueue new commands until operation completes.
 */
void cmd_handled_register_cb(cmd_handled_t *cb, void *cb_arg);

/**
 * Unregister the callback handler.
 * This operation is not synchronized with cmd_handle_task, so don't use after
 * cmd_handle_task is started unless the command queue is empty and guaranteed 
 * not to enqueue new commands until operation completes.
 */
void cmd_handled_unregister_cb(void);

/**
 * Enqueue a command with its own callback handler (cmd struct will be copied).
 * The specified callback handler will be executed before the global handler.
 */
int cmd_enqueue_cb(struct cmd *cmd, cmd_handled_t *cb, void *cb_arg);

/**
 * Enqueue a command (cmd struct will be copied).
 */
int cmd_enqueue(struct cmd *cmd);

/**
 * Drop all commands in the queue without handling them.
 */
size_t cmd_drop_all(void);

/**
 * Start the cmd_handle_task (task must be created by the caller).
 */
rtems_status_code cmd_handle_task_start(rtems_id task_id, cmd_handler_t cb,
                                        rtems_interval timeout_ticks);

/**
 * Stop the cmd_handle_task.
 */
rtems_status_code cmd_handle_task_destroy(void);

#endif // COMMAND_H
