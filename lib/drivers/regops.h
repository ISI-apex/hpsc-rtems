#ifndef REGOPS_H
#define REGOPS_H

#include <stdint.h>

#include <rtems.h>

#include "debug.h"

#define REG_WRITE32(reg, val) reg_write32(#reg, (volatile uint32_t *)(reg), val)
#define REG_WRITE64(reg, val) reg_write64(#reg, (volatile uint64_t *)(reg), val)

#define REGB_WRITE32(base, reg, val) \
        reg_write32(#reg, (volatile uint32_t *)((uintptr_t)(base) + (reg)), val)
#define REGB_WRITE64(base, reg, val) \
        reg_write64(#reg, (volatile uint64_t *)((uintptr_t)(base) + (reg)), val)

#define REG_SET32(reg, val)   reg_set32(#reg,   (volatile uint32_t *)(reg), val)
#define REG_CLEAR32(reg, val) reg_clear32(#reg, (volatile uint32_t *)(reg), val)

#define REGB_SET32(base, reg, val) \
        reg_set32(#reg, (volatile uint32_t *)((uintptr_t)(base) + (reg)), val)
#define REGB_CLEAR32(base, reg, val) \
        reg_clear32(#reg, (volatile uint32_t *)((uintptr_t)(base) + (reg)), val)

#define REG_READ32(reg) reg_read32(#reg, (volatile uint32_t *)(reg))
#define REG_READ64(reg) reg_read64(#reg, (volatile uint32_t *)(reg))

#define REGB_READ32(base, reg) \
        reg_read32(#reg, (volatile uint32_t *)((uintptr_t)(base) + (reg)))
#define REGB_READ64(base, reg) \
        reg_read64(#reg, (volatile uint64_t *)((uintptr_t)(base) + (reg)))

RTEMS_INLINE_ROUTINE void reg_write32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTK("%32s: %p <- %x\n", name, addr, val);
    *addr = val;
}
RTEMS_INLINE_ROUTINE void reg_write64(const char *name, volatile uint64_t *addr, uint64_t val)
{
    DPRINTK("%32s: %p <- %08x%08x\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    *addr = val;
}
RTEMS_INLINE_ROUTINE uint32_t reg_read32(const char *name, volatile uint32_t *addr)
{
    uint32_t val = *addr;
    DPRINTK("%32s: %p -> %x\n", name, addr, val);
    return val;
}
RTEMS_INLINE_ROUTINE uint64_t reg_read64(const char *name, volatile uint64_t *addr)
{
    uint64_t val = *addr;
    DPRINTK("%32s: %p -> %08x%08x\n", name, addr,
           (uint32_t)(val >> 32), (uint32_t)val);
    return val;
}
RTEMS_INLINE_ROUTINE void reg_set32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTK("%32s: %p |= %x\n", name, addr, val);
    *addr |= val;
}
RTEMS_INLINE_ROUTINE void reg_clear32(const char *name, volatile uint32_t *addr, uint32_t val)
{
    DPRINTK("%32s: %p &= ~%x\n", name, addr, val);
    *addr &= ~val;
}

#endif // REGOPS_H
