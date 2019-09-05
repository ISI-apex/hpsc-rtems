#ifndef GIC_H
#define GIC_H

#include <rtems.h>
#include <bsp/arm-gic.h>

#define GIC_INTERNAL   32
#define GIC_NR_SGIS    16

typedef enum {
    GIC_IRQ_TYPE_SPI,
    GIC_IRQ_TYPE_LPI,
    GIC_IRQ_TYPE_PPI,
    GIC_IRQ_TYPE_SGI,
} gic_irq_type_t;

/**
 * Translate internal IRQ values to those that RTEMS requires.
 * Actual interrupt enable/disable managed by RTEMS interrupt manager.
 */
RTEMS_INLINE_ROUTINE rtems_vector_number gic_irq_to_rvn(
    unsigned irq,
    gic_irq_type_t type
)
{
    switch (type) {
        case GIC_IRQ_TYPE_SPI:
            return GIC_INTERNAL + irq;
        case GIC_IRQ_TYPE_PPI:
            return GIC_NR_SGIS + irq;
        case GIC_IRQ_TYPE_SGI:
            return irq;
        default:
            rtems_panic("unknown irq type");
    }
}

gic_trigger_mode gic_trigger_get(rtems_vector_number vector);

void gic_trigger_set(rtems_vector_number vector, gic_trigger_mode condition);

#endif // GIC_H
