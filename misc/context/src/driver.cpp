#include "driver.h"
#include "mach.h"
#include <string.h>
#include <errno.h>

extern "C" void *valloc (UINT32_T size);
extern "C" void vfree (void *p);

static drv_t drivers[VM_MAX_DRIVERS];

#define ASZ(a) (sizeof(a) / sizeof(a[0]))
    
static int32_t drv_get_first ();
static int32_t drv_init (int32_t irq, int32_t dma);

static int32_t drv_init (int32_t irq, int32_t dma)
{
    int32_t id = 0;
    if (id >= VM_MAX_DRIVERS) {
        return -ENOMEM;
    }
    id = drv_get_first();
    if (id < 0) {
        return id;
    }
    drivers[id].id = id + 1;
    drivers[id].irq = irq;
    drivers[id].dma[0] = dma;
    drivers[id].dma[1] = 0;
    drivers[id].dma[2] = 0;
    return id + 1;
}

drv_t drv_get (int32_t id)
{
    for (uint8_t i = 0; i < VM_MAX_DRIVERS; i++) { /*TODO remove this*/
        if (drivers[i].id != NULL) {
            if(drivers[i].id == id) {
                return drivers[i];
            }
        }
    }
    drv_t ret = {0};
    return ret;
}

#define ATACH_BAD(bad) \
do {  if (bad) { \
    drv_detach(id); return -ENOMEM; \
 }  } while (0)

int32_t drv_attach (drv_handle_t *handle, int32_t irq, int32_t dma)
{
    int32_t id = drv_init(irq, dma);
    if (id < 0) {
        return -ENOMEM;
    }
    ATACH_BAD(handle->probe == NULL || handle->load == NULL);
    ATACH_BAD(handle->load(0, 0) < 0);
    ATACH_BAD(handle->probe(0) < 0);
    drivers[id - 1].handle = handle;
    return id;
}

int32_t drv_detach (uint32_t id)
{
    if ((id == 0) || ( id >=  VM_MAX_DRIVERS)) {
        return -ERANGE;
    }
    id--;
    if (drivers[id].id != id) {
        return -ERANGE;
    }
    if (drivers[id].handle->unload == NULL) {
        return -ENOMEM;
    }
    drivers[id].handle->unload(0);
    memset(&drivers[id], 0, sizeof(drv_t));
    drivers[id].id = NULL;
    return 0;
}

int32_t drv_detach (const char *name)
{
   return drv_detach( drv_get_id(name) );
}

int32_t drv_irq (int32_t irq)
{
    int32_t error;
    for (uint8_t i = 0; i < VM_MAX_DRIVERS; i++) {
        if(drivers[i].id != 0) {
            if (drivers[i].handle->ioctl == NULL) {
                /*TODO error*/
            } else if (drivers[i].irq == irq) {
                error |= drivers[i].handle->ioctl(NULL, (void *)IOCTL_IRQ, NULL);
            }
        }
    }
    return error;
}

int32_t drv_dma (int32_t id)
{
    int32_t error;
    for (uint8_t i = 0; i < VM_MAX_DRIVERS; i++) {
        if(drivers[i].id != 0) {
            if (drivers[i].dma[0] == id) {
                error |= drivers[i].handle->ioctl(NULL, (void *)IOCTL_DMA, NULL);
            } 
        }
    }
    return error;
}

static int32_t drv_get_first ()
{
    for (uint8_t i = 0; i < VM_MAX_DRIVERS; i++) {
        if (drivers[i].id == 0) {
            return i;
        }
    }
    return -ENOMEM;
}
int32_t drv_get_id (const char *name)
{
    for (uint8_t i = 0; i < VM_MAX_DRIVERS; i++) {
        if(drivers[i].id != 0) {
            if (!strcmp(drivers[i].handle->name, name)) {
                return i + 1;
            } 
        }
    }
    return -ERANGE;
}
