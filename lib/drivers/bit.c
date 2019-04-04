#include <assert.h>

#include "bit.h"

unsigned log2_of_pow2(unsigned long v)
{
    assert(v);
    int b = 1;
    while ((v & 0x1) == 0x0) {
        v >>= 1;
        b++;
    }
    assert(v == 0x1); // power of 2
    return b;
}
