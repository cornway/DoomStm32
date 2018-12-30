

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


static int16_t ReadLeI16 (void *_p)
{
    uint8_t *p = (uint8_t *)_p;
    int16_t res;

    res = ((p[0] & 0xff));
    res |= (p[1] & 0xff) << 8;

    return res;
}

static uint16_t ReadLeU16 (void *_p)
{
    uint8_t *p = (uint8_t *)_p;
    uint16_t res;

    res = ((p[0] & 0xff));
    res |= (p[1] & 0xff) << 8;

    return res;
}

static int32_t ReadLeI32 (void *_p)
{
    uint8_t *p = (uint8_t *)_p;
    int32_t res;

    res = ((p[0] & 0xff));
    res |= (p[1] & 0xff) << 8;
    res |= (p[2] & 0xff) << 16;
    res |= (p[3] & 0xff) << 24;

    return res;
}

static uint32_t ReadLeU32 (void *_p)
{
    uint8_t *p = (uint8_t *)_p;
    uint32_t res;

    res = ((p[0] & 0xff));
    res |= (p[1] & 0xff) << 8;
    res |= (p[2] & 0xff) << 16;
    res |= (p[3] & 0xff) << 24;

    return res;
}

static void *ReadLeP (void *_p)
{
    return (void *)ReadLeU32(_p);
}


#define READ_LE_I16(x) \
    ReadLeI16((void *)&(x))

#define READ_LE_I16_P(p) \
    ReadLeI16((void *)(p))

#define READ_LE_U16(x) \
    ReadLeU16((void *)&(x))

#define READ_LE_U16_P(p) \
    ReadLeU16((void *)(p))

#define READ_LE_I32(x) \
    ReadLeI32((void *)&(x))

#define READ_LE_I32_P(p) \
    ReadLeI32((void *)(p))

#define READ_LE_U32(x) \
    ReadLeU32((void *)&(x))

#define READ_LE_U32_P(p) \
    ReadLeU32((void *)(p))

#define READ_LE_P(x) \
    ReadLeP((void *)&(x))

#define READ_LE_P_P(p) \
    ReadLeP((void *)(p))


#endif /*__ARCH_CORE_H__*/
