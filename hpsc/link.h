#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <unistd.h>

/**
 * The link struct is effectively an API, which can be populated by other
 * link-like interfaces.
 */
struct link {
    void *priv;
    const char *name;
    int (*disconnect)(struct link *link);
    // returns 0 on timeout, or positive value for number of bytes sent
    int (*send)(struct link *link, int timeout_ms, void *buf, size_t sz);
    bool (*is_send_acked)(struct link *link);
    // returns -1 on send failure, 0 on read timeout, or number of bytes read
    int (*request)(struct link *link,
                   int wtimeout_ms, void *wbuf, size_t wsz,
                   int rtimeout_ms, void *rbuf, size_t rsz);
    // recv not used for interrupt-based exchange mechanisms
    // returns 0 if no data, or number of bytes received
    int (*recv)(struct link *link, void *buf, size_t sz);
};

#endif // LINK_H
