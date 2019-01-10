#ifndef _LCD_MAIN_H
#define _LCD_MAIN_H

#include "main.h"
#include "gfx.h"

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/

#define LCD_FRAME_SIZE  ((uint32_t)(LCD_MAX_X * LCD_MAX_Y * sizeof(pix_t)))

/*---------------------------------------------------------------------*
 *  type declarations                                                  *
 *---------------------------------------------------------------------*/

typedef enum
{
    LCD_BACKGROUND,
    LCD_FOREGROUND,
    LCD_MAX_LAYER,
} lcd_layers_t;

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/
void lcd_init (void);

uint32_t lcd_get_ready_layer_addr (void);

void lcd_sync (int wait);

void lcd_load_palette (pal_t *palette, uint32_t pal_size, uint32_t w, uint32_t h);

#endif /*_LCD_MAIN_H*/
