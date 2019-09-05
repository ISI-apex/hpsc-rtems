#include <stdint.h>

#include <rtems.h>
#include <bsp/arm-gic-irq.h>

#include "gic.h"

// #define ARM_GIC_REDIST ((volatile gic_redist *) BSP_ARM_GIC_REDIST_BASE)
#define ARM_GIC_SGI_PPI (((volatile gic_sgi_ppi *) ((uintptr_t)BSP_ARM_GIC_REDIST_BASE + (1 << 16))))

#define GIC_TRIGGER_BIT(vec) (2 * (vec % 16) + 1)

static volatile uint32_t *to_reg_ptr(rtems_vector_number vector)
{
    volatile gic_dist *dist = ARM_GIC_DIST;
    volatile gic_sgi_ppi *sgi_ppi = ARM_GIC_SGI_PPI;
    volatile uint32_t *reg;
    if (vector < 16)
        reg = &(sgi_ppi->icspicfgr0);
    else if (vector < 32)
        reg = &(sgi_ppi->icspicfgr1);
    else
        reg = &(dist->icdicfr[vector / 16]);
    return reg;
}

gic_trigger_mode gic_trigger_get(rtems_vector_number vector)
{
    uint32_t bit = GIC_TRIGGER_BIT(vector);
    volatile uint32_t *reg = to_reg_ptr(vector);
    uint32_t value = *reg;
    return (gic_trigger_mode)((value >> bit) & 0x1);
}

void gic_trigger_set(rtems_vector_number vector, gic_trigger_mode condition)
{
    uint32_t bit = GIC_TRIGGER_BIT(vector);
    uint32_t mask = 1 << bit;
    uint32_t bit_value = condition << bit;
    volatile uint32_t *reg = to_reg_ptr(vector);
    uint32_t value = *reg;
    value &= ~mask;
    value |= bit_value;
    *reg = value;
}
