#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>

#include "command.h"

// Compatible with cmd_handler_t function
int server_process(struct cmd *cmd, void *reply, size_t reply_sz);

#endif // SERVER_H
