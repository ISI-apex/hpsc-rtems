#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <rtems.h>
#include <rtems/bspIo.h>

#include "command.h"
#include "link.h"


// caller responsible for setting rctx tid_requester
static size_t _link_request_send(struct link *link, void *buf, size_t sz,
                                 rtems_interval ticks)
{
    rtems_event_set events = 0;
    size_t rc;

    link->rctx.tx_acked = false;
    rc = link->write(link, buf, sz);
    if (rc) {
        printk("%s: request: waiting for ACK...\n", link->name);
        rtems_event_receive(link->rctx.event_wait, RTEMS_EVENT_ANY, ticks,
                            &events);
        if (events & link->rctx.event_wait) {
            printk("%s: request: ACK received\n", link->name);
            assert(link->rctx.tx_acked);
        } else {
            printk("%s: request: timed out waiting for ACK...\n", link->name);
            rc = 0;
        }
    }
    return rc;
}

// caller responsible for setting rctx reply-related / tid_requester fields
static ssize_t _link_request_recv(struct link *link, rtems_interval ticks)
{
    rtems_event_set events = 0;
    ssize_t rc;

    printk("%s: request: waiting for reply...\n", link->name);
    rtems_event_receive(link->rctx.event_wait, RTEMS_EVENT_ANY, ticks, &events);
    if (events & link->rctx.event_wait) {
        printk("%s: request: reply received\n", link->name);
        rc = link->rctx.reply_sz_read;
    } else {
        printk("%s: request: timed out waiting for reply...\n", link->name);
        rc = -2;
    }
    link->rctx.tid_requester = RTEMS_ID_NONE;
    return rc;
}

size_t link_request_send(struct link *link, void *buf, size_t sz,
                         rtems_interval ticks, rtems_event_set event_wait)
{
    size_t rc;
    link->rctx.tid_requester = rtems_task_self();
    link->rctx.event_wait = event_wait;
    rc = _link_request_send(link, buf, sz, ticks);
    link->rctx.tid_requester = RTEMS_ID_NONE;
    return rc;
}

ssize_t link_request(struct link *link,
                     rtems_interval wtimeout_ticks, void *wbuf, size_t wsz,
                     rtems_interval rtimeout_ticks, void *rbuf, size_t rsz,
                     rtems_event_set event_wait)
{
    size_t rc;
    printk("%s: request\n", link->name);
    link->rctx.reply_sz_read = 0;
    link->rctx.reply = rbuf;
    link->rctx.reply_sz = rsz;
    link->rctx.tid_requester = rtems_task_self();
    link->rctx.event_wait = event_wait;
    rc = _link_request_send(link, wbuf, wsz, wtimeout_ticks);
    if (!rc)
        return -1;
    return _link_request_recv(link, rtimeout_ticks);
}

int link_disconnect(struct link *link)
{
    return link->close(link);
}

void link_init(struct link *link, const char *name, void *priv)
{
    link->rctx.tid_requester = RTEMS_ID_NONE;
    link->rctx.tx_acked = false;
    link->rctx.reply = NULL;
    link->name = name;
    link->priv = priv;
}

void link_recv_cmd(void *arg)
{
    struct link *link = arg;
    struct cmd cmd = {
        .link = link,
        .msg = { 0 }
    };
    printk("%s: recv_cmd\n", link->name);
    link->read(link, cmd.msg, sizeof(cmd.msg));
    if (cmd_enqueue(&cmd))
        rtems_panic("%s: recv_cmd: failed to enqueue command", link->name);
}

void link_recv_reply(void *arg)
{
    struct link *link = arg;
    rtems_status_code sc;
    printk("%s: recv_reply\n", link->name);
    link->rctx.reply_sz_read = link->read(link, link->rctx.reply,
                                          link->rctx.reply_sz);
    if (link->rctx.tid_requester != RTEMS_ID_NONE) {
        sc = rtems_event_send(link->rctx.tid_requester, link->rctx.event_wait);
        if (sc != RTEMS_SUCCESSFUL)
            // there was a race with reply timeout and clearing tid_requester
            rtems_panic("%s: recv_reply: failed to send reply to listening task",
                        link->name);
    }
}

void link_ack(void *arg)
{
    struct link *link = arg;
    rtems_status_code sc;
    printk("%s: ACK\n", link->name);
    link->rctx.tx_acked = true;
    if (link->rctx.tid_requester != RTEMS_ID_NONE) {
        sc = rtems_event_send(link->rctx.tid_requester, link->rctx.event_wait);
        if (sc != RTEMS_SUCCESSFUL)
            // there was a race with send timeout and clearing tid_requester
            rtems_panic("%s: ACK: failed to send ACK to listening task",
                        link->name);
    }
}
