#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>
#include <rtems/chain.h>

#include "link.h"

struct link_store_node {
    rtems_chain_node node; // must be at the top of the struct to cast
    struct link *link;
};

static RTEMS_CHAIN_DEFINE_EMPTY(lchain);

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
    rtems_chain_append(&lchain, &snode->node);
    return RTEMS_SUCCESSFUL;
}

bool link_store_contains(const char *name)
{
    return link_store_node_get(name) ? true : false;
}

bool link_store_is_empty(void)
{
    return rtems_chain_is_empty(&lchain);
}

struct link *link_store_get(const char *name)
{
    struct link_store_node *snode = link_store_node_get(name);
    return snode ? snode->link : NULL;
}

struct link *link_store_extract(const char *name)
{
    struct link *link = NULL;
    struct link_store_node *snode = link_store_node_get(name);
    if (snode) {
        link = snode->link;
        rtems_chain_extract(&snode->node);
        free(snode);
    }
    return link;
}

struct link *link_store_extract_first(void)
{
    rtems_chain_node* node = rtems_chain_first(&lchain);
    if (rtems_chain_is_null_node(node))
        return NULL;
    return link_store_extract(((struct link_store_node *)node)->link->name);
}
