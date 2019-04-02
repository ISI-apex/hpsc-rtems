#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "devices.h"

static struct hpsc_mbox *devs_mbox[DEV_ID_MBOX_COUNT] = { 0 };

int dev_add_mbox(dev_id_mbox id, struct hpsc_mbox *dev)
{
    assert(id < DEV_ID_MBOX_COUNT);
    if (devs_mbox[id]) {
        printf("ERROR: dev_add_mbox: already added id=%u\n", id);
        return -1;
    }
    devs_mbox[id] = dev;
    return 0;
}

void dev_remove_mbox(dev_id_mbox id)
{
    assert(id < DEV_ID_MBOX_COUNT);
    devs_mbox[id] = NULL;
}

struct hpsc_mbox *dev_get_mbox(dev_id_mbox id)
{
    assert(id < DEV_ID_MBOX_COUNT);
    return devs_mbox[id];
}

static struct hpsc_wdt *devs_wdt[DEV_ID_CPU_COUNT] = { 0 };

int dev_add_wdt(dev_id_cpu id, struct hpsc_wdt *dev)
{
    assert(id < DEV_ID_CPU_COUNT);
    if (devs_wdt[id]) {
        printf("ERROR: dev_add_wdt: already added id=%u\n", id);
        return -1;
    }
    devs_wdt[id] = dev;
    return 0;
}

void dev_remove_wdt(dev_id_cpu id)
{
    assert(id < DEV_ID_CPU_COUNT);
    devs_wdt[id] = NULL;
}

struct hpsc_wdt *dev_get_wdt(dev_id_cpu id)
{
    assert(id < DEV_ID_CPU_COUNT);
    return devs_wdt[id];
}
