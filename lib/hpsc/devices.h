#ifndef DEVICES_H
#define DEVICES_H

#include <rtems.h>
#include <bsp/hpsc-wdt.h>

// drivers
#include <hpsc-mbox.h>
#include <hpsc-rti-timer.h>

// A store for device driver handles.
// Users are responsible for synchronizing access to devices and safely managing
// pointers in the store.

/**
 * Enumeration of possible HPSC Chiplet mailbox device instances.
 */
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
/**
 * Set (or unset) the pointer to a mailbox device handle.
 */
void dev_set_mbox(dev_id_mbox id, struct hpsc_mbox *dev);
/**
 * Get the pointer to a mailbox device handle, or NULL.
 */
struct hpsc_mbox *dev_get_mbox(dev_id_mbox id);

#define dev_cpu_for_each(cpu) \
    for (cpu = 0; \
         cpu < rtems_get_processor_count(); \
         cpu++)

/**
 * Set (or unset) the pointer to the current CPU's RTI Timer device handle.
 */
void dev_cpu_set_rtit(struct hpsc_rti_timer *dev);
/**
 * Get the pointer to the current CPU's RTI Timer device handle, or NULL.
 */
struct hpsc_rti_timer *dev_cpu_get_rtit(void);

/**
 * Set (or unset) the pointer to the current CPU's Watchdog device handle.
 */
void dev_cpu_set_wdt(struct HPSC_WDT_Config *dev);
/**
 * Get the pointer to the current CPU's Watchdog device handle, or NULL.
 */
struct HPSC_WDT_Config *dev_cpu_get_wdt(void);

#endif // DEVICES_H
