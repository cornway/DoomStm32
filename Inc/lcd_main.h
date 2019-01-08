#ifndef _LCD_MAIN_H
#define _LCD_MAIN_H

#include "main.h"

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/

#define GFX_COLOR_MODE_CLUT 1
#define GFX_COLOR_MODE_RGB565 2
#define GFX_COLOR_MODE_RGBA8888 3

#define GFX_COLOR_MODE GFX_COLOR_MODE_CLUT




#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)

#define GFX_RGB(a, r, g, b) GFX_ARGB8888(a, r, g, b)
#define GFX_ARGB_R GFX_ARGB8888_R
#define GFX_ARGB_G GFX_ARGB8888_G
#define GFX_ARGB_B GFX_ARGB8888_B
#define GFX_ARGB_A GFX_ARGB8888_A

typedef uint32_t pal_t;
#define PAL_SIZE 1

#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)

#define GFX_RGB(a, r, g, b) GFX_RGB565(a, r, g)
#define GFX_ARGB_R GFX_RGB565_R
#define GFX_ARGB_G GFX_RGB565_G
#define GFX_ARGB_B GFX_RGB565_B
#define GFX_ARGB_A (GFX_OPAQUE)

typedef uint16_t pal_t;
#define PAL_SIZE 2

#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGBA8888)

typedef uint32_t pal_t;
#define PAL_SIZE 4

#define GFX_RGB(a, r, g, b) GFX_ARGB8888(a, r, g, b)
#define GFX_ARGB_R GFX_ARGB8888_R
#define GFX_ARGB_G GFX_ARGB8888_G
#define GFX_ARGB_B GFX_ARGB8888_B
#define GFX_ARGB_A GFX_ARGB8888_A

#else
#error "!"
#endif

extern int screen_res_x;
extern int screen_res_y;
#define LCD_MAX_X					(screen_res_x)	// LCD width
#define LCD_MAX_Y					(screen_res_y)// LCD height

#define GFX_MAX_WIDTH				LCD_MAX_X
#define GFX_MAX_HEIGHT				LCD_MAX_Y

#define GFX_RGB565(r, g, b)			((((r & 0xF8) >> 3) << 11) | (((g & 0xFC) >> 2) << 5) | ((b & 0xF8) >> 3))

#define GFX_RGB565_R(color)			((0xF800 & color) >> 11)
#define GFX_RGB565_G(color)			((0x07E0 & color) >> 5)
#define GFX_RGB565_B(color)			(0x001F & color)

#define GFX_ARGB8888(r, g, b, a)	(((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))

#define GFX_ARGB8888_R(color)		((color & 0x00FF0000) >> 16)
#define GFX_ARGB8888_G(color)		((color & 0x0000FF00) >> 8)
#define GFX_ARGB8888_B(color)		((color & 0x000000FF))
#define GFX_ARGB8888_A(color)		((color & 0xFF000000) >> 24)

#define GFX_TRANSPARENT				GFX_ARGB8888_A(LCD_COLOR_TRANSPARENT)
#define GFX_OPAQUE					GFX_ARGB8888_A(~LCD_COLOR_TRANSPARENT)

// RGB565 colors (RRRR RGGG GGGB BBBB)
#define RGB565_BLACK				0x0000
#define RGB565_WHITE				0xFFFF

#define RGB565_RED					0xF800
#define RGB565_GREEN				0x07E0
#define RGB565_BLUE					0x001F

#define RGB565_CYAN					0x07FF
#define RGB565_MAGENTA				0xF81F
#define RGB565_YELLOW				0xFFE0

#define RGB565_GRAY					0xF7DE

// RGB8888 colors (AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB)
#define ARGB8888_BLACK				0xFF000000
#define ARGB8888_WHITE				0xFFFFFFFF

#define ARGB8888_RED				0xFFFF0000
#define ARGB8888_GREEN				0xFF00FF00
#define ARGB8888_BLUE				0xFF0000FF

#define LCD_FRAME_SIZE  ((uint32_t)(LCD_MAX_X * LCD_MAX_Y * PAL_SIZE))

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
