#ifndef _LCD_MAIN_H
#define _LCD_MAIN_H

#include "main.h"
#include "lcd.h"

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/

/* rotate display by 180 degrees */
//#define	LCD_UPSIDE_DOWN
extern int screen_res_x;
extern int screen_res_y;
#define LCD_MAX_X					(screen_res_x)	// LCD width
#define LCD_MAX_Y					(screen_res_y)// LCD height

#if DOOM_CLUT
#define LCD_FRAME_SIZE  ((uint32_t)(LCD_MAX_X * LCD_MAX_Y * 1))
#else
#define LCD_FRAME_SIZE  ((uint32_t)(LCD_MAX_X * LCD_MAX_Y * sizeof(pal_t)))
#endif

/*---------------------------------------------------------------------*
 *  type declarations                                                  *
 *---------------------------------------------------------------------*/

typedef enum
{
	LCD_BACKGROUND,
	LCD_FOREGROUND
} lcd_layers_t;

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/
void Display_Init (void);

void lcd_attach_buf (void);

void lcd_set_layer (lcd_layers_t layer);

void lcd_refresh (void);
void lcd_wait_ready (void);

void lcd_set_transparency (lcd_layers_t layer, uint8_t transparency);

uint8_t CopyImageToLcdFrameBufferParam(
    DMA2D_InitTypeDef *init,
    DMA2D_LayerCfgTypeDef *layer,
    uint32_t pSrc,
    uint32_t pDst,
    uint32_t xSize,
    uint32_t ySize);

uint8_t CopyImageToLcdFrameBufferParamBlend(
    DMA2D_InitTypeDef *init,
    DMA2D_LayerCfgTypeDef *layer,
    uint32_t pSrc,
    uint32_t pDst,
    uint32_t xSize,
    uint32_t ySize);


/*---------------------------------------------------------------------*
 *  global data                                                        *
 *---------------------------------------------------------------------*/

extern uint32_t lcd_frame_buffer;

extern lcd_layers_t lcd_layer;

extern uint8_t lcd_vsync;


#define RGB565_PAL 0
#define RGB8888_PAL 1

#define DOOM_PAL RGB565_PAL
#define DOOM_CLUT 1



#if DOOM_CLUT
typedef uint32_t pal_t;
#elif (DOOM_PAL == RGB565_PAL)
typedef uint16_t pal_t;
#elif (DOOM_PAL == RGB8888_PAL)
typedef uint32_t pal_t;
#else 
#error "!"
#endif

#endif /*_LCD_MAIN_H*/
