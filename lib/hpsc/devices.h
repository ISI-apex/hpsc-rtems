#ifndef DEVICES_H
#define DEVICES_H

#include <rtems.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

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

#define dev_cpu_for_each(cpu) \
    for (cpu = 0; \
         cpu < rtems_get_processor_count(); \
         cpu++)

// Access to current CPU's RTI Timers
void dev_cpu_set_rtit(struct hpsc_rti_timer *dev);
struct hpsc_rti_timer *dev_cpu_get_rtit(void);

// Access to current CPU's Watchdog Timers
void dev_cpu_set_wdt(struct hpsc_wdt *dev);
struct hpsc_wdt *dev_cpu_get_wdt(void);

#endif // DEVICES_H
