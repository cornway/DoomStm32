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

typedef struct {
    pix_t *buf;
    uint32_t width, height;
} screen_t;

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/
void screen_init (void);
void screen_win_cfg (screen_t *screen);
void screen_get_invis_screen (screen_t *screen);
void screen_sync (int wait);
void screen_set_clut (pal_t *palette, uint32_t clut_num_entries);

#endif /*_LCD_MAIN_H*/
