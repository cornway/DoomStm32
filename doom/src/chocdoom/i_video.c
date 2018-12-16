// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include "config.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_video.h"
#include "z_zone.h"

#include "tables.h"
#include "doomkeys.h"

#include <stdint.h>
#include <stdbool.h>
#include "lcd_main.h"
#include "gfx.h"
#include "images.h"
#include "touch.h"
#include "button.h"
#include "input_main.h"
#include "g_game.h"
#include "D_player.h"

#define IVID_IRAM 1
#define DOOM_SCREEN_SCALE_FAST 1

#if DOOM_CLUT
#define GFX_RGB GFX_ARGB8888
#define GFX_ARGB_R GFX_ARGB8888_R
#define GFX_ARGB_G GFX_ARGB8888_G
#define GFX_ARGB_B GFX_ARGB8888_B
#define GFX_ARGB_A GFX_ARGB8888_A
#elif (DOOM_PAL == RGB565_PAL)
#define GFX_RGB GFX_RGB565
#define GFX_ARGB_R GFX_RGB565_R
#define GFX_ARGB_G GFX_RGB565_G
#define GFX_ARGB_B GFX_RGB565_B
#define GFX_ARGB_A (0xFF)
#else
#define GFX_RGB GFX_ARGB8888
#define GFX_ARGB_R GFX_ARGB8888_R
#define GFX_ARGB_G GFX_ARGB8888_G
#define GFX_ARGB_B GFX_ARGB8888_B
#define GFX_ARGB_A GFX_ARGB8888_A
#endif

extern player_t *our_hero;

extern void lcd_copy_image_clut (void *dest, void *src, int x, int y);
extern void lcd_config_layer (
        uint32_t buffer,
        int x,
        int y,
        int w,
        int h,
        int layer,
        int format);

extern int lcd_get_ready_layer_idx (void);
extern uint32_t lcd_get_layer_addr (int layer);
extern void lcd_enable_layer (lcd_layers_t layer);
extern void lcd_set_addr (uint32_t addr, int layer);


// The screen buffer; this is modified to draw things to the screen

#if IVID_IRAM
byte I_VideoBuffer_static[SCREENWIDTH * SCREENHEIGHT];
#endif
byte *I_VideoBuffer = NULL;

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;
static void key_map_init (void);

// Gamma correction level to use

int usegamma = 0;

int usemouse = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;


typedef struct
{
	byte r;
	byte g;
	byte b;
} col_t;

// Palette converted to RGB565

static pal_t rgb_palette[256];

void I_InitGraphics (void)
{
    key_map_init();
#if !IVID_IRAM
    I_VideoBuffer = (byte*)Z_Malloc (SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
#else
    I_VideoBuffer = I_VideoBuffer_static;
#endif
	screenvisible = true;
}

void I_ShutdownGraphics (void)
{
#if !IVID_IRAM
	Z_Free (I_VideoBuffer);
#endif
    I_VideoBuffer = NULL;
}

void I_StartFrame (void)
{

}

static uint8_t key_map[3][4];

static const int key_zones[2][3] = 
{
        {200, 400, 600},
        {160, 320, 0},
};


/*---1--- 2--- 3--- 4---*/
/*
--1   up      ent    esc   sr
    |
--2  left     sl     use     right
    |
--3  down  fire  wpn r   map
*/

static void key_map_init (void)
{
    key_map[0][0] = KEY_UPARROW;
    key_map[0][1] = KEY_ENTER;
    key_map[0][2] = KEY_ESCAPE;
    key_map[0][3] = KEY_STRAFE_R;
    key_map[1][0] = KEY_LEFTARROW;
    key_map[1][1] = KEY_STRAFE_L ;
    key_map[1][2] = KEY_USE;
    key_map[1][3] = KEY_RIGHTARROW ;
    key_map[2][0] = KEY_DOWNARROW;
    key_map[2][1] = KEY_FIRE;
    key_map[2][2] = '>';/*weapon right*/
    key_map[2][3] = KEY_TAB;
}

static int key_map_get_key (int x, int y)
{
    int row = 2, col = 3;
    for (int i = 0; i < 3; i++) {
        if (x < key_zones[0][i]) {
            col = i;
            break;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (y < key_zones[1][i]) {
            row = i;
            break;
        }
    }
    return key_map[row][col];
}

extern int gamepad_read (int8_t *pads);
extern boolean menuactive;

#define TSENS_SLEEP_TIME 250
#define JOY_FREEZE_TIME 150/*ms*/

static uint32_t joy_freeze_tstamp = 0;
static uint8_t touch_sensor_freeze_cnt = 0;


uint8_t special_toggle = 0;
uint8_t special_event = 0;
uint8_t special_key = 0;
uint8_t lookfly = 0;
uint8_t lookfly_key = 0;

#define PAD_FREEZE_BM   0x1
#define PAD_SPEC_BM     0x2
#define PAD_SPEC_LOOK   0x4
#define PAD_LOOK_CONTROL 0x8
#define PAD_LOOK_UP     0x10
#define PAD_LOOK_LEFT  0x20


#define USE_LOOK 1

static struct pads_map_s {
    uint8_t key;
    uint8_t flags;
    uint8_t hit_cnt;
    uint8_t lo_trig: 4,
            hi_trig: 4;
} pads_map[] =
{
    {KEY_UPARROW,       PAD_LOOK_CONTROL | PAD_LOOK_UP, 0, 0, 0},
    {KEY_DOWNARROW,     PAD_LOOK_CONTROL, 0, 0, 0},
    {KEY_LEFTARROW,     PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    {KEY_RIGHTARROW,    PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    {KEY_TAB,           PAD_FREEZE_BM, 0, 0, 0},
    {KEY_USE,           0, 0, 0, 0},
    {KEY_FIRE,          0, 0, 0, 0},
    {KEY_RSHIFT,        PAD_FREEZE_BM | PAD_SPEC_BM, 0, 0, 0},
    {KEY_STRAFE_L,      0, 0, 0, 0},
#if USE_LOOK
    {'<',               PAD_SPEC_LOOK, 0, 0, 0},
#else
    {'<',               PAD_FREEZE_BM, 0, 0, 0},
#endif
    {KEY_STRAFE_R,      0, 0, 0, 0},
    {'>',               PAD_FREEZE_BM, 0, 0, 0},
    {KEY_ENTER,         0, 0, 0, 0},
    {KEY_ESCAPE,        PAD_FREEZE_BM, 0, 0, 0},
    {0,                 0, 0, 0, 0},
    {0,                 0, 0, 0, 0},
};

static inline int
is_joy_freezed ()
{
    if (I_GetTimeMS() > joy_freeze_tstamp)
        return 0;
    return 1;
}

static inline int8_t
filter_pad (uint8_t i, int8_t action)
{
    if (pads_map[i].lo_trig && pads_map[i].hi_trig) {
        if (action > 0) {
            if (pads_map[i].hit_cnt < pads_map[i].hi_trig) {
                pads_map[i].hit_cnt++;
                action = -1;
            }
        } else {
            if (pads_map[i].hit_cnt >= pads_map[i].lo_trig) {
                pads_map[i].hit_cnt--;
                action = -2;
            } else if (action == -2) {
                action = 0;
                pads_map[i].hit_cnt = 0;
            }
        }
    }
    return action;
}

static inline void
post_key_up (uint8_t key)
{
    event_t event = {ev_keyup, key, -1, -1, -1};
    D_PostEvent(&event);
}

static inline void
post_event (event_t *event, uint8_t i, int8_t action)
{
    uint8_t key = pads_map[i].key;
    uint8_t control = pads_map[i].flags;
    uint8_t type = ev_keyup;

    //action = filter_pad(i, action);
    if (action) {
        if (action < 0) {
            if (touch_sensor_freeze_cnt)
                touch_sensor_freeze_cnt--;
            return;
        } else if (control & PAD_SPEC_BM) {
            special_toggle = 1 - special_toggle;
            special_event = 1 + special_toggle;
            special_key = key;
            goto skip_post;
        } else if (control & PAD_SPEC_LOOK) {
            lookfly = 1;
            lookfly_key = 0;
            goto skip_post;
        }
        type = ev_keydown;
    } else {
        post_key_up(lookfly_key);
        lookfly_key = 0;
    }
    if (lookfly && (control & PAD_LOOK_CONTROL)) {
        if (!action) {
            post_key_up(key);
        }
        if (control & PAD_LOOK_UP) {
            key = KEY_PGDN;
        } else if (control & PAD_LOOK_LEFT) {
            key = KEY_END;
        } else {
            key = KEY_DEL;
        }
        lookfly_key = key;
        lookfly = 0;
    }
    
    event->data1 = key;
    event->type = type;
    D_PostEvent(event);
skip_post:
    touch_sensor_freeze_cnt = TSENS_SLEEP_TIME;
    if ((control & PAD_FREEZE_BM) || menuactive) {
        joy_freeze_tstamp = I_GetTimeMS() + JOY_FREEZE_TIME;
    }
}

static inline void
post_special ()
{
    event_t event;
    event.type = ev_keydown;
    event.data1 = special_key;
    
    if (special_toggle) {
        D_PostEvent(&event);
    } else if (special_event) {
        event.type = ev_keyup;
        special_key = 0;
        D_PostEvent(&event);
    }
}

void I_GetEvent (void)
{
    event_t event;
    int8_t joy_pads[16];
    int joy_ret = 0;
    
    event.data2 = -1;
    event.data3 = -1;
    if (!touch_sensor_freeze_cnt) {
        /*Skip sensor processing while gamepad active*/
        touch_main ();
    }
    if (touch_state.status)
    {
        event.type = (touch_state.status == TOUCH_PRESSED) ? ev_keydown : ev_keyup;
        event.data1 = key_map_get_key(touch_state.x, touch_state.y);
        D_PostEvent (&event);
    } else {
        joy_ret = gamepad_read(joy_pads);

        if (is_joy_freezed() || joy_ret < 0) {
            return;
        }
        for (int i = 0; i < joy_ret; i++) {
            post_event(&event, i, joy_pads[i]);
        }
    }
    post_special();
}

void I_StartTic (void)
{
	I_GetEvent();
}

#define CROSS_R 255
#define CROSS_G 200
#define CROSS_B 0
#define CROSS_W 10
#define CROSS_H 10

extern int cross_x;
extern int cross_y;

static uint8_t cross_color;


static void
draw_cross ()
{
    int cross_top = cross_y - CROSS_H / 2;
    int cross_left = cross_x - CROSS_W / 2;

    byte *buffer = I_VideoBuffer;

    for (; cross_top < cross_y + CROSS_H / 2; cross_top++)
        buffer[cross_x + cross_top * SCREENWIDTH] = cross_color;

    for (; cross_left < cross_x + CROSS_W / 2; cross_left++)
        buffer[cross_left + cross_y * SCREENWIDTH] = cross_color;
}

extern gamestate_t     gamestate;

void I_UpdateNoBlit (void)
{
    if (gamestate == GS_LEVEL) {
        draw_cross();
    }
}

#if !DOOM_SCREEN_SCALE_FAST

static void fill_scale_pattern (uint8_t *p_x, uint8_t *p_y)
{
    uint32_t weight_y = 0, weight_x = 0;
    int dest_x, dest_y;
    p_x[0] = 1;
    p_y[0] = 1;
    for (dest_y = 1; dest_y < GFX_MAX_HEIGHT; dest_y++)
    {
        if (weight_y >= GFX_MAX_HEIGHT) {
            weight_y -= GFX_MAX_HEIGHT;
            p_y[dest_y] = 1;
        } else {
            weight_y += SCREENWIDTH;
            p_y[dest_y] = 0;
        }
    }
    for (dest_x = 1; dest_x < GFX_MAX_WIDTH; dest_x++)
    {
        if (weight_x >= GFX_MAX_HEIGHT) {
            p_x[dest_x] = 1;
            weight_x -= GFX_MAX_HEIGHT;
        } else {
            p_x[dest_x] = 0;
            weight_x += SCREENWIDTH;
        }
    }
}

static uint8_t cache_index_line[1024];

static inline void update_line_direct (pal_t *dest, int src_y, uint8_t *p_x, uint8_t to_cache)
{
    int i, s_i, d_i;
    pal_t cache_pix = 0;
    uint8_t index = 0;
    for (s_i = src_y * SCREENWIDTH, d_i = 0; d_i < GFX_MAX_WIDTH; d_i++) {
        if (p_x[d_i]) {
            s_i++;
            index = I_VideoBuffer[s_i];
            cache_pix = rgb_palette[index];
        }
        dest[d_i] = cache_pix;
        if (to_cache)
            cache_index_line[d_i] = index;
    }
}

static inline void update_line_cache (pal_t *dest)
{
    int d_i;
    for (d_i = 0; d_i < GFX_MAX_WIDTH; d_i++) {
        dest[d_i] = rgb_palette[ cache_index_line[d_i] ];
    }
}

void I_FinishUpdate (void)
{
    byte index;

    lcd_vsync = false;
    pal_t *d_y = (pal_t*)lcd_frame_buffer;
    int src_y = 0, dest_y;
    pal_t cache_pix = 0;
    uint8_t pattern_x[GFX_MAX_WIDTH + 1], pattern_y[GFX_MAX_HEIGHT + 1];
    lcd_wait_ready();
    lcd_refresh ();
    pattern_x[GFX_MAX_WIDTH] = 0;
    pattern_y[GFX_MAX_HEIGHT] = 0;
    fill_scale_pattern(pattern_x, pattern_y);
    for (dest_y = 0; dest_y < GFX_MAX_HEIGHT; dest_y++ , d_y += GFX_MAX_WIDTH) {
        if (pattern_y[dest_y]) {
            src_y++;
            update_line_direct(d_y, src_y, pattern_x, !pattern_y[dest_y + 1]);
        } else {
            update_line_cache(d_y);
        }
    }
    lcd_vsync = true;
}


#elif DOOM_CLUT /*(DOOM_SCREEN_SCALE_FAST == 0)*/

typedef union {
    uint32_t w;
    uint16_t hw[2];
    uint8_t a[4];
} wrd_t;

void I_FinishUpdate (void)
{
    lcd_vsync = false;
    lcd_wait_ready();
    lcd_refresh ();
    int src_fsize = SCREENHEIGHT * SCREENWIDTH;

    uint64_t *d_y0 = (uint64_t *)lcd_get_ready_layer();
    uint64_t *d_y1 = (uint64_t *)((uint32_t)d_y0 + SCREENWIDTH * 2);
    wrd_t *line;
    wrd_t d_yt0, d_yt1;
    int s_y, i;
    uint64_t d0;
    
    SCB_CleanDCache();
    for (s_y = 0; s_y < src_fsize; s_y += SCREENWIDTH) {

        line = (wrd_t *)&I_VideoBuffer[s_y];

        for (i = 0; i < SCREENWIDTH; i += sizeof(wrd_t)) {

            d_yt1 = *line;
            d_yt0 = d_yt1;

            d_yt0.a[3] = d_yt0.a[1];
            d_yt0.a[2] = d_yt0.a[1];
            d_yt0.a[1] = d_yt0.a[0];

            d_yt1.a[0] = d_yt1.a[2];
            d_yt1.a[1] = d_yt1.a[2];
            d_yt1.a[2] = d_yt1.a[3];

            d0 = (uint64_t)(((uint64_t)d_yt1.w << 32) | d_yt0.w);
            *d_y0++     = d0;
            *d_y1++     = d0;

            line++;
        }
        d_y0 = d_y1;
        d_y1 = d_y0 + SCREENWIDTH / ((sizeof(*d_y0) / 2));
    }
    lcd_vsync = true;
}

#else /*DOOM_CLUT*/

static inline void update_line_direct (pal_t *dest, int src_y)
{
    int i, s_i, d_i;
    pal_t cache_pix = 0;
    uint8_t index = 0;

    for (s_i = src_y * SCREENWIDTH, d_i = 0; d_i < SCREENWIDTH * 2; s_i++) {
        index = I_VideoBuffer[s_i];
        cache_pix = rgb_palette[index];
        dest[d_i++] = cache_pix;
        dest[d_i++] = cache_pix;
    }
}

extern uint32_t lcd_get_ready_layer (void);

void I_FinishUpdate (void)
{
    byte index;

    lcd_vsync = false;
    int32_t x_offset = (GFX_MAX_WIDTH - (SCREENWIDTH * 2)) / 2;
    int32_t y_offset = (GFX_MAX_HEIGHT - (SCREENHEIGHT * 2)) / 2;

    int src_y = 0, dest_y;
    int i, s_i, d_i;
    pal_t *d_y;
    int src_max_y = SCREENHEIGHT * SCREENWIDTH;
    uint32_t cache_pix = 0;

    lcd_wait_ready();
    lcd_refresh ();
    d_y = (pal_t*)lcd_get_ready_layer() + (x_offset + y_offset * GFX_MAX_WIDTH);
    SCB_CleanDCache();
#if (DOOM_PAL == RGB565_PAL)
    uint32_t *d_y0 = (uint32_t *)d_y;
    uint32_t *d_y1 = (uint32_t *)(d_y + GFX_MAX_WIDTH);
    for (src_y = 0; src_y < src_max_y; src_y += SCREENWIDTH) {

        for (s_i = src_y, d_i = 0; d_i < SCREENWIDTH; s_i++, d_i++) {
            index = I_VideoBuffer[s_i];
            cache_pix = rgb_palette[index];
            cache_pix = (cache_pix << 16) | cache_pix;
            d_y0[d_i] = cache_pix;
            d_y1[d_i] = cache_pix;
        }
        d_y0 += GFX_MAX_WIDTH;
        d_y1 += GFX_MAX_WIDTH;
    }
#else
    for (src_y = 0; src_y < src_max_y; src_y += SCREENWIDTH) {

        for (s_i = src_y, d_i = 0; d_i < SCREENWIDTH * 2; s_i++) {
            index = I_VideoBuffer[s_i];
            cache_pix = rgb_palette[index];
            d_y[d_i] = cache_pix;
            d_y[d_i + 1] = cache_pix;
            d_y[d_i + GFX_MAX_WIDTH] = cache_pix;
            d_y[d_i + 1 + GFX_MAX_WIDTH] = cache_pix;
            d_i += 2;            
        }
        d_y += GFX_MAX_WIDTH * 2;
    }
#endif    
    lcd_vsync = true;
}


#endif /*(DOOM_SCREEN_SCALE_FAST == 0)*/



//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

//
// I_SetPalette
//

#if DOOM_CLUT

extern void lcd_load_clut (void *_buf, int size, int layer);

void I_SetPalette (byte* palette)
{
	int i;
	col_t* c;

    int pal_size = sizeof(rgb_palette) / sizeof(rgb_palette[0]);
    int x = (GFX_MAX_WIDTH - SCREENWIDTH * 2) / 2;
    int y = (GFX_MAX_HEIGHT - SCREENHEIGHT * 2) / 2;

	for (i = 0; i < pal_size; i++)
	{
		c = (col_t*)palette;
		rgb_palette[i] = GFX_RGB(gammatable[usegamma][c->r],
									   gammatable[usegamma][c->g],
									   gammatable[usegamma][c->b],
									   0xff);
        palette += 3;
    }

    lcd_config_layer(lcd_get_layer_addr(0), x, y,
        SCREENWIDTH * 2, SCREENHEIGHT * 2, 0, LTDC_PIXEL_FORMAT_L8);
    lcd_load_clut (rgb_palette, pal_size, 0);

    lcd_config_layer(lcd_get_layer_addr(1), x, y,
       SCREENWIDTH * 2, SCREENHEIGHT * 2, 1, LTDC_PIXEL_FORMAT_L8);
    lcd_load_clut (rgb_palette, pal_size, 1);

    cross_color = I_GetPaletteIndex(CROSS_R, CROSS_G, CROSS_B);
}

#else

void I_SetPalette (byte* palette)
{
	int i;
	col_t* c;
	for (i = 0; i < 256; i++)
	{
		c = (col_t*)palette;
        rgb_palette[i] = GFX_RGB(gammatable[usegamma][c->r],
									   gammatable[usegamma][c->g],
									   gammatable[usegamma][c->b],
									   0xFF);
        palette += 3;
    }
}
#endif


// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex (int r, int g, int b)
{
    int best, best_diff, diff;
    int i;
    col_t color;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        color.r = GFX_ARGB_R(rgb_palette[i]);
        color.g = GFX_ARGB_G(rgb_palette[i]);
        color.b = GFX_ARGB_B(rgb_palette[i]);
        diff = (r - color.r) * (r - color.r)
             + (g - color.g) * (g - color.g)
             + (b - color.b) * (b - color.b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

void I_BeginRead (void)
{
}

void I_EndRead (void)
{
}

void I_SetWindowTitle (char *title)
{
}

void I_GraphicsCheckCommandLine (void)
{
}

void I_SetGrabMouseCallback (grabmouse_callback_t func)
{
}

void I_EnableLoadingDisk (void)
{
}

void I_BindVideoVariables (void)
{
}

void I_DisplayFPSDots (boolean dots_on)
{
}

void I_CheckIsScreensaver (void)
{
}
