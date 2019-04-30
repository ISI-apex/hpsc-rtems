#ifndef DEVICES_H
#define DEVICES_H

#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

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

#define dev_id_cpu_for_each(cpu) \
    for (cpu = 0; \
         cpu < rtems_get_processor_count(); \
         cpu++)

// Access to current CPU's RTI Timers
void cpu_set_rtit(struct hpsc_rti_timer *dev);
struct hpsc_rti_timer *cpu_get_rtit(void);

// Access to current CPU's Watchdog Timers
void cpu_set_wdt(struct hpsc_wdt *dev);
struct hpsc_wdt *cpu_get_wdt(void);

#endif // DEVICES_H
