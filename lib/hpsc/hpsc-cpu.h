#ifndef HPSC_CPU_H
#define HPSC_CPU_H

#include <rtems.h>

// drivers
#include <hpsc-rti-timer.h>
#include <hpsc-wdt.h>

#define hpsc_cpu_for_each(cpu) \
    for (cpu = 0; \
         cpu < rtems_get_processor_count(); \
         cpu++)

// Access to current CPU's RTI Timers
void hpsc_cpu_set_rtit(struct hpsc_rti_timer *dev);
struct hpsc_rti_timer *hpsc_cpu_get_rtit(void);

// Access to current CPU's Watchdog Timers
void hpsc_cpu_set_wdt(struct hpsc_wdt *dev);
struct hpsc_wdt *hpsc_cpu_get_wdt(void);

#endif // HPSC_CPU_H
