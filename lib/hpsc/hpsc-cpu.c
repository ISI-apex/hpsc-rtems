#include <assert.h>

#include <rtems.h>
#include <rtems/score/percpudata.h>

#include "hpsc-cpu.h"

static Per_CPU_Control *cpu_get_control(void)
{
    return _Per_CPU_Get_by_index(rtems_get_current_processor());
}

static PER_CPU_DATA_ITEM(struct hpsc_rti_timer *, rtits) = { 0 };
void hpsc_cpu_set_rtit(struct hpsc_rti_timer *dev)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_rti_timer *hpsc_cpu_get_rtit(void)
{
    struct hpsc_rti_timer **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_rti_timer *, rtits);
    assert(tmp);
    return *tmp;
}

static PER_CPU_DATA_ITEM(struct hpsc_wdt *, wdts) = { 0 };
void hpsc_cpu_set_wdt(struct hpsc_wdt *dev)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    if (dev)
        assert(!*tmp); // not already set
    *tmp = dev;
}
struct hpsc_wdt *hpsc_cpu_get_wdt(void)
{
    struct hpsc_wdt **tmp;
    tmp = PER_CPU_DATA_GET(cpu_get_control(), struct hpsc_wdt *, wdts);
    assert(tmp);
    return *tmp;
}
