//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//




#include "doomdef.h"
#include "deh_main.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"
#include "p_local.h"
#include <misc_utils.h>

// status bar height at bottom of screen
#define SBARHEIGHT		32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//
extern int render_on_distance;

extern byte*		viewimage; 
extern int		viewwidth;
extern int		scaledviewwidth;
extern int		viewheight;
extern int		viewwindowx;
extern int		viewwindowy; 
extern pix_t*		ylookup[MAXHEIGHT]; 
extern int		columnofs[MAXWIDTH]; 

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
extern byte		translations[3][256];	
 
// Backing buffer containing the bezel drawn around the screen and 
// surrounding background.

extern pix_t *background_buffer = NULL;


//
// R_DrawColumn
// Source is the top of the column to scale.
//
extern lighttable_t*		dc_colormap; 
extern int			dc_x; 
extern int			dc_yl; 
extern int			dc_yh; 
extern fixed_t			dc_iscale; 
extern fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual) 
extern byte*			dc_source;		

// just for profiling 
extern int			dccount;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
// 

rw_range_attr rw_render_downscale[R_RANGE_MAX] =
{
    [R_RANGE_NEAREST]   = {0, R_RANGE_NEAREST},
    [R_RANGE_NEAR]      = {1, R_RANGE_NEAREST},
    [R_RANGE_MID]       = {2, R_RANGE_NEAR},
    [R_RANGE_FAR]       = {3, R_RANGE_MID},
    [R_RANGE_INVIS]     = {3, R_RANGE_MID},
};

void R_SetRwRange (fixed_t distance)
{
    if (distance > 0) {
        if (distance > R_DISTANCE_INVIS) {
            rw_render_range = R_RANGE_INVIS;
        } else if (distance > R_DISTANCE_FAR) {
            rw_render_range = R_RANGE_FAR;
        } else if (distance > R_DISTANCE_MID) {
           rw_render_range = R_RANGE_MID;
        } else if (distance > R_DISTANCE_NEAR) {
           rw_render_range = R_RANGE_NEAR;
        }
    } else if (distance < 0) {
        distance = -distance;
        if (distance > R_DISTANCE_INVIS) {
            rw_render_range = R_RANGE_INVIS;
        }
    }
}

static
void v_set_line2 (pix_t *dest, pix_t c)
{
    size_t i = 0;
    dest[i] = c;
    dest[i + 1] = c;
}

static
void v_set_line4 (pix_t *dest, pix_t c)
{
    size_t i = 0;
    dest[i] = c;
    dest[i + 1] = c;
    dest[i + 2] = c;
    dest[i + 3] = c;
}

static
void v_set_line8 (pix_t *dest, pix_t c)
{
    size_t i = 0;
    dest[i] = c;
    dest[i + 1] = c;
    dest[i + 2] = c;
    dest[i + 3] = c;
    dest[i + 4] = c;
    dest[i + 5] = c;
    dest[i + 6] = c;
    dest[i + 7] = c;
}


typedef void (*set_line_t) (pix_t *, pix_t);

set_line_t set_line_tbl[] =
{
    [R_RANGE_INVIS] = v_set_line8,
    [R_RANGE_FAR] = v_set_line8,
    [R_RANGE_MID] = v_set_line4,
    [R_RANGE_NEAR] = v_set_line2,
    [R_RANGE_NEAREST] = NULL,
};


int
R_ProcDownscale (int start, int stop)
{
    byte downscale = 1;
    rw_render_range_t next;

    if (render_on_distance) {
        downscale = 1 << rw_render_downscale[rw_render_range].shift;
        next = rw_render_downscale[rw_render_range].next;
        while ((start + downscale >= stop) && (downscale > 1)) {
            rw_render_range = next;
            downscale = 1 << rw_render_downscale[rw_render_range].shift;
            next = rw_render_downscale[rw_render_range].next;
        }
    }
    return downscale;
}

static void
R_RenderColVar (
    pix_t *dest,
    float frac,
    float fracstep,
    int count)
{
    pix_t c;
    byte downscale;

    set_line_t vstlinefunc = set_line_tbl[rw_render_range];

    switch (rw_render_range) {
        case R_RANGE_INVIS:
        case R_RANGE_FAR:
            while (count >= 7) {
                c = dc_colormap[dc_source[(int)frac & 0x7f]];
                c = pixel(c);
                vstlinefunc(dest, c);
                vstlinefunc(dest + (SCREENWIDTH * 1), c);
                vstlinefunc(dest + (SCREENWIDTH * 2), c);
                vstlinefunc(dest + (SCREENWIDTH * 3), c);
                vstlinefunc(dest + (SCREENWIDTH * 4), c);
                vstlinefunc(dest + (SCREENWIDTH * 5), c);
                vstlinefunc(dest + (SCREENWIDTH * 6), c);
                vstlinefunc(dest + (SCREENWIDTH * 7), c);
                frac += fracstep * 8;
                dest += SCREENWIDTH * 8;
                count -= 8;
            }
        case R_RANGE_MID:
            while (count >= 3) {
                c = dc_colormap[dc_source[(int)frac & 0x7f]];
                c = pixel(c);
                vstlinefunc(dest, c);
                vstlinefunc(dest + (SCREENWIDTH * 1), c);
                vstlinefunc(dest + (SCREENWIDTH * 2), c);
                vstlinefunc(dest + (SCREENWIDTH * 3), c);
                dest += SCREENWIDTH * 4;
                frac += fracstep * 4;
                count -= 4;
            }
        case R_RANGE_NEAR:
            while (count >= 1) {
                c = dc_colormap[dc_source[(int)frac & 0x7f]];
                c = pixel(c);
                vstlinefunc(dest, c);
                vstlinefunc(dest + (SCREENWIDTH * 1), c);
                dest += SCREENWIDTH * 2;
                frac += fracstep * 2;
                count -= 2;
            }
            break;
        default :
        break;
    }
    downscale = 1 << rw_render_downscale[rw_render_range].shift;
    while (count >= 0) {
        c = pixel(dc_colormap[dc_source[(int)frac & 0x7f]]);
        v_set_line(dest, c, downscale);
        dest += SCREENWIDTH;
        frac += fracstep;
        count--;
    };
}

void R_DrawColumn (void) 
{ 
    int			count; 
    pix_t*		dest; 
    float		frac;
    float		fracstep;

    count = dc_yh - dc_yl; 

    // Zero length, column does not exceed a pixel.
    if (count < 0) 
	return; 
				 
#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT) 
	I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows? 
    dest = ylookup[dc_yl] + columnofs[dc_x];  

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = (float)dc_iscale; 
    frac = (float)(dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep); 

    fracstep /= DOUBLEUNIT;
    frac /= DOUBLEUNIT;
    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.
    if (render_on_distance) {
        R_RenderColVar(dest, frac, fracstep, count);
    } else {
        while (count-- >= 0) {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            *dest = pixel(dc_colormap[dc_source[(int)frac & 0x7f]]);

            dest += SCREENWIDTH;
            frac += fracstep;
        }
    }
} 



// UNUSED.
// Loop unrolled.
#if 0
void R_DrawColumn (void) 
{ 
    int			count; 
    byte*		source;
    byte*		dest;
    byte*		colormap;
    
    unsigned		frac;
    unsigned		fracstep;
    unsigned		fracstep2;
    unsigned		fracstep3;
    unsigned		fracstep4;	 
 
    count = dc_yh - dc_yl + 1; 

    source = dc_source;
    colormap = dc_colormap;		 
    dest = ylookup[dc_yl] + columnofs[dc_x];  
	 
    fracstep = dc_iscale<<9; 
    frac = (dc_texturemid + (dc_yl-centery)*dc_iscale)<<9; 
 
    fracstep2 = fracstep+fracstep;
    fracstep3 = fracstep2+fracstep;
    fracstep4 = fracstep3+fracstep;
	
    while (count >= 8) 
    { 
	dest[0] = colormap[source[frac>>25]]; 
	dest[SCREENWIDTH] = colormap[source[(frac+fracstep)>>25]]; 
	dest[SCREENWIDTH*2] = colormap[source[(frac+fracstep2)>>25]]; 
	dest[SCREENWIDTH*3] = colormap[source[(frac+fracstep3)>>25]];
	
	frac += fracstep4; 

	dest[SCREENWIDTH*4] = colormap[source[frac>>25]]; 
	dest[SCREENWIDTH*5] = colormap[source[(frac+fracstep)>>25]]; 
	dest[SCREENWIDTH*6] = colormap[source[(frac+fracstep2)>>25]]; 
	dest[SCREENWIDTH*7] = colormap[source[(frac+fracstep3)>>25]]; 

	frac += fracstep4; 
	dest += SCREENWIDTH*8; 
	count -= 8;
    } 
	
    while (count > 0)
    { 
	*dest = colormap[source[frac>>25]]; 
	dest += SCREENWIDTH; 
	frac += fracstep; 
	count--;
    } 
}
#endif


void R_DrawColumnLow (void) 
{ 
    int			count; 
    pix_t*		dest; 
    pix_t*		dest2;
    float		frac;
    float		fracstep;	 
    int                 x;
 
    count = dc_yh - dc_yl; 

    // Zero length.
    if (count < 0) 
	return; 
				 
#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	
	I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
    //	dccount++; 
#endif 
    // Blocky mode, need to multiply by 2.
    x = dc_x << 1;
    
    dest = ylookup[dc_yl] + columnofs[x];
    dest2 = ylookup[dc_yl] + columnofs[x+1];
    
    fracstep = (float)dc_iscale; 
    frac = (float)dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep;
    frac = frac / DOUBLEUNIT;
    fracstep = fracstep / DOUBLEUNIT;
    do 
    {
        // Hack. Does not work corretly.
        *dest2 = *dest = pixel(dc_colormap[dc_source[(int)frac & 0x7f]]);
        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;
        frac += fracstep;

    } while (count--);
}


//
// Spectre/Invisibility.
//
#if 1/*(GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)*/
#define FUZZTABLE		50 
#define FUZZOFF	(SCREENWIDTH)


int	fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF 
}; 

int	fuzzpos = 0; 

#endif
//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn (void) 
{ 
    int         count;
    pix_t       *dest;
    float       frac;
    float       fracstep;
#if 1/*(GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)*/
    int         clut_idx;
#endif

    // Adjust borders. Low... 
    if (!dc_yl) 
	dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1)
	dc_yh = viewheight - 2; 
		 
    count = dc_yh - dc_yl; 

    // Zero length.
    if (count < 0) 
	return; 

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);
    }
#endif
    
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = (float)dc_iscale;
    frac = (float)(dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep);

    fracstep /= DOUBLEUNIT;
    frac /= DOUBLEUNIT;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do 
    {
	// Lookup framebuffer, and retrieve
	//  a pixel that is either one column
	//  left or right of the current one.
	// Add index from colormap to index.
        /*FIXME : !!!*/
#if 0/*(GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT)*/

    pix = dc_colormap[dc_source[(int)frac & 0x7f]];
    *dest = I_BlendPix(pixel(pix), *dest, 48);

#else

    clut_idx = I_GetClutIndex(dest[fuzzoffset[fuzzpos]]);
    *dest = pixel(colormaps[6*256+clut_idx]);

    // Clamp table lookup index.
    if (++fuzzpos == FUZZTABLE)
        fuzzpos = 0;
#endif /*(GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT)*/


    dest += SCREENWIDTH;

    frac += fracstep;
    } while (count--); 
} 

// low detail mode version
 
void R_DrawFuzzColumnLow (void) 
{ 
    int         count;
    pix_t       *dest;
    pix_t       *dest2;
    float       frac;
    float       fracstep;
#if 1/*(GFX_COLOR_MODE == GFX_COLOR_MODE_CLUT)*/
    int         clut_idx;
#endif
    int x;

    // Adjust borders. Low... 
    if (!dc_yl) 
	dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1) 
	dc_yh = viewheight - 2; 
		 
    count = dc_yh - dc_yl; 

    // Zero length.
    if (count < 0) 
	return; 

    // low detail mode, need to multiply by 2
    
    x = dc_x << 1;
    
#ifdef RANGECHECK 
    if ((unsigned)x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);
    }
#endif
    
    dest = ylookup[dc_yl] + columnofs[x];
    dest2 = ylookup[dc_yl] + columnofs[x+1];

    // Looks familiar.
    fracstep = (float)dc_iscale;
    frac = (float)(dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep);

    fracstep /= DOUBLEUNIT;
    frac /= DOUBLEUNIT;

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do 
    {
	// Lookup framebuffer, and retrieve
	//  a pixel that is either one column
	//  left or right of the current one.
	// Add index from colormap to index.
#if 0/*(GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT)*/

    pix = dc_colormap[dc_source[(int)frac & 0x7f]];
    *dest = I_BlendPix(pixel(pix), *dest, 48);
    *dest2 = I_BlendPix(pixel(pix), *dest2, 48);

#else

    clut_idx = I_GetClutIndex(dest[fuzzoffset[fuzzpos]]);
    *dest = pixel(colormaps[6*256+clut_idx]);
    clut_idx = I_GetClutIndex(dest2[fuzzoffset[fuzzpos]]);
    *dest2 = pixel(colormaps[6*256+clut_idx]);

    // Clamp table lookup index.
    if (++fuzzpos == FUZZTABLE)
        fuzzpos = 0;

#endif /*(GFX_COLOR_MODE != GFX_COLOR_MODE_CLUT)*/

    dest += SCREENWIDTH;
    dest2 += SCREENWIDTH;

    frac += fracstep;
    } while (count--); 
} 
 
  
  
 

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte*	dc_translation;
byte*	translationtables;

void R_DrawTranslatedColumn (void) 
{ 
    int			count; 
    pix_t*		dest; 
    float		frac;
    float		fracstep;	 
 
    count = dc_yh - dc_yl; 
    if (count < 0) 
	return; 
				 
#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	I_Error ( "R_DrawColumn: %i to %i at %i",
		  dc_yl, dc_yh, dc_x);
    }
    
#endif 


    dest = ylookup[dc_yl] + columnofs[dc_x]; 

    // Looks familiar.
    fracstep = (float)dc_iscale; 
    frac = (float)(dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep); 

    // Here we do an additional index re-mapping.
    do 
    {
        // Translation tables are used
        //  to map certain colorramps to other ones,
        //  used with PLAY sprites.
        // Thus the "green" ramp of the player 0 sprite
        //  is mapped to gray, red, black/indigo. 
        *dest = pixel(dc_colormap[dc_source[(int)frac]]);
        dest += SCREENWIDTH;

        frac += fracstep; 
    } while (count--); 
} 

void R_DrawTranslatedColumnLow (void) 
{ 
    int			count; 
    pix_t*		dest; 
    pix_t*		dest2; 
    float		frac;
    float		fracstep;	 
    int                 x;
 
    count = dc_yh - dc_yl; 
    if (count < 0) 
	return; 

    // low detail, need to scale by 2
    x = dc_x << 1;
				 
#ifdef RANGECHECK 
    if ((unsigned)x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	I_Error ( "R_DrawColumn: %i to %i at %i",
		  dc_yl, dc_yh, x);
    }
    
#endif 


    dest = ylookup[dc_yl] + columnofs[x]; 
    dest2 = ylookup[dc_yl] + columnofs[x+1]; 

    // Looks familiar.
    fracstep = (float)dc_iscale; 
    frac = (float)(dc_texturemid + (dc_yl-centery)*(fixed_t)fracstep); 
    fracstep /= DOUBLEUNIT;
    frac /= DOUBLEUNIT;
    // Here we do an additional index re-mapping.
    do 
    {
        // Translation tables are used
        //  to map certain colorramps to other ones,
        //  used with PLAY sprites.
        // Thus the "green" ramp of the player 0 sprite
        //  is mapped to gray, red, black/indigo. 
        *dest = pixel(dc_colormap[dc_source[(int)frac]]);
        *dest2 = pixel(dc_colormap[dc_source[(int)frac]]);
        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;

        frac += fracstep; 
    } while (count--); 
} 




//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
    int		i;
	
    translationtables = Z_Malloc (256*3, PU_STATIC, 0);
    
    // translate just the 16 green colors
    for (i=0 ; i<256 ; i++)
    {
	if (i >= 0x70 && i<= 0x7f)
	{
	    // map green ramp to gray, brown, red
	    translationtables[i] = 0x60 + (i&0xf);
	    translationtables [i+256] = 0x40 + (i&0xf);
	    translationtables [i+512] = 0x20 + (i&0xf);
	}
	else
	{
	    // Keep all other colors as is.
	    translationtables[i] = translationtables[i+256] 
		= translationtables[i+512] = i;
	}
    }
}




//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int			ds_y; 
int			ds_x1; 
int			ds_x2;

lighttable_t*		ds_colormap; 

fixed_t			ds_xfrac; 
fixed_t			ds_yfrac; 
fixed_t			ds_xstep; 
fixed_t			ds_ystep;

// start of a 64*64 tile image 
byte*			ds_source;	

// just for profiling
int			dscount;


//
// Draws the actual span.
void R_DrawSpan (void) 
{ 
    unsigned int position, step;
    pix_t *dest;
    int count;
    int spot;
    unsigned int xtemp, ytemp;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++;
#endif

    // Pack position and step variables into a single 32-bit integer,
    // with x in the top 16 bits and y in the bottom 16 bits.  For
    // each 16-bit part, the top 6 bits are the integer part and the
    // bottom 10 bits are the fractional part of the pixel position.

    position = ((ds_xfrac << 10) & 0xffff0000)
             | ((ds_yfrac >> 6)  & 0x0000ffff);
    step = ((ds_xstep << 10) & 0xffff0000)
         | ((ds_ystep >> 6)  & 0x0000ffff);

    dest = ylookup[ds_y] + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
	// Calculate current texture index in u,v.
        ytemp = (position >> 4) & 0x0fc0;
        xtemp = (position >> 26);
        spot = xtemp | ytemp;

	// Lookup pixel from flat texture tile,
	//  re-index using light/colormap.
	*dest++ = pixel(ds_colormap[ds_source[spot]]);

        position += step;

    } while (count--);
}



// UNUSED.
// Loop unrolled by 4.
#if 0
void R_DrawSpan (void) 
{ 
    unsigned	position, step;

    byte*	source;
    byte*	colormap;
    byte*	dest;
    
    unsigned	count;
    usingned	spot; 
    unsigned	value;
    unsigned	temp;
    unsigned	xtemp;
    unsigned	ytemp;
		
    position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
    step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
		
    source = ds_source;
    colormap = ds_colormap;
    dest = ylookup[ds_y] + columnofs[ds_x1];	 
    count = ds_x2 - ds_x1 + 1; 
	
    while (count >= 4) 
    { 
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[0] = colormap[source[spot]]; 

	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[1] = colormap[source[spot]];
	
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[2] = colormap[source[spot]];
	
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[3] = colormap[source[spot]]; 
		
	count -= 4;
	dest += 4;
    } 
    while (count > 0) 
    { 
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	*dest++ = colormap[source[spot]]; 
	count--;
    } 
} 
#endif


//
// Again..
//
void R_DrawSpanLow (void)
{
    unsigned int position, step;
    unsigned int xtemp, ytemp;
    pix_t *dest;
    int count;
    int spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++; 
#endif

    position = ((ds_xfrac << 10) & 0xffff0000)
             | ((ds_yfrac >> 6)  & 0x0000ffff);
    step = ((ds_xstep << 10) & 0xffff0000)
         | ((ds_ystep >> 6)  & 0x0000ffff);

    count = (ds_x2 - ds_x1);

    // Blocky mode, need to multiply by 2.
    ds_x1 <<= 1;
    ds_x2 <<= 1;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    do
    {
	// Calculate current texture index in u,v.
        ytemp = (position >> 4) & 0x0fc0;
        xtemp = (position >> 26);
        spot = xtemp | ytemp;

	// Lowres/blocky mode does it twice,
	//  while scale is adjusted appropriately.
	*dest++ = pixel(ds_colormap[ds_source[spot]]);
	*dest++ = pixel(ds_colormap[ds_source[spot]]);

	position += step;

    } while (count--);
}

//
// R_InitBuffer 
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int		width,
  int		height ) 
{ 
    int		i; 

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    viewwindowx = (SCREENWIDTH-width) >> 1; 

    // Column offset. For windows.
    for (i=0 ; i<width ; i++) 
	columnofs[i] = viewwindowx + i;

    // Samw with base row offset.
    if (width == SCREENWIDTH) 
	viewwindowy = 0; 
    else 
	viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1; 

    // Preclaculate all row offsets.
    for (i=0 ; i<height ; i++) 
	ylookup[i] = I_VideoBuffer + (i+viewwindowy)*SCREENWIDTH; 
} 
 
 


//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void) 
{ 
    byte*	src;
    pix_t*	dest; 
    int		x;
    int		y; 
    patch_t*	patch;

    // DOOM border patch.
    char       *name1 = DEH_String("FLOOR7_2");

    // DOOM II border patch.
    char *name2 = DEH_String("GRNROCK");

    char *name;

    // If we are running full screen, there is no need to do any of this,
    // and the background buffer can be freed if it was previously in use.

    if (scaledviewwidth == SCREENWIDTH)
    {
        if (background_buffer != NULL)
        {
            Z_Free(background_buffer);
            background_buffer = NULL;
        }

	return;
    }

    // Allocate the background buffer if necessary
	
    if (background_buffer == NULL)
    {
        background_buffer = Z_Malloc(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT),
                                     PU_STATIC, NULL);
    }

    if (gamemode == commercial)
	name = name2;
    else
	name = name1;
    
    src = W_CacheLumpName(name, PU_CACHE); 
    dest = background_buffer;
	 
    for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++) 
    { 
	for (x=0 ; x<SCREENWIDTH/64 ; x++) 
	{ 
	    d_memcpy (dest, src+((y&63)<<6), 64 * sizeof(pix_t)); 
	    dest += 64; 
	} 

	if (SCREENWIDTH&63) 
	{ 
	    d_memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63 * sizeof(pix_t)); 
	    dest += (SCREENWIDTH&63); 
	} 
    } 
     
    // Draw screen and bezel; this is done to a separate screen buffer.

    V_UseBuffer(background_buffer);

    patch = W_CacheLumpName(DEH_String("brdr_t"),PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
	V_DrawPatch(viewwindowx+x, viewwindowy-8, patch);
    patch = W_CacheLumpName(DEH_String("brdr_b"),PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
	V_DrawPatch(viewwindowx+x, viewwindowy+viewheight, patch);
    patch = W_CacheLumpName(DEH_String("brdr_l"),PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
	V_DrawPatch(viewwindowx-8, viewwindowy+y, patch);
    patch = W_CacheLumpName(DEH_String("brdr_r"),PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
	V_DrawPatch(viewwindowx+scaledviewwidth, viewwindowy+y, patch);

    // Draw beveled edge. 
    V_DrawPatch(viewwindowx-8,
                viewwindowy-8,
                W_CacheLumpName(DEH_String("brdr_tl"),PU_CACHE));
    
    V_DrawPatch(viewwindowx+scaledviewwidth,
                viewwindowy-8,
                W_CacheLumpName(DEH_String("brdr_tr"),PU_CACHE));
    
    V_DrawPatch(viewwindowx-8,
                viewwindowy+viewheight,
                W_CacheLumpName(DEH_String("brdr_bl"),PU_CACHE));
    
    V_DrawPatch(viewwindowx+scaledviewwidth,
                viewwindowy+viewheight,
                W_CacheLumpName(DEH_String("brdr_br"),PU_CACHE));

    V_RestoreBuffer();
} 
 

//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned	ofs,
  int		count ) 
{ 
  // LFB copy.
  // This might not be a good idea if memcpy
  //  is not optiomal, e.g. byte by byte on
  //  a 32bit CPU, as GNU GCC/Linux libc did
  //  at one point.

    if (background_buffer != NULL)
    {
        d_memcpy(I_VideoBuffer + ofs, background_buffer + ofs, count * sizeof(pix_t)); 
    }
} 


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void R_DrawViewBorder (void) 
{ 
    int		top;
    int		side;
    int		ofs;
    int		i; 
 
    if (scaledviewwidth == SCREENWIDTH) 
	return; 
  
    top = ((SCREENHEIGHT-SBARHEIGHT)-viewheight)/2; 
    side = (SCREENWIDTH-scaledviewwidth)/2; 
 
    // copy top and one line of left side 
    R_VideoErase (0, top*SCREENWIDTH+side); 
 
    // copy one line of right side and bottom 
    ofs = (viewheight+top)*SCREENWIDTH-side; 
    R_VideoErase (ofs, top*SCREENWIDTH+side); 
 
    // copy sides using wraparound 
    ofs = top*SCREENWIDTH + SCREENWIDTH-side; 
    side <<= 1;
    
    for (i=1 ; i<viewheight ; i++) 
    { 
	R_VideoErase (ofs, side); 
	ofs += SCREENWIDTH; 
    } 

    // ? 
    V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT); 
} 
 
 
