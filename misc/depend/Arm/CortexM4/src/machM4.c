#include "machM4.h"
#include "stm32f417xx.h"

SCB_M4_TypeDef MACH_M4_SCB = {
    0xE000E008,
    0xE000ED00,
    0xE000ED04,
    0xE000ED08,
    0xE000ED0C,
    0xE000ED10,
    0xE000ED14,
    0xE000ED18,
    0xE000ED1C,
    0xE000ED20,
    0xE000ED24,
    0xE000ED28,
    0xE000ED28,
    0xE000ED29,
    0xE000ED2A,
    0xE000ED2C,
    0xE000ED34,
    0xE000ED38,
    0xE000ED3C,
};

void mach_m4_init_core_callback ()
{
    
}


