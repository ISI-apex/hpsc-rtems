#ifndef DEVICES_H
#define DEVICES_H

#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

// TODO: CPU-related handling should be eventually be merged with the subsys
// configuration currently in TRCH baremetal source.
typedef enum {
    DEV_ID_CPU_RTPS_R52_0 = 0,
    DEV_ID_CPU_RTPS_R52_1,
    DEV_ID_CPU_COUNT
} dev_id_cpu;

#define dev_id_cpu_for_each(cpu) for (cpu = 0; cpu < DEV_ID_CPU_COUNT; cpu++)

// It would be nice to keep mailbox device tracking more dynamic, but without
// advanced data structures readily available, we'll settle for a static count
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
int dev_add_mbox(dev_id_mbox id, struct hpsc_mbox *dev);
void dev_remove_mbox(dev_id_mbox id);
struct hpsc_mbox *dev_get_mbox(dev_id_mbox id);

#define dev_id_cpu_for_each_rtit(cpu, rtit) \
    for (cpu = 0, rtit = dev_get_rtit(cpu); \
         cpu < DEV_ID_CPU_COUNT; \
         cpu++, rtit = (cpu < DEV_ID_CPU_COUNT) ? dev_get_rtit(cpu) : NULL)
int dev_add_rtit(dev_id_cpu id, struct hpsc_rti_timer *dev);
void dev_remove_rtit(dev_id_cpu id);
struct hpsc_rti_timer *dev_get_rtit(dev_id_cpu id);

#define dev_id_cpu_for_each_wdt(cpu, wdt) \
    for (cpu = 0, wdt = dev_get_wdt(cpu); \
         cpu < DEV_ID_CPU_COUNT; \
         cpu++, wdt = (cpu < DEV_ID_CPU_COUNT) ? dev_get_wdt(cpu) : NULL)
int dev_add_wdt(dev_id_cpu id, struct hpsc_wdt *dev);
void dev_remove_wdt(dev_id_cpu id);
struct hpsc_wdt *dev_get_wdt(dev_id_cpu id);

#endif // DEVICES_H
