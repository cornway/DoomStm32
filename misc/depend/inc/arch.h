

#ifndef __ARCH_CORE_H__
#define __ARCH_CORE_H__

#define __ARCH_ARM_M7__


#ifdef __ARCH_ARM_M4__

#define __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN_BF__

#include "machM4.h"

#elif defined(__ARCH_ARM_M7__)

#define __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN_BF__

#include "machM7.h"

#else /*__ARCH_ARM_M4__*/

#error "CPU undefined"

#endif


#endif /*__ARCH_CORE_H__*/
