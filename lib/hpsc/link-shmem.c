#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

#include "link.h"
#include "link-shmem.h"
#include "shmem.h"
#include "shmem-poll.h"

struct link_shmem {
    struct shmem *shmem_out;
    struct shmem *shmem_in;
    struct shmem_poll *sp_recv;
    struct shmem_poll *sp_ack;
};


static void link_shmem_ack(void *arg)
{
    struct link *link = arg;
    struct link_shmem *slink = link->priv;
    shmem_clear_ack(slink->shmem_out);
    link_ack(arg);
}

static size_t link_shmem_write(struct link *link, void *buf, size_t sz)
{
    struct link_shmem *slink = link->priv;
    return shmem_write(slink->shmem_out, buf, sz);
}

static size_t link_shmem_read(struct link *link, void *buf, size_t sz)
{
    struct link_shmem *slink = link->priv;
    return shmem_read(slink->shmem_in, buf, sz);
}

static int link_shmem_close(struct link *link)
{
    struct link_shmem *slink = link->priv;
    int rc = 0;
    rtems_status_code sc;
    printk("%s: close\n", link->name);
    sc = shmem_poll_task_destroy(slink->sp_ack);
    if (sc != RTEMS_SUCCESSFUL)
        rc = -1;
    sc = shmem_poll_task_destroy(slink->sp_recv);
    if (sc != RTEMS_SUCCESSFUL)
        rc = -1;
    shmem_close(slink->shmem_out);
    shmem_close(slink->shmem_in);
    free(slink);
    free(link);
    return rc;
}

static int link_shmem_init(
    struct link_shmem *slink,
    struct link *link,
    uintptr_t addr_out,
    uintptr_t addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_id tid_recv,
    rtems_id tid_ack
)
{
    rtems_status_code sc;
    slink->shmem_out = shmem_open(addr_out);
    if (!slink->shmem_out)
        return -1;
    slink->shmem_in = shmem_open(addr_in);
    if (!slink->shmem_in)
        goto free_out;
    // start listening tasks
    if (is_server) {
        sc = shmem_poll_task_start(&slink->sp_recv, slink->shmem_in,
                                   poll_ticks, HPSC_SHMEM_STATUS_BIT_NEW,
                                   tid_recv, link_recv_cmd, link);
    } else {
        sc = shmem_poll_task_start(&slink->sp_recv, slink->shmem_in,
                                   poll_ticks, HPSC_SHMEM_STATUS_BIT_NEW,
                                   tid_recv, link_recv_reply, link);
    }
    if (sc != RTEMS_SUCCESSFUL) {
        printk("Failed to create receive polling task: %s\n",
               rtems_status_text(sc));
        goto free_all;
    }
    sc = shmem_poll_task_start(&slink->sp_ack, slink->shmem_out, poll_ticks,
                               HPSC_SHMEM_STATUS_BIT_ACK,
                               tid_ack, link_shmem_ack, link);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("Failed to create ACK polling task: %s\n",
               rtems_status_text(sc));
        goto stop_recv_task;
    }
    return 0;

stop_recv_task:
    shmem_poll_task_destroy(slink->sp_recv);
free_all:
    shmem_close(slink->shmem_in);
free_out:
    shmem_close(slink->shmem_out);
    return -1;
}

struct link *link_shmem_connect(
    const char* name,
    uintptr_t addr_out,
    uintptr_t addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_id tid_recv,
    rtems_id tid_ack
)
{
    struct link_shmem *slink;
    struct link *link;
    assert(name);
    assert(addr_out);
    assert(addr_in);

    printk("%s: connect\n", name);
    printk("\taddr_out = 0x%"PRIXPTR"\n", (uintptr_t) addr_out);
    printk("\taddr_in  = 0x%"PRIXPTR"\n", (uintptr_t) addr_in);
    printk("\tpoll_ticks  = %u\n", poll_ticks);

    link = malloc(sizeof(*link));
    if (!link)
        return NULL;
    slink = malloc(sizeof(*slink));
    if (!slink)
        goto free_link;

    link_init(link, name, slink);
    link->write = link_shmem_write;
    link->read = link_shmem_read;
    link->close = link_shmem_close;

    if (link_shmem_init(slink, link, addr_out, addr_in, is_server, poll_ticks,
                        tid_recv, tid_ack))
        goto free_links;

    return link;

free_links:
    free(slink);
free_link:
    free(link);
    return NULL;
}
