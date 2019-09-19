#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

// drivers
#include <hpsc-mbox.h>

#include "link.h"
#include "link-mbox.h"

struct link_mbox {
    struct hpsc_mbox *mbox;
    unsigned chan_from;
    unsigned chan_to;
};


static void link_mbox_ack(void *arg)
{
    struct link *link = arg;
    struct link_mbox *mlink = link->priv;
    hpsc_mbox_chan_event_clear_ack(mlink->mbox, mlink->chan_to);
    link_ack(arg);
}

static size_t link_mbox_write(struct link *link, void *buf, size_t sz)
{
    struct link_mbox *mlink = link->priv;
    size_t rc = hpsc_mbox_chan_write(mlink->mbox, mlink->chan_to, buf, sz);
    hpsc_mbox_chan_event_set_rcv(mlink->mbox, mlink->chan_to);
    return rc;
}

static size_t link_mbox_read(struct link *link, void *buf, size_t sz)
{
    struct link_mbox *mlink = link->priv;
    size_t rc = hpsc_mbox_chan_read(mlink->mbox, mlink->chan_from, buf, sz);
    hpsc_mbox_chan_event_clear_rcv(mlink->mbox, mlink->chan_from);
    hpsc_mbox_chan_event_set_ack(mlink->mbox, mlink->chan_from);
    return rc;
}

static int link_mbox_close(struct link *link) {
    struct link_mbox *mlink = link->priv;
    rtems_status_code sc;
    int rc = 0;
    printk("%s: close\n", link->name);
    // in case of failure, keep going and fwd code
    sc = hpsc_mbox_chan_release(mlink->mbox, mlink->chan_from);
    if (sc != RTEMS_SUCCESSFUL)
        rc = 1;
    sc = hpsc_mbox_chan_release(mlink->mbox, mlink->chan_to);
    if (sc != RTEMS_SUCCESSFUL)
        rc = 1;
    free(mlink);
    free(link);
    return rc;
}

struct link *link_mbox_connect(const char *name, struct hpsc_mbox *mbox,
                               unsigned idx_from, unsigned idx_to,
                               uint8_t server, uint8_t client)
{
    struct link_mbox *mlink;
    struct link *link;
    rtems_status_code sc;
    rtems_interrupt_handler rcv_cb = server ? link_recv_cmd : link_recv_reply;
    assert(name);

    printk("%s: connect\n", name);
    printk("\tidx_from = %u\n", idx_from);
    printk("\tidx_to   = %u\n", idx_to);
    printk("\tserver   = 0x%x\n", server);
    printk("\tclient   = 0x%x\n", client);
    link = malloc(sizeof(*link));
    if (!link)
        return NULL;
    mlink = malloc(sizeof(*mlink));
    if (!mlink)
        goto free_link;

    link_init(link, name, mlink);
    link->write = link_mbox_write;
    link->read = link_mbox_read;
    link->close = link_mbox_close;

    mlink->mbox = mbox;
    mlink->chan_from = idx_from;
    mlink->chan_to = idx_to;

    sc = hpsc_mbox_chan_claim(mbox, idx_from, server, client, server,
                              rcv_cb, NULL, link);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("ERROR: link_mbox_connect: failed to claim chan_from\n");
        goto free_links;
    }
    sc = hpsc_mbox_chan_claim(mbox, idx_to, server, server, client,
                              NULL, link_mbox_ack, link);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("ERROR: link_mbox_connect: failed to claim chan_to\n");
        goto free_from;
    }

    return link;

free_from:
    hpsc_mbox_chan_release(mbox, idx_from);
free_links:
    free(mlink);
free_link:
    free(link);
    return NULL;
}
