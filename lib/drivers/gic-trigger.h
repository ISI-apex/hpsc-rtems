/**
 * RTEMS does not natively support modification of interrupt trigger mode from
 * the default (level triggered).
 * Get/set gic_trigger_mode: GIC_LEVEL_SENSITIVE or GIC_EDGE_TRIGGERED
 */
#ifndef GIC_TRIGGER_H
#define GIC_TRIGGER_H

#include <rtems.h>
#include <bsp/arm-gic.h>

gic_trigger_mode gic_trigger_get(rtems_vector_number vector);

void gic_trigger_set(rtems_vector_number vector, gic_trigger_mode condition);

#endif // GIC_TRIGGER_H
