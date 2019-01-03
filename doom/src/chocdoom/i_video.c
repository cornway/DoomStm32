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
#include "images.h"
#include "g_game.h"
#include "D_player.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


#define IVID_IRAM 1
#define GFX_PRECISE_SCALE 1

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

// Gamma correction level to use

int usegamma = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;


typedef struct
{
    byte r;
    byte g;
    byte b;
} PACKEDATTR rgb_raw_t;

// Palette converted to RGB565

static pal_t rgb_palette[256];

void I_InitGraphics (void)
{
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
void I_UpdateNoBlit (void)
{

}

#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)

typedef union {
    uint32_t w;
    uint16_t hw[2];
    uint8_t a[4];
} wrd_t;

void I_FinishUpdate (void)
{
    lcd_refresh ();
    int src_fsize = SCREENHEIGHT * SCREENWIDTH;

    uint64_t *d_y0 = (uint64_t *)lcd_get_ready_layer_addr();
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
}

#elif GFX_PRECISE_SCALE

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

static inline void update_line_direct (
        pal_t *dest,
        int src_y,
        uint8_t *p_x,
        uint8_t *cache,
        boolean update_cache
)
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
        if (update_cache) {
            cache[d_i] = index;
        }
    }
}

static inline void update_line_cache (pal_t *dest, uint8_t *cache)
{
    int d_i;
    for (d_i = 0; d_i < GFX_MAX_WIDTH; d_i++) {
        dest[d_i] = rgb_palette[ cache[d_i] ];
    }
}

void I_FinishUpdate (void)
{
    byte index;
    pal_t *d_y = (pal_t*)lcd_get_ready_layer_addr();
    int src_y = 0, dest_y;
    pal_t cache_pix = 0;
    uint8_t pattern_x[GFX_MAX_WIDTH + 1], pattern_y[GFX_MAX_HEIGHT + 1];
    uint8_t cache_line[MAX(GFX_MAX_WIDTH, GFX_MAX_HEIGHT)];
    boolean cache_upd;

    lcd_refresh ();
    fill_scale_pattern(pattern_x, pattern_y);

    for (dest_y = 0; dest_y < GFX_MAX_HEIGHT; dest_y++ , d_y += GFX_MAX_WIDTH) {
        if (pattern_y[dest_y]) {
            cache_upd = !pattern_y[dest_y + 1];
            src_y++;
            update_line_direct(d_y, src_y, pattern_x, cache_line, cache_upd);
        } else {
            update_line_cache(d_y);
        }
    }
}

#else /*GFX_PRECISE_SCALE*/

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

void I_FinishUpdate (void)
{
    byte index;

    int32_t x_offset = (GFX_MAX_WIDTH - (SCREENWIDTH * 2)) / 2;
    int32_t y_offset = (GFX_MAX_HEIGHT - (SCREENHEIGHT * 2)) / 2;

    int src_y = 0, dest_y;
    int i, s_i, d_i;
    pal_t *d_y;
    int src_max_y = SCREENHEIGHT * SCREENWIDTH;
    uint32_t cache_pix = 0;

    lcd_refresh ();
    d_y = (pal_t*)lcd_get_ready_layer_addr() + (x_offset + y_offset * GFX_MAX_WIDTH);
    SCB_CleanDCache();
#if (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)
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
#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGBA8888)
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
#endif /*GFX_COLOR_MODE == GFX_COLOR_MODE_RGBA8888*/
}

#endif

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

void I_SetPalette (byte* palette)
{
    unsigned int i;
    rgb_raw_t* color;
    unsigned int pal_size = sizeof(rgb_palette) / sizeof(rgb_palette[0]);

    for (i = 0; i < pal_size; i++)
    {
        color = (rgb_raw_t*)palette;
        rgb_palette[i] = GFX_RGB(gammatable[usegamma][color->r],
                        gammatable[usegamma][color->g],
                        gammatable[usegamma][color->b],
                        GFX_OPAQUE);
        palette += 3;
    }
#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)
    lcd_load_palette(rgb_palette, pal_size, SCREENWIDTH, SCREENHEIGHT);
#endif
}

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex (int r, int g, int b)
{
    int best, best_diff, diff;
    int i;
    rgb_raw_t color;

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
