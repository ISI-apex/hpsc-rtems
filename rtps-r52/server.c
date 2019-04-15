#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// libhpsc
#include <command.h>
#include <server.h>

int server_process(struct cmd *cmd, void *reply, size_t reply_sz)
{
    assert(reply_sz <= HPSC_MSG_SIZE);
    switch (cmd->msg[0]) {
        case NOP:
            // do nothing and reply nothing command
            return 0;
        case PING:
            printf("PING ...\n");
            hpsc_msg_pong(reply, reply_sz, &cmd->msg[HPSC_MSG_PAYLOAD_OFFSET],
                          HPSC_MSG_PAYLOAD_SIZE);
            return reply_sz;
        case PONG:
            printf("PONG ...\n");
            return 0;
        default:
            printf("ERROR: unknown cmd: %x\n", cmd->msg[0]);
            return -1;
    }
}
