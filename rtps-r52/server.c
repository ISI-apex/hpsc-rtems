#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../hpsc/command.h"
#include "../hpsc/server.h"

int server_process(struct cmd *cmd, void *reply, size_t reply_sz)
{
    size_t i;
    uint8_t *reply_u8 = (uint8_t *)reply;
    assert(reply_sz <= CMD_MSG_SZ);
    switch (cmd->msg[0]) {
        case CMD_NOP:
            // do nothing and reply nothing command
            return 0;
        case CMD_PING:
            printf("PING ...\n");
            reply_u8[0] = CMD_PONG;
            for (i = 1; i < CMD_MSG_PAYLOAD_OFFSET && i < reply_sz; i++)
                reply_u8[i] = 0;
            for (i = CMD_MSG_PAYLOAD_OFFSET; i < reply_sz; i++)
                reply_u8[i] = cmd->msg[i];
            return reply_sz;
        case CMD_PONG:
            printf("PONG ...\n");
            return 0;
        default:
            printf("ERROR: unknown cmd: %x\n", cmd->msg[0]);
            return -1;
    }
}
