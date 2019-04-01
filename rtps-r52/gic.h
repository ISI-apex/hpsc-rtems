#ifndef GIC_H
#define GIC_H

#include <rtems.h>

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
static inline rtems_vector_number gic_irq_to_rvn(
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

#endif // GIC_H