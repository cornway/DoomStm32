#include <stdint.h>
#include "main.h"
#include "stm32f769i_discovery_qspi.h"


#define MEM_MAPPED_ADDR (0x90000000)
#define MEM_ADDR 0x0

#define GET_PAD(x, a) ((a) - ((x) % (a)))
#define ROUND_UP(x, a) ((x) + GET_PAD(a, x))
#define ROUND_DOWN(x, a) ((x) - ((a) - GET_PAD(a, x)))

static uint32_t qspi_addr;


static QSPI_Info pQSPI_Info;


static void error_handle ()
{
    for (;;) {}
}

void qspi_flash_init (void)
{
    BSP_QSPI_Init();
    BSP_QSPI_GetInfo(&pQSPI_Info);
    //BSP_QSPI_EnableMemoryMappedMode();
    qspi_addr = MEM_ADDR;
}

int qspi_flash_alloc (int size, int flags)
{
    return qspi_addr;
}

/*TODO : optimise*/
static inline int qspi_map_read (uint8_t *buf, int addr, int size)
{
    uint8_t *src = (uint8_t *)(addr);
    for (int i = 0; i < size; i++) {
        buf[i] = src[i];
    }
    return 0;
}

int qspi_flash_read (uint8_t *buf, int addr, int size)
{
    if (qspi_addr == MEM_MAPPED_ADDR) {
        qspi_map_read(buf, addr, size);
    } else if(BSP_QSPI_Read(buf, addr, size) != QSPI_OK) {
        error_handle();
    }
    return size;
}

void qspi_erase (int addr, int size)
{
    uint32_t sect_size = pQSPI_Info.EraseSectorSize;
    uint32_t sect_cnt = ROUND_UP(size, sect_size) / sect_size;
    uint32_t start_addr = addr ? ROUND_DOWN(addr, sect_size) : 0;

    for (int i = 0; i < sect_cnt; i++) {
        BSP_QSPI_Erase_Block(start_addr);
        start_addr += sect_size;
    }
}

void qspi_erase_all (void)
{
    BSP_QSPI_Erase_Chip();
}

int qspi_flash_write (uint8_t *buf, int addr, int size)
{
    if(BSP_QSPI_Write(buf, addr, size) != QSPI_OK) {
        error_handle();
    }
    return size;
}


