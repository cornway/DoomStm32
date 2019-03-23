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
//	System specific interface stuff.
//

// ?
#define MAXWIDTH			1120
#define MAXHEIGHT			832


#ifndef __R_DRAW__
#define __R_DRAW__




extern lighttable_t*	dc_colormap;
extern int		dc_x;
extern int		dc_yl;
extern int		dc_yh;
extern fixed_t		dc_iscale;
extern fixed_t		dc_texturemid;

// first pixel in a column
extern byte*		dc_source;		


// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.
void 	R_DrawColumn (void);
void 	R_DrawColumnLow (void);

// The Spectre/Invisibility effect.
void 	R_DrawFuzzColumn (void);
void 	R_DrawFuzzColumnLow (void);

// Draw with color translation tables,
//  for player sprite rendering,
//  Green/Red/Blue/Indigo shirts.
void	R_DrawTranslatedColumn (void);
void	R_DrawTranslatedColumnLow (void);

void R_CopyColumn (int dest, int src);


void
R_VideoErase
( unsigned	ofs,
  int		count );

extern int		ds_y;
extern int		ds_x1;
extern int		ds_x2;

extern lighttable_t*	ds_colormap;

extern fixed_t		ds_xfrac;
extern fixed_t		ds_yfrac;
extern fixed_t		ds_xstep;
extern fixed_t		ds_ystep;

// start of a 64*64 tile image
extern byte*		ds_source;		

extern byte*		translationtables;
extern byte*		dc_translation;


// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void 	R_DrawSpan (void);

// Low resolution mode, 160x200?
void 	R_DrawSpanLow (void);

int
R_ProcDownscale (int start, int stop);

void
R_InitBuffer
( int		width,
  int		height );


// Initialize color translation tables,
//  for player rendering etc.
void	R_InitTranslationTables (void);



// Rendering function.
void R_FillBackScreen (void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);

#define R_DISTANCE_NEAR (MELEERANGE * 4)
#define R_DISTANCE_MID  (MELEERANGE * 8)
#define R_DISTANCE_FAR  (MELEERANGE * 16)
#define R_DISTANCE_INVIS (MELEERANGE * 32)


typedef enum {
    R_RANGE_NEAREST = 0,
    R_RANGE_NEAR,
    R_RANGE_MID,
    R_RANGE_FAR,
    R_RANGE_INVIS,
    R_RANGE_MAX,
} rw_render_range_t;

typedef struct {
    byte shift;
    rw_render_range_t next;
} rw_range_attr;

void R_SetRwRange (fixed_t distance);

extern rw_range_attr rw_render_downscale[R_RANGE_MAX];
extern rw_render_range_t rw_render_range;

#endif
