#ifndef LINK_MBOX_H
#define LINK_MBOX_H

#include <hpsc-mbox.h>

#include "link.h"

// We use 'owner' to indicate both the ID (arbitrary value) to which the
// mailbox should be claimed (i.e. OWNER HW register should be set) and whether
// the connection originator is a server or a client: owner!=0 ==> server;
// owner=0 ==> client. However, these concepts are orthogonal, so it would be
// easy to decouple them if desired by adding another arg to this function.
//
// To claim as server: set both server and client to non-zero ID
// To claim as client: set server to 0 and set client to non-zero ID
struct link *link_mbox_connect(const char *name, struct hpsc_mbox *mbox,
                               unsigned idx_from, unsigned idx_to,
                               unsigned server, unsigned client);

#endif // LINK_MBOX_H
