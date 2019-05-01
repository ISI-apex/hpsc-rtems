#include <assert.h>

#include "devices.h"

static struct hpsc_mbox *devs_mbox[DEV_ID_MBOX_COUNT] = { 0 };
void dev_set_mbox(dev_id_mbox id, struct hpsc_mbox *dev)
{
    assert(id < DEV_ID_MBOX_COUNT);
    if (dev)
        assert(!devs_mbox[id]); // not already set
    devs_mbox[id] = dev;
}
struct hpsc_mbox *dev_get_mbox(dev_id_mbox id)
{
    assert(id < DEV_ID_MBOX_COUNT);
    return devs_mbox[id];
}
