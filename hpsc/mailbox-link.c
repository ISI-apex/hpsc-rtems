#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>

#include <hpsc-mbox.h>

#include "command.h"
#include "link.h"
#include "mailbox-link.h"


struct cmd_ctx {
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
    printk("%s: handle_ack\n", link->name);
    mlink->cmd_ctx.tx_acked = true;
}

static void handle_cmd(void *arg)
{
    struct link *link = arg;
    struct mbox_link *mlink = link->priv;
    struct cmd cmd; // can't use initializer, because GCC inserts a memset for initing .msg
    cmd.link = link;
    assert(sizeof(cmd.msg) == HPSC_MBOX_DATA_SIZE); // o/w zero-fill rest of msg

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
    printk("%s: handle_reply\n", link->name);
    mlink->cmd_ctx.reply_sz_read = hpsc_mbox_chan_read(mlink->chan_from,
                                                       mlink->cmd_ctx.reply,
                                                       mlink->cmd_ctx.reply_sz);
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

static int mbox_link_send(struct link *link, int timeout_ms, void *buf,
                          size_t sz)
{
    // TODO: timeout
    struct mbox_link *mlink = link->priv;
    mlink->cmd_ctx.tx_acked = false;
    return hpsc_mbox_chan_write(mlink->chan_to, buf, sz);
}

static bool mbox_link_is_send_acked(struct link *link)
{
    struct mbox_link *mlink = link->priv;
    return mlink->cmd_ctx.tx_acked;
}

static int mbox_link_poll(struct link *link, int timeout_ms)
{
    // TODO: timeout
    struct mbox_link *mlink = link->priv;
    printk("%s: poll: waiting for reply...\n", link->name);
    while (!mlink->cmd_ctx.reply_sz_read);
    printk("%s: poll: reply received\n", link->name);
    return mlink->cmd_ctx.reply_sz_read;
}

static int mbox_link_request(struct link *link,
                             int wtimeout_ms, void *wbuf, size_t wsz,
                             int rtimeout_ms, void *rbuf, size_t rsz)
{
    struct mbox_link *mlink = link->priv;
    int rc;

    printk("%s: request\n", link->name);
    mlink->cmd_ctx.reply_sz_read = 0;
    mlink->cmd_ctx.reply = rbuf;
    mlink->cmd_ctx.reply_sz = rsz / sizeof(uint32_t);

    rc = mbox_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printk("mbox_link_request: send failed\n");
        return -1;
    }

    // TODO: timeout on ACKs as part of rtimeout_ms
    printk("%s: request: waiting for ACK...\n", link->name);
    while (!mlink->cmd_ctx.tx_acked);
    printk("%s: request: ACK received\n", link->name);

    rc = mbox_link_poll(link, rtimeout_ms);
    if (!rc)
        printk("mbox_link_request: recv failed\n");
    return rc;
}

struct link *mbox_link_connect(const char *name, struct hpsc_mbox *mbox,
                               unsigned idx_from, unsigned idx_to,
                               unsigned server, unsigned client)
{
    struct mbox_link *mlink;
    struct link *link;
    hpsc_mbox_chan_irq_cb rcv_cb = server ? handle_cmd : handle_reply;
    int ret;

    printk("%s: connect\n", name);
    link = malloc(sizeof(*link));
    if (!link)
        return NULL;

    mlink = malloc(sizeof(*mlink));
    if (!mlink) {
        printk("ERROR: mbox_link_connect: failed to allocate mlink state\n");
        goto free_link;
    }

    // NOTE: we don't verify that channels aren't already claimed
    ret = hpsc_mbox_chan_claim(&mbox->chans[idx_from],
                               server, client, server,
                               rcv_cb, NULL, link);
    if (ret) {
        printk("ERROR: mbox_link_connect: failed to claim chan_from\n");
        goto free_links;
    }
    mlink->chan_from = &mbox->chans[idx_from];
    ret = hpsc_mbox_chan_claim(&mbox->chans[idx_to],
                               server, server, client,
                               NULL, handle_ack, link);
    if (ret) {
        printk("ERROR: mbox_link_connect: failed to claim chan_to\n");
        goto free_from;
    }
    mlink->chan_to = &mbox->chans[idx_to];

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
