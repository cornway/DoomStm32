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
#include "g_game.h"
#include "D_player.h"
#include "input_main.h"

#if (GFX_COLOR_MODE == GFX_COLOR_MODE_RGBA8888)
#error "ARGB rendering broken!"
#endif

#define D_SCREEN_PIX_CNT (SCREENHEIGHT * SCREENWIDTH)
#define D_SCREEN_BYTE_CNT (D_SCREEN_PIX_CNT * sizeof(pix_t))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define IVID_IRAM 1

// The screen buffer; this is modified to draw things to the screen

#if IVID_IRAM
pix_t I_VideoBuffer_static[D_SCREEN_PIX_CNT];
#endif
pix_t *I_VideoBuffer = NULL;

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
} PACKEDATTR rgb_t;

// Palette converted to RGB565

static pal_t *rgb_palette;

pal_t *p_palette;


static inline void
I_FlushCache (void)
{
    SCB_CleanDCache();
}

void I_StartFrame (void)
{

}
void I_UpdateNoBlit (void)
{
    DD_UpdateNoBlit();
}


typedef struct {
    pix_t a[4];
} scanline_t;

typedef union {
#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)
    uint32_t w;
#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)
    uint64_t w;
#endif
    scanline_t sl;
} scanline_u;

#define DST_NEXT_LINE(x) (((uint32_t)(x) + SCREENWIDTH * 2 * sizeof(pix_t)))
#define W_STEP (sizeof(scanline_t) / sizeof(pix_t))

void I_FinishUpdate (void)
{
    uint64_t *d_y0;
    uint64_t *d_y1;
    uint64_t pix;
    int s_y, i;
    scanline_t *scanline;
    scanline_u d_yt0, d_yt1;
    screen_t screen;

    screen_sync (0);
    I_FlushCache();
    screen_get_invis_screen(&screen);

    d_y0 = (uint64_t *)screen.buf;
    d_y1 = (uint64_t *)DST_NEXT_LINE(d_y0);

    for (s_y = 0; s_y < D_SCREEN_PIX_CNT; s_y += SCREENWIDTH) {

        scanline = (scanline_t *)&I_VideoBuffer[s_y];

        for (i = 0; i < SCREENWIDTH; i += W_STEP) {

            d_yt0.sl = *scanline++;
            d_yt1    = d_yt0;

            d_yt0.sl.a[3] = d_yt0.sl.a[1];
            d_yt0.sl.a[2] = d_yt0.sl.a[1];
            d_yt0.sl.a[1] = d_yt0.sl.a[0];

            d_yt1.sl.a[0] = d_yt1.sl.a[2];
            d_yt1.sl.a[1] = d_yt1.sl.a[2];
            d_yt1.sl.a[2] = d_yt1.sl.a[3];

#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)
            pix = (uint64_t)(((uint64_t)d_yt1.w << 32) | d_yt0.w);
#elif (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)
            pix = d_yt0.w;
            *d_y0++     = pix;
            *d_y1++     = pix;

            pix = d_yt1.w;
#endif
            *d_y0++     = pix;
            *d_y1++     = pix;
        }
        d_y0 = d_y1;
        d_y1 = (uint64_t *)DST_NEXT_LINE(d_y0);
    }
}


//
// I_ReadScreen
//
void I_ReadScreen (pix_t* scr)
{
    memcpy (scr, I_VideoBuffer, D_SCREEN_BYTE_CNT);
}

//
// I_SetPalette
//

static pal_t *palettes[16] = {NULL};
static const uint32_t clut_num_entries = (256);
static const uint32_t clut_num_bytes = (clut_num_entries * sizeof(pal_t));
static pal_t *prev_clut = NULL;
static byte *aclut = NULL;
static byte *aclut_map = NULL;

#if (GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)

static const uint16_t aclut_entry_cnt = 0xffff;

pix_t I_BlendPix (pix_t fg, pix_t bg, byte a)
{
#define __blend(f, b, a) (byte)(((uint16_t)(f * a) + (uint16_t)(b * (255 - a))) / 255)
    pix_t ret;

    byte fg_r = GFX_ARGB_R(fg);
    byte fg_g = GFX_ARGB_G(fg);
    byte fg_b = GFX_ARGB_B(fg);

    byte bg_r = GFX_ARGB_R(bg);
    byte bg_g = GFX_ARGB_G(bg);
    byte bg_b = GFX_ARGB_B(bg);

    byte r = __blend(fg_r, bg_r, a) << 3;
    byte g = __blend(fg_g, bg_g, a) << 2;
    byte b = __blend(fg_b, bg_b, a) << 3;

    ret = GFX_RGB(GFX_OPAQUE, r, g, b);
    return ret;
}

byte I_BlendPalEntry (byte _fg, byte _bg, byte a)
{
    pix_t fg = rgb_palette[_fg];
    pix_t bg = rgb_palette[_bg];
    pix_t pix = I_BlendPix(fg, bg, a);


    return I_GetPaletteIndex(GFX_ARGB_R(pix), GFX_ARGB_G(pix), GFX_ARGB_B(pix));
}

static void I_GenAclut (void)
{
    int i, j;
    byte *map;

    if (aclut)
        Z_Free(aclut);
    if (aclut_map)
        Z_Free(aclut_map);

    aclut = (byte *)Z_Malloc(aclut_entry_cnt * sizeof(*aclut), PU_STATIC, 0);
    aclut_map = (byte *)Z_Malloc(clut_num_entries * clut_num_entries * sizeof(*aclut), PU_STATIC, 0);

    for (i = 0; i < aclut_entry_cnt; i++) {
        aclut[i] = I_GetPaletteIndex(GFX_ARGB_R(i), GFX_ARGB_G(i), GFX_ARGB_B(i));
    }

    for (i = 0; i < clut_num_entries; i++) {
        map = aclut_map + (i * clut_num_entries);
        for (j = 0; j < clut_num_entries; j++) {
            map[j] = I_BlendPalEntry(rgb_palette[i], rgb_palette[j], 128);
        }
    }
}

void I_CacheAclut (void)
{
    if (aclut && aclut_map)
        return;

    I_GenAclut();
}

#endif /*(GFX_COLOR_MODE == GFX_COLOR_MODE_RGB565)*/

pix_t I_BlendPixMap (pix_t fg, pix_t bg)
{
    byte *map = aclut_map + (aclut[bg] * clut_num_entries);
    return map[aclut[fg]];
}

void I_SetPlayPal (void)
{
    if (!rgb_palette) {
        fatal_error("");
    }
    prev_clut = p_palette;
    p_palette = rgb_palette;
}

void I_RestorePal (void)
{
    if (!prev_clut) {
        fatal_error("");
    }
    p_palette = prev_clut;
}

void I_SetPalette (byte* palette, int idx)
{
    unsigned int i;
    rgb_t* color;
    pal_t *pal;

    if (idx > arrlen(palettes)) {
        fatal_error("");
    }
    if (palettes[idx]) {
        p_palette = palettes[idx];
        goto sw_done;
    }
    pal = Z_Malloc(clut_num_bytes, PU_STATIC, 0);
    palettes[idx] = pal;
    p_palette = pal;

    if (idx == 0) {
        rgb_palette = pal;
    }
    for (i = 0; i < clut_num_entries; i++)
    {
        color = (rgb_t*)palette;
        pal[i] = GFX_RGB(gammatable[usegamma][color->r],
                        gammatable[usegamma][color->g],
                        gammatable[usegamma][color->b],
                        GFX_OPAQUE);
        palette += 3;
    }
sw_done:
#if (GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)
    screen_sync(1);
    screen_set_clut(p_palette, clut_num_entries);
#endif
    //I_CacheAclut();
    return;
}

static void I_RefreshPalette (int pal_idx)
{
    pal_t *pal;
    int i;
    byte r, g, b;
    if (pal_idx >= arrlen(palettes)) {
        fatal_error("");
    }
    pal = palettes[pal_idx];

    if (pal == rgb_palette) {
        fatal_error("");
    }

    for (i = 0; i < clut_num_entries; i++) {
        r = GFX_ARGB_R(pal[i]);
        g = GFX_ARGB_G(pal[i]);
        b = GFX_ARGB_B(pal[i]);

        pal[i] = GFX_RGB(GFX_OPAQUE,
                        gammatable[usegamma][r],
                        gammatable[usegamma][g],
                        gammatable[usegamma][b]);
    }
}


void I_RefreshClutsButPlaypal (void)
{
    int i;
    for (i = 1; i < arrlen(palettes); i++) {
        if (palettes[i]) {
            I_RefreshPalette(i);
        }
    }
}

#if (GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT)

static int _I_GetClutIndex (pal_t *pal, pix_t pix)
{
    int i = 0;
    for (i = 0; i < clut_num_entries; ++i) {
        if (pal[i] == pix) {
            return i;
        }
    }
    return -1;
}

int I_GetClutIndex (pix_t pix)
{
    int i, pal_idx = -1;
    if (!rgb_palette || !p_palette) {
        fatal_error("");
    }
    for (i = 0; i < arrlen(palettes); i++) {
        if (palettes[i]) {
            pal_idx = _I_GetClutIndex(palettes[i], pix);
            if (pal_idx >= 0) {
                return pal_idx;
            }
        }
    }
    pal_idx = I_GetPaletteIndex(GFX_ARGB_R(pix), GFX_ARGB_G(pix), GFX_ARGB_B(pix));
    return pal_idx;
}

#endif /*(GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)*/

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex (int r, int g, int b)
{
    int best, best_diff, diff;
    int i;
    rgb_t color;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < clut_num_entries; ++i)
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


float mouse_acceleration = 1.0f;
int usemouse = 0;
int mouse_threshold;

const kbdmap_t gamepad_to_kbd_map[JOY_STD_MAX] =
{
    [JOY_UPARROW]       = {KEY_UPARROW, 0},
    [JOY_DOWNARROW]     = {KEY_DOWNARROW, 0},
    [JOY_LEFTARROW]     = {KEY_LEFTARROW ,0},
    [JOY_RIGHTARROW]    = {KEY_RIGHTARROW, 0},
    [JOY_K1]            = {KEY_USE, PAD_FREQ_LOW},
    [JOY_K4]            = {KEY_END,  0},
    [JOY_K3]            = {KEY_FIRE, 0},
    [JOY_K2]            = {KEY_TAB,    PAD_FREQ_LOW},
    [JOY_K5]            = {KEY_STRAFE_L,    0},
    [JOY_K6]            = {KEY_STRAFE_R,    0},
    [JOY_K7]            = {KEY_DEL,  0},
    [JOY_K8]            = {KEY_PGDN, 0},
    [JOY_K9]            = {KEY_ENTER, 0},
    [JOY_K10]           = {KEY_ESCAPE, PAD_FREQ_LOW},
};

void input_post_key (i_event_t e)
{
    event_t event =
        {
            e.state == keyup ? ev_keyup : ev_keydown,
            e.sym, -1, -1, -1
        };
    D_PostEvent(&event);
}

void I_GetEvent (void)
{
    input_proc_keys();
}

void I_InitGraphics (void)
{
    screen_t screen;
#if !IVID_IRAM
    I_VideoBuffer = (pix_t*)Z_Malloc (D_SCREEN_BYTE_CNT, PU_STATIC, NULL);
#else
    I_VideoBuffer = I_VideoBuffer_static;
#endif
	screenvisible = true;
    p_palette = rgb_palette;
    screen.buf = NULL;
    screen.width = SCREENWIDTH;
    screen.height = SCREENHEIGHT;
    screen_win_cfg(&screen);

    input_soft_init(gamepad_to_kbd_map);
    input_bind_extra(K_EX_LOOKUP, KEY_HOME);
    input_bind_extra(K_EX_LOOKUP, KEY_DEL);
    input_bind_extra(K_EX_LOOKUP, KEY_INS);
}

void I_ShutdownGraphics (void)
{
#if !IVID_IRAM
	Z_Free (I_VideoBuffer);
#endif
    I_VideoBuffer = NULL;
}

