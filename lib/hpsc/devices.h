#ifndef DEVICES_H
#define DEVICES_H

#include <hpsc-mbox.h>

typedef enum {
    DEV_ID_MBOX_LSIO = 0,
    DEV_ID_MBOX_HPPS_TRCH,
    DEV_ID_MBOX_HPPS_RTPS,
    DEV_ID_MBOX_COUNT
} dev_id_mbox;

#define dev_id_mbox_for_each(id, mbox) \
    for (id = 0, mbox = dev_get_mbox(id); \
         id < DEV_ID_MBOX_COUNT; \
         id++, mbox = (id < DEV_ID_MBOX_COUNT) ? dev_get_mbox(id) : NULL)
void dev_set_mbox(dev_id_mbox id, struct hpsc_mbox *dev);
struct hpsc_mbox *dev_get_mbox(dev_id_mbox id);

#endif // DEVICES_H
