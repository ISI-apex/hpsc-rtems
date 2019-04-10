#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

// drivers
#include <hpsc-mbox.h>

#include "command.h"
#include "link.h"
#include "mailbox-link.h"

struct cmd_ctx {
    // listens for: RTEMS_EVENT_0 (ACK), RTEMS_EVENT_1 (reply)
    rtems_id tid_requester;
    volatile bool tx_acked;
    uint32_t *reply;
    size_t reply_sz;
    size_t reply_sz_read;
};

struct mbox_link {
    struct hpsc_mbox_chan *chan_from;
    struct hpsc_mbox_chan *chan_to;
    struct cmd_ctx cmd_ctx;
};


static void handle_ack(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    rtems_status_code sc;
    printk("%s: handle_ack\n", link->name);
    mlink->cmd_ctx.tx_acked = true;
    // only used internally for request
    if (mlink->cmd_ctx.tid_requester != RTEMS_ID_NONE) {
        sc = rtems_event_send(mlink->cmd_ctx.tid_requester, RTEMS_EVENT_0);
        if (sc != RTEMS_SUCCESSFUL)
            // there was a race with send timeout and clearing tid_requester
            rtems_panic("handle_ack: failed to send ACK to listening task");
    }
}

static void handle_cmd(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    struct cmd cmd = {
        .link = link,
        .msg = { 0 }
    };
    printk("%s: handle_cmd\n", link->name);
    // read never fails if sizeof(cmd.msg) > 0
    hpsc_mbox_chan_read(mlink->chan_from, cmd.msg, sizeof(cmd.msg));
    if (cmd_enqueue(&cmd))
        rtems_panic("handle_cmd: failed to enqueue command");
}

static void handle_reply(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    rtems_status_code sc;
    printk("%s: handle_reply\n", link->name);
    mlink->cmd_ctx.reply_sz_read = hpsc_mbox_chan_read(mlink->chan_from,
                                                       mlink->cmd_ctx.reply,
                                                       mlink->cmd_ctx.reply_sz);
    // only used internally for request
    if (mlink->cmd_ctx.tid_requester != RTEMS_ID_NONE) {
        sc = rtems_event_send(mlink->cmd_ctx.tid_requester, RTEMS_EVENT_1);
        if (sc != RTEMS_SUCCESSFUL)
            // there was a race with reply timeout and clearing tid_requester
            rtems_panic("handle_reply: failed to send reply to listening task");
    }
}

static int mbox_link_disconnect(struct link *link) {
    struct mbox_link *mlink = link->priv;
    int rc;
    printk("%s: disconnect\n", link->name);
    // in case of failure, keep going and fwd code
    rc = hpsc_mbox_chan_release(mlink->chan_from);
    rc |= hpsc_mbox_chan_release(mlink->chan_to);
    free(mlink);
    free(link);
    return rc;
}

static size_t mbox_link_send(struct link *link, int timeout_ms, void *buf,
                             size_t sz)
{
    struct mbox_link *mlink = link->priv;
    mlink->cmd_ctx.tx_acked = false;
    return hpsc_mbox_chan_write(mlink->chan_to, buf, sz);
}

static bool mbox_link_is_send_acked(struct link *link)
{
    struct mbox_link *mlink = link->priv;
    return mlink->cmd_ctx.tx_acked;
}

static ssize_t mbox_link_request(struct link *link,
                                 int wtimeout_ms, void *wbuf, size_t wsz,
                                 int rtimeout_ms, void *rbuf, size_t rsz)
{
    struct mbox_link *mlink = link->priv;
    rtems_interval ticks;
    rtems_event_set events;
    ssize_t rc;

    printk("%s: request\n", link->name);
    mlink->cmd_ctx.reply_sz_read = 0;
    mlink->cmd_ctx.reply = rbuf;
    mlink->cmd_ctx.reply_sz = rsz / sizeof(uint32_t);

    rc = mbox_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printk("mbox_link_request: send failed\n");
        return -1;
    }
    mlink->cmd_ctx.tid_requester = rtems_task_self();

    printk("%s: request: waiting for ACK...\n", link->name);
    ticks = wtimeout_ms < 0 ? RTEMS_NO_TIMEOUT :
        RTEMS_MILLISECONDS_TO_TICKS(wtimeout_ms);
    events = 0;
    rtems_event_receive(RTEMS_EVENT_0, RTEMS_EVENT_ANY, ticks, &events);
    if (events & RTEMS_EVENT_0) {
        printk("%s: request: ACK received\n", link->name);
        assert(mlink->cmd_ctx.tx_acked);
    } else {
        printk("%s: request: timed out waiting for ACK...\n", link->name);
        rc = -1; // send timeout (considered a send failure)
        goto out;
    }

    printk("%s: request: waiting for reply...\n", link->name);
    ticks = rtimeout_ms < 0 ? RTEMS_NO_TIMEOUT :
        RTEMS_MILLISECONDS_TO_TICKS(wtimeout_ms);
    events = 0;
    rtems_event_receive(RTEMS_EVENT_1, RTEMS_EVENT_ANY, ticks, &events);
    if (events & RTEMS_EVENT_1) {
        printk("%s: request: reply received\n", link->name);
        rc = mlink->cmd_ctx.reply_sz_read;
        assert(rc);
    } else {
        printk("%s: request: timed out waiting for reply...\n", link->name);
        rc = 0;
    }

out:
    mlink->cmd_ctx.tid_requester = RTEMS_ID_NONE;
    return rc;
}

struct link *mbox_link_connect(const char *name, struct hpsc_mbox *mbox,
                               unsigned idx_from, unsigned idx_to,
                               unsigned server, unsigned client)
{
    struct mbox_link *mlink;
    struct link *link;
    rtems_interrupt_handler rcv_cb = server ? handle_cmd : handle_reply;

    printk("%s: connect\n", name);
    link = malloc(sizeof(*link));
    if (!link)
        return NULL;

    mlink = malloc(sizeof(*mlink));
    if (!mlink) {
        printk("ERROR: mbox_link_connect: failed to allocate mlink state\n");
        goto free_link;
    }

    mlink->chan_from = hpsc_mbox_chan_claim(mbox, idx_from,
                                            server, client, server,
                                            rcv_cb, NULL, link);
    if (!mlink->chan_from) {
        printk("ERROR: mbox_link_connect: failed to claim chan_from\n");
        goto free_links;
    }
    mlink->chan_to = hpsc_mbox_chan_claim(mbox, idx_to,
                                          server, server, client,
                                          NULL, handle_ack, link);
    if (!mlink->chan_to) {
        printk("ERROR: mbox_link_connect: failed to claim chan_to\n");
        goto free_from;
    }

    mlink->cmd_ctx.tid_requester = RTEMS_ID_NONE;
    mlink->cmd_ctx.tx_acked = false;
    mlink->cmd_ctx.reply = NULL;

    link->priv = mlink;
    link->name = name;
    link->disconnect = mbox_link_disconnect;
    link->send = mbox_link_send;
    link->is_send_acked = mbox_link_is_send_acked;
    link->request = mbox_link_request;
    link->recv = NULL;
    return link;

free_from:
    hpsc_mbox_chan_release(mlink->chan_from);
free_links:
    free(mlink);
free_link:
    free(link);
    return NULL;
}
