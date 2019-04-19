#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/irq-extension.h>

#include "link.h"
#include "shmem.h"
#include "shmem-link.h"
#include "shmem-poll.h"

struct shmem_link {
    struct shmem *shmem_out;
    struct shmem *shmem_in;
    struct shmem_poll *sp_recv;
    struct shmem_poll *sp_ack;
};


static void shmem_link_ack(void *arg)
{
    struct link *link = arg;
    struct shmem_link *slink = link->priv;
    shmem_clear_ack(slink->shmem_out);
    link_ack(arg);
}

static size_t shmem_link_write(struct link *link, void *buf, size_t sz)
{
    struct shmem_link *slink = link->priv;
    return shmem_send(slink->shmem_out, buf, sz);
}

static size_t shmem_link_read(struct link *link, void *buf, size_t sz)
{
    struct shmem_link *slink = link->priv;
    return shmem_recv(slink->shmem_in, buf, sz);
}

static int shmem_link_close(struct link *link)
{
    struct shmem_link *slink = link->priv;
    int rc = 0;
    rtems_status_code sc;
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

static int shmem_link_init(
    struct shmem_link *slink,
    struct link *link,
    volatile void *addr_out,
    volatile void *addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_name tname_recv,
    rtems_name tname_ack
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
        sc = shmem_poll_task_create(&slink->sp_recv, slink->shmem_in,
                                    poll_ticks, HPSC_SHMEM_STATUS_BIT_NEW,
                                    tname_recv, link_recv_cmd, link);
    } else {
        sc = shmem_poll_task_create(&slink->sp_recv, slink->shmem_in,
                                    poll_ticks, HPSC_SHMEM_STATUS_BIT_NEW,
                                    tname_recv, link_recv_reply, link);
    }
    if (sc != RTEMS_SUCCESSFUL) {
        printk("Failed to create receive polling task\n");
        goto free_all;
    }
    sc = shmem_poll_task_create(&slink->sp_ack, slink->shmem_out, poll_ticks,
                                HPSC_SHMEM_STATUS_BIT_ACK,
                                tname_ack, shmem_link_ack, link);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("Failed to create ACK polling task\n");
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

struct link *shmem_link_connect(
    const char* name,
    volatile void *addr_out,
    volatile void *addr_in,
    bool is_server,
    rtems_interval poll_ticks,
    rtems_name tname_recv,
    rtems_name tname_ack
)
{
    struct shmem_link *slink;
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
    link->write = shmem_link_write;
    link->read = shmem_link_read;
    link->close = shmem_link_close;

    if (shmem_link_init(slink, link, addr_out, addr_in, is_server, poll_ticks,
                        tname_recv, tname_ack))
        goto free_links;

    return link;

free_links:
    free(slink);
free_link:
    free(link);
    return NULL;
}
