#include <assert.h>

#include <rtems.h>
#include <rtems/score/percpudata.h>

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

static Per_CPU_Control *cpu_get_control(void)
{
    return _Per_CPU_Get_by_index(rtems_get_current_processor());
}

static PER_CPU_DATA_ITEM(struct hpsc_rti_timer *, rtits) = { 0 };
void dev_cpu_set_rtit(struct hpsc_rti_timer *dev)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_rti_timer *dev_cpu_get_rtit(void)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    return *tmp;
}

static PER_CPU_DATA_ITEM(struct hpsc_wdt *, wdts) = { 0 };
void dev_cpu_set_wdt(struct hpsc_wdt *dev)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_wdt *dev_cpu_get_wdt(void)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    return *tmp;
}
