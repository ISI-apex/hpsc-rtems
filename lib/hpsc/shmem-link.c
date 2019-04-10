#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <rtems.h>

#include "link.h"
#include "shmem.h"

struct shmem_link {
    struct shmem *shmem_out;
    struct shmem *shmem_in;
};

// TODO: use signals instead of actually polling
#define MIN_SLEEP_MS 10


static int shmem_link_disconnect(struct link *link)
{
    struct shmem_link *slink = link->priv;
    printf("%s: disconnect\n", link->name);
    shmem_close(slink->shmem_out);
    shmem_close(slink->shmem_in);
    free(slink);
    free(link);
    return 0;
}

static void msleep_and_dec(int *ms_rem)
{
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(MIN_SLEEP_MS));
    // if < 0, timeout is infinite
    if (*ms_rem > 0)
        *ms_rem -= *ms_rem >= MIN_SLEEP_MS ? MIN_SLEEP_MS : *ms_rem;
}

static size_t shmem_link_send(struct link *link, int timeout_ms, void *buf,
                              size_t sz)
{
    struct shmem_link *slink = link->priv;
    int sleep_ms_rem = timeout_ms;
    do {
        if (!shmem_get_status(slink->shmem_out)) {
            return shmem_send(slink->shmem_out, buf, sz);
        }
        if (!sleep_ms_rem)
            break; // timeout
        msleep_and_dec(&sleep_ms_rem);
    } while (1);
    return 0;
}

static bool shmem_link_is_send_acked(struct link *link)
{
    struct shmem_link *slink = link->priv;
    // status is currently the "is_new" field, so not ACK'd until cleared
    return shmem_get_status(slink->shmem_out) ? false : true;
}

static size_t shmem_link_recv(struct link *link, void *buf, size_t sz)
{
    struct shmem_link *slink = link->priv;
    if (shmem_get_status(slink->shmem_in))
        return shmem_recv(slink->shmem_in, buf, sz);
    return 0;
}

static size_t shmem_link_poll(struct link *link, int timeout_ms, void *buf,
                              size_t sz)
{
    int sleep_ms_rem = timeout_ms;
    size_t rc;
    do {
        rc = shmem_link_recv(link, buf, sz);
        if (rc > 0)
            break; // got data
        if (!sleep_ms_rem)
            break; // timeout
        msleep_and_dec(&sleep_ms_rem);
    } while (1);
    return rc;
}

static ssize_t shmem_link_request(struct link *link,
                                  int wtimeout_ms, void *wbuf, size_t wsz,
                                  int rtimeout_ms, void *rbuf, size_t rsz)
{
    ssize_t rc;
    printf("%s: request\n", link->name);
    rc = shmem_link_send(link, wtimeout_ms, wbuf, wsz);
    if (!rc) {
        printf("shmem_link_request: send timed out\n");
        return -1;
    }
    rc = shmem_link_poll(link, rtimeout_ms, rbuf, rsz);
    if (!rc)
        printf("shmem_link_request: recv timed out\n");
    return rc;
}

struct link *shmem_link_connect(const char* name, void *addr_out, void *addr_in)
{
    struct shmem_link *slink;
    struct link *link;
    printf("%s: connect\n", name);
    printf("\taddr_out = 0x%"PRIXPTR"\n", (uintptr_t) addr_out);
    printf("\taddr_in  = 0x%"PRIXPTR"\n", (uintptr_t) addr_in);
    link = malloc(sizeof(*link));
    if (!link)
        return NULL;

    slink = malloc(sizeof(*slink));
    if (!slink) {
        goto free_link;
    }
    slink->shmem_out = shmem_open(addr_out);
    if (!slink->shmem_out)
        goto free_links;
    slink->shmem_in = shmem_open(addr_in);
    if (!slink->shmem_in)
        goto free_out;

    link->priv = slink;
    link->name = name;
    link->disconnect = shmem_link_disconnect;
    link->send = shmem_link_send;
    link->is_send_acked = shmem_link_is_send_acked;
    link->request = shmem_link_request;
    link->recv = shmem_link_recv;
    return link;

free_out:
    shmem_close(slink->shmem_out);
free_links:
    free(slink);
free_link:
    free(link);
    return NULL;
}
