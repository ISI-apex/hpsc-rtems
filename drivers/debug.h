#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

// Define DEBUG to 1 in the source file that you want to debug
// before the #include statement for this header.
#ifndef DEBUG
#define DEBUG 0
#endif // undef DEBUG

#define DPRINTF(...) \
        if (DEBUG) printf(__VA_ARGS__)

#define DPRINTK(...) \
        if (DEBUG) printk(__VA_ARGS__)

#endif // DEBUG_H
