//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Fixed point arithemtics, implementation.
//


#ifndef __M_FIXED__
#define __M_FIXED__

#include <stdint.h>
#include <limits.h>


//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS		16
#define FRACUNIT		(1<<FRACBITS)
#define DOUBLEUNIT     ((double)(FRACUNIT))
#define DOUBLEUNIT_SQR (DOUBLEUNIT * DOUBLEUNIT)
#define FIXED_DENOM(x)  (fixed_t)((1.0 / (double)(x)) * (double)FRACUNIT)

typedef int fixed_t;


#if 0
fixed_t FixedMul	(fixed_t a, fixed_t b);
fixed_t FixedDiv	(fixed_t a, fixed_t b);
#else

static inline fixed_t
FixedMul (fixed_t a, fixed_t b)
{
    return ((int64_t) a * (int64_t) b) >> FRACBITS;
}

extern fixed_t
IMFixDiv (fixed_t a, fixed_t b);

static inline fixed_t
FixedDiv (fixed_t a, fixed_t b)
{
#if 1
    if ((int)b == 0) {
        return (a^b) < 0 ? INT_MIN : INT_MAX;
    } else {
        return (fixed_t)(((double)(a) * DOUBLEUNIT) / (double)b);
    }
#else
    return IMFixDiv (a, b);
#endif
}
  


#endif


#endif
