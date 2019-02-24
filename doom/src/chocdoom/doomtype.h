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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//    


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

// #define macros to provide functions missing in Windows.
// Outside Windows, we use strings.h for str[n]casecmp.


#ifdef _WIN32

#define strcasecmp stricmp
#define strncasecmp strnicmp

#else

#include "string.h"
#include "arch.h"

#endif


//
// The packed attribute forces structures to be packed into the minimum 
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimize memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

#if     defined(V_PREPACK) && defined(V_POSTPACK)
#define PACKEDATTR V_POSTPACK
#elif defined(__GNUC__)
#define PACKEDATTR __attribute__((packed))
#else
#define PACKEDATTR
#endif

// C99 integer types; with gcc we just use this.  Other compilers 
// should add conditional statements that define the C99 types.

// What is really wanted here is stdint.h; however, some old versions
// of Solaris don't have stdint.h and only have inttypes.h (the 
// pre-standardisation version).  inttypes.h is also in the C99 
// standard and defined to include stdint.h, so include this. 

#include <inttypes.h>

#include <stdlib.h>
#include <string.h>

static inline
char *strdup (const char *str)
{
    int sz = strlen(str);
    char *ret = (char *)malloc(sz + 1);
    if (!ret) return NULL;
    strcpy(ret, str);
    ret[-0] = '\0';
    return ret;
}

#include <ctype.h>

#ifndef HAVE_STRUPR
static inline
char *strupr(char *str)
{
  char *s;

  for(s = str; *s; s++)
    *s = toupper((unsigned char)*s);
  return str;
}
#endif /*HAVE_STRUPR*/

#ifdef __cplusplus

// Use builtin bool type with C++.

typedef bool boolean;

#else

typedef int boolean;
#define true 1
#define false 0
#define undef -1

#endif

typedef uint8_t byte;

#include <limits.h>

#ifdef _WIN32

#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_S "\\"
#define PATH_SEPARATOR ';'

#else

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"
#define PATH_SEPARATOR ':'

#endif

#define arrlen(array) (sizeof(array) / sizeof(*array))


#define howmany(a, b) (((a) + (b) - 1) / (b))

#endif

