#ifndef LINK_CMD_LISTENER_H
#define LINK_CMD_LISTENER_H

#include <stdbool.h>
#include <unistd.h>

#include "cmd.h"
#include "link.h"

int link_cmd_listener(struct link *link, void *buf, size_t len);

#endif // LINK_CMD_LISTENER_H
