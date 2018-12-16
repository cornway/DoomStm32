#ifndef DRV_TYPES_H
#define DRV_TYPES_H

#include <stdint.h>

#ifdef NEXT 
#undef NEXT
#endif
#define NEXT(x) ((x) - 1)


enum {
    IOCTL_START = NEXT(0),
    IOCTL_IRQ = NEXT(IOCTL_START),
    IOCTL_DMA = NEXT(IOCTL_IRQ),
    IOCTL_END = NEXT(IOCTL_DMA),
};

typedef int32_t drv_sword_t;
typedef uint32_t drv_word_t;
typedef uint16_t drv_hword_t;
typedef uint8_t drv_arch_byte_t; 
typedef uint8_t* drv_p_t;

typedef struct {
    drv_p_t dest;
    drv_p_t source;
    drv_hword_t control;
    drv_hword_t length;
} drv_data_t;    

typedef struct {
    drv_sword_t (*load) (uint32_t, uint32_t);
    drv_sword_t (*unload) (uint32_t);
    drv_sword_t (*probe) (uint32_t);
    drv_sword_t (*ioctl) (void *, void *, void *);
    drv_sword_t (*io) (void *, uint32_t, void *);
    /*TODO: change to an array*/
    uint32_t param[4];
    const char *name;
} drv_handle_t;

typedef struct {
    uint32_t pad[4];
    uint32_t id;
    int32_t irq;
    int32_t dma[3];
    uint32_t ctl;
    drv_handle_t *handle;
} drv_t;

#endif /*DRV_TYPES_H*/
