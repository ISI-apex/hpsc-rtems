#ifndef BIT_H
#define BIT_H

#define ALIGN_MASK(bits) ((1 << (bits)) - 1)
#define ALIGN(x, bits) (typeof(x))(((uint32_t)(x) + ALIGN_MASK(bits)) & ~ALIGN_MASK(bits))
#define ALIGNED(x, bits) (x == ALIGN(x, bits))

unsigned log2_of_pow2(unsigned long v);

#endif // BIT_H
