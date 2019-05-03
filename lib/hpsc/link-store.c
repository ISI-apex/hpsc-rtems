#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>
#include <rtems/chain.h>
#include <rtems/thread.h>

#include "link.h"
#include "link-store.h"

struct link_store_node {
    rtems_chain_node node; // must be at the top of the struct to cast
    struct link *link;
};

static RTEMS_CHAIN_DEFINE_EMPTY(lchain);
static rtems_mutex lmtx = RTEMS_MUTEX_INITIALIZER("Link Store");

static struct link_store_node *link_store_node_get(const char *name)
{
    struct link_store_node *snode;
    rtems_chain_node* node;
    for (node = rtems_chain_first(&lchain);
         !rtems_chain_is_tail(&lchain, node);
         node = rtems_chain_next(node)) {
        snode = (struct link_store_node *)node;
        if (!strcmp(name, snode->link->name))
            return snode;
    }
    return NULL;
}

rtems_status_code link_store_append(struct link *link)
{
    struct link_store_node *snode;
    snode = malloc(sizeof(struct link_store_node));
    if (!snode)
        return RTEMS_NO_MEMORY;
    snode->link = link;
    rtems_mutex_lock(&lmtx);
    rtems_chain_append(&lchain, &snode->node);
    rtems_mutex_unlock(&lmtx);
    return RTEMS_SUCCESSFUL;
}

bool link_store_contains(const char *name)
{
    bool rc;
    rtems_mutex_lock(&lmtx);
    rc = link_store_node_get(name) ? true : false;
    rtems_mutex_unlock(&lmtx);
    return rc;
}

bool link_store_is_empty(void)
{
    bool rc;
    rtems_mutex_lock(&lmtx);
    rc = rtems_chain_is_empty(&lchain);
    rtems_mutex_unlock(&lmtx);
    return rc;
}

struct link *link_store_get(const char *name)
{
    struct link_store_node *snode;
    rtems_mutex_lock(&lmtx);
    snode = link_store_node_get(name);
    rtems_mutex_unlock(&lmtx);
    return snode ? snode->link : NULL;
}

struct link *link_store_extract(const char *name)
{
    struct link *link = NULL;
    struct link_store_node *snode;
    rtems_mutex_lock(&lmtx);
    snode = link_store_node_get(name);
    if (snode) {
        link = snode->link;
        rtems_chain_extract(&snode->node);
        free(snode);
    }
    rtems_mutex_unlock(&lmtx);
    return link;
}

struct link *link_store_extract_first(void)
{
    struct link *link = NULL;
    struct link_store_node *snode;
    rtems_chain_node* node;
    rtems_mutex_lock(&lmtx);
    node = rtems_chain_get(&lchain);
    rtems_mutex_unlock(&lmtx);
    if (node) {
        snode = ((struct link_store_node *)node);
        link = snode->link;
        free(snode);
    }
    return link;
}
