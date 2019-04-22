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

struct mbox_link {
    struct hpsc_mbox *mbox;
    unsigned chan_from;
    unsigned chan_to;
};


static size_t mbox_link_write(struct link *link, void *buf, size_t sz)
{
    struct mbox_link *mlink = link->priv;
    return hpsc_mbox_chan_write(mlink->mbox, mlink->chan_to, buf, sz);
}

static size_t mbox_link_read(struct link *link, void *buf, size_t sz)
{
    struct mbox_link *mlink = link->priv;
    return hpsc_mbox_chan_read(mlink->mbox, mlink->chan_from, buf, sz);
}

static int mbox_link_close(struct link *link) {
    struct mbox_link *mlink = link->priv;
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
                               unsigned server, unsigned client)
{
    struct mbox_link *mlink;
    struct link *link;
    rtems_status_code sc;
    rtems_interrupt_handler rcv_cb = server ? link_recv_cmd : link_recv_reply;

    printk("%s: connect\n", name);
    link = malloc(sizeof(*link));
    if (!link)
        return NULL;
    mlink = malloc(sizeof(*mlink));
    if (!mlink)
        goto free_link;

    link_init(link, name, mlink);
    link->write = mbox_link_write;
    link->read = mbox_link_read;
    link->close = mbox_link_close;

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
                              NULL, link_ack, link);
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
