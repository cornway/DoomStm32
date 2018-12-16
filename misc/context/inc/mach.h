#ifndef MACH_H
#define MACH_H

#include "arch.h"
#include "arch_hal.h"

#define V_ASSERT(cond) \
do { \
    if ((cond) == 0) { \
        for (;;) {} \
    } \
} while (0)

#endif /*MACH_H*/
