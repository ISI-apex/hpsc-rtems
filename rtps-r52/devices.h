#ifndef DEVICES_H
#define DEVICES_H

#include "hpsc-mbox.h"

// It would be nice to keep mailbox device tracking more dynamic, but without
// advanced data structures readily available, we'll settle for a static count
typedef enum {
    DEV_ID_MBOX_HPPS = 0,
    DEV_ID_MBOX_LSIO,
    DEV_ID_MBOX_COUNT
} dev_id_mbox;

int dev_add_mbox(dev_id_mbox id, struct hpsc_mbox *dev);

void dev_remove_mbox(dev_id_mbox id);

struct hpsc_mbox *dev_get_mbox(dev_id_mbox id);

#endif // DEVICES_H
