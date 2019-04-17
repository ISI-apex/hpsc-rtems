#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <unistd.h>

#include <rtems.h>

struct link_request_ctx {
    // listens for: RTEMS_EVENT_0 (ACK), RTEMS_EVENT_1 (reply)
    rtems_id tid_requester;
    bool tx_acked;
    uint32_t *reply;
    size_t reply_sz;
    size_t reply_sz_read;
};

/**
 * Link implementations populate this struct.
 * Link users use the functions described below, NOT the function pointers.
 */
struct link {
    struct link_request_ctx rctx;
    const char *name;
    void *priv;
    size_t (*write)(struct link *link, void *buf, size_t sz);
    size_t (*read)(struct link *link, void *buf, size_t sz);
    int (*close)(struct link *link);
};

/*
 * These functions are for link users
 */
size_t link_send(struct link *link, void *buf, size_t sz);
bool link_is_send_acked(struct link *link);
// returns -1 on send failure, 0 on read timeout, or number of bytes read
ssize_t link_request(struct link *link,
                     int wtimeout_ms, void *wbuf, size_t wsz,
                     int rtimeout_ms, void *rbuf, size_t rsz);
int link_disconnect(struct link *link);

/*
 * These functions are for link implementations
 */
void link_init(struct link *link, const char *name, void *priv);
// the following are compatible with rtems_interrupt_handler, expect a link arg
void link_recv_cmd(void *arg);
void link_recv_reply(void *arg);
void link_ack(void *arg);

#endif // LINK_H
