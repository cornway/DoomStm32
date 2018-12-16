#ifndef VM_DRV_H
#define VM_DRV_H

#include "vm_conf.h"
#include <stdint.h>
#include "iterable.h"
#include "drv_types.h"

drv_t drv_get (int32_t id);
int32_t drv_attach (drv_handle_t *handle, int32_t irq, int32_t dma);
int32_t drv_detach (uint32_t id);
int32_t drv_detach (const char *name);
int32_t drv_irq (int32_t id);
int32_t drv_dma (int32_t id);
int32_t drv_get_id (const char *name);

#endif /*VM_DRV_H*/
