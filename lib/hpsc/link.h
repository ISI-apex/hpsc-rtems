#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <unistd.h>

#include <rtems.h>

struct link_request_ctx {
    rtems_id tid_requester;
    rtems_event_set event_wait;
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
    volatile struct link_request_ctx rctx; // may be modified in interrupts
    const char *name;
    void *priv;
    size_t (*write)(struct link *link, void *buf, size_t sz);
    size_t (*read)(struct link *link, void *buf, size_t sz);
    int (*close)(struct link *link);
};

/*
 * These functions are for link users
 */
/**
 * Send a message and wait for an ACK, but not a reply message.
 * Use RTEMS_NO_TIMEOUT for tick parameters to wait forever.
 * The event_wait value should be a single event not in use by the calling task.
 * Returns 0 on send failure or timeout, or number of bytes read.
 */
size_t link_request_send(struct link *link, void *buf, size_t sz,
                         rtems_interval ticks, rtems_event_set event_wait);
/**
 * Send a message and wait for a reply.
 * Use RTEMS_NO_TIMEOUT for tick parameters to wait forever.
 * The event_wait value should be a single event not in use by the calling task.
 * Returns -1 on send failure or timeout, -2 on read timeout, or number of bytes
 * read.
 */
ssize_t link_request(struct link *link,
                     rtems_interval wtimeout_ticks, void *wbuf, size_t wsz,
                     rtems_interval rtimeout_ticks, void *rbuf, size_t rsz,
                     rtems_event_set event_wait);
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
