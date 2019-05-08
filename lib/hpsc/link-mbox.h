#ifndef LINK_MBOX_H
#define LINK_MBOX_H

#include <stdint.h>

// drivers
#include <hpsc-mbox.h>

#include "link.h"

// To claim as server: set both server and client to non-zero ID
//                     the server value is also used as the owner
// To claim as client: set server to 0 and set client to non-zero ID
struct link *link_mbox_connect(const char *name, struct hpsc_mbox *mbox,
                               unsigned idx_from, unsigned idx_to,
                               uint32_t server, uint32_t client);

#endif // LINK_MBOX_H
