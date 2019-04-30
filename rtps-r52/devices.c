#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/score/percpudata.h>

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

static Per_CPU_Control *cpu_get_control(void)
{
    return _Per_CPU_Get_by_index(rtems_get_current_processor());
}

static PER_CPU_DATA_ITEM(struct hpsc_rti_timer *, rtits) = { 0 };
void cpu_set_rtit(struct hpsc_rti_timer *dev)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_rti_timer *cpu_get_rtit(void)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    return *tmp;
}

static PER_CPU_DATA_ITEM(struct hpsc_wdt *, wdts) = { 0 };
void cpu_set_wdt(struct hpsc_wdt *dev)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_wdt *cpu_get_wdt(void)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    return *tmp;
}
