#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <unistd.h>

// libhpsc
#include <command.h>

// Compatible with cmd_handler_t function
ssize_t server_process(struct cmd *cmd, void *reply, size_t reply_sz);

#endif // SERVER_H
