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
//	All the clipping: columns, horizontal spans, sky columns.
//






#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"
#include "st_stuff.h"


// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
extern boolean		segtextured;	

// False if the back side is the same plane.
extern boolean		markfloor;	
extern boolean		markceiling;

extern boolean		maskedtexture;
extern int		toptexture;
extern int		bottomtexture;
extern int		midtexture;


extern angle_t		rw_normalangle;
// angle to line origin
extern int		rw_angle1;	

//
// regular wall
//
extern int		rw_x;
extern int		rw_stopx;
extern angle_t		rw_centerangle;
extern fixed_t		rw_offset;
extern fixed_t		rw_distance;
extern fixed_t		rw_scale;
extern fixed_t		rw_scalestep;
extern fixed_t		rw_midtexturemid;
extern fixed_t		rw_toptexturemid;
extern fixed_t		rw_bottomtexturemid;

extern int		worldtop;
extern int		worldbottom;
extern int		worldhigh;
extern int		worldlow;

extern fixed_t		pixhigh;
extern fixed_t		pixlow;
extern fixed_t		pixhighstep;
extern fixed_t		pixlowstep;

extern fixed_t		topfrac;
extern fixed_t		topstep;

extern fixed_t		bottomfrac;
extern fixed_t		bottomstep;

extern short*		maskedtexturecol;

extern lighttable_t**	walllights;

extern boolean render_on_distance;
//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned	index;
    column_t*	col;
    int		lightnum;
    int		texnum;
    
    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    texnum = texturetranslation[curline->sidedef->midtexture];
	
    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight + 1;

    if (curline->v1->y == curline->v2->y)
	lightnum--;
    else if (curline->v1->x == curline->v2->x)
	lightnum++;

    if (lightnum < 0)		
	walllights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	walllights = scalelight[LIGHTLEVELS-1];
    else
	walllights = scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->scalestep;		
    spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
    mfloorclip = ds->sprbottomclip;
    mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	dc_texturemid = frontsector->floorheight > backsector->floorheight
	    ? frontsector->floorheight : backsector->floorheight;
	dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
    else
    {
	dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
	    ? frontsector->ceilingheight : backsector->ceilingheight;
	dc_texturemid = dc_texturemid - viewz;
    }
    dc_texturemid += curline->sidedef->rowoffset;
			
    if (fixedcolormap)
	dc_colormap = fixedcolormap;
    
    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
	// calculate lighting
	if (maskedtexturecol[dc_x] != SHRT_MAX)
	{
	    if (!fixedcolormap)
	    {
		index = spryscale>>LIGHTSCALESHIFT;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		dc_colormap = walllights[index];
	    }
			
	    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
	    dc_iscale = 0xffffffffu / (unsigned)spryscale;
	    
	    // draw the texture
	    col = (column_t *)( 
		(byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) -3);
			
	    R_DrawMaskedColumn (col);
	    maskedtexturecol[dc_x] = SHRT_MAX;
	}
	spryscale += rw_scalestep;
    }
	
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS		12
#define HEIGHTUNIT		(1<<HEIGHTBITS)

extern pix_t*		ylookup[MAXHEIGHT];
extern int		columnofs[MAXWIDTH];
extern int plyr_wpflash_light;

int rw_scale_var = 0;

void (*render_col) (void);



static fixed_t rw_scalestep_lut[R_RANGE_MAX];
static fixed_t topstep_lut[R_RANGE_MAX];
static fixed_t bottomstep_lut[R_RANGE_MAX];
static fixed_t pixlowstep_lut[R_RANGE_MAX];
static fixed_t pixhighstep_lut[R_RANGE_MAX];
static byte dscale_lut[R_RANGE_MAX];

static void R_SetStepLut (void)
{
    int i;
    byte shift, downscale;
    for (i = 0; i < R_RANGE_MAX; i++) {

        shift = rw_render_downscale[i].shift;
        downscale = (1 << shift) - 1;

        dscale_lut[i]       = downscale;
        rw_scalestep_lut[i] = rw_scalestep * downscale;
        topstep_lut[i]      = topstep * downscale;
        bottomstep_lut[i]   = bottomstep * downscale;
        pixlowstep_lut[i]   = pixlowstep * downscale;
        pixhighstep_lut[i]  = pixhighstep * downscale;
    }
}
static void R_CopySegClip (
    int dest,
    int src)
{
    floorclip[dest] = floorclip[src];
    ceilingclip[dest] = ceilingclip[src];

    if (markfloor) {
        floorplane->top[dest] = floorplane->top[src];
        floorplane->bottom[dest] = floorplane->bottom[src];
    }
    if (markceiling) {
        ceilingplane->top[dest] = ceilingplane->top[src];
        ceilingplane->bottom[dest] = ceilingplane->bottom[src];
    }
    if (maskedtexture) {
        maskedtexturecol[dest] = maskedtexturecol[src];
    }
}

static void R_CopySegRange (void)
{
    byte downscale = dscale_lut[rw_render_range];

    if (downscale && render_on_distance && segtextured) {

        rw_scale    += rw_scalestep_lut[rw_render_range];
        topfrac     += topstep_lut[rw_render_range];
        bottomfrac  += bottomstep_lut[rw_render_range];
        pixlow      += pixlowstep_lut[rw_render_range];
        pixhigh     += pixhighstep_lut[rw_render_range];
        while (downscale--) {
            rw_x++;
            R_CopySegClip(rw_x, rw_x - 1);
        }
    }
}

void R_RenderSegLoop (void)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    fixed_t		texturecolumn;
    int			top;
    int			bottom;
    fixed_t     hyp = 0;

    rw_render_range = R_RANGE_NEAREST;
    R_SetStepLut();

    if (frontsector) {
        ST_StartLight(-1, 1, frontsector->extrlight, LT_SECT);
    } else if (backsector) {
        ST_StartLight(-1, 1, backsector->extrlight, LT_SECT);
    }

    if (plyr_wpflash_light) {
        ST_StartLight(rw_distance, 2, plyr_wpflash_light, LT_WPN);
    }

    for ( ; rw_x < rw_stopx ; rw_x++) {
        short temp_ceil = ceilingclip[rw_x]+1;
        short temp_floor = floorclip[rw_x]-1;

        // mark floor / ceiling areas
        yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;
        // no space above wall?
        if (yl < temp_ceil)
            yl = temp_ceil;

        if (markceiling) {
            top = temp_ceil;
            bottom = yl-1;

            if (bottom >= temp_floor + 1)
                bottom = temp_floor;

            if (top <= bottom) {
                ceilingplane->top[rw_x] = top;
                ceilingplane->bottom[rw_x] = bottom;
            }
        }
        yh = bottomfrac>>HEIGHTBITS;

        if (yh >= temp_floor + 1)
            yh = temp_floor;

        if (markfloor) {
            top = yh+1;
            bottom = temp_floor;
            if (top <= temp_ceil - 1)
                top = temp_ceil;
            if (top <= bottom)
            {
                floorplane->top[rw_x] = top;
                floorplane->bottom[rw_x] = bottom;
            }
        }
        // texturecolumn and lighting are independent of wall tiers

        // calculate texture offset
        angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
        hyp = FixedMul(finesine_n[angle], rw_distance);

        R_SetRwRange(hyp);
        R_ProcDownscale(rw_x, rw_stopx);

        if (segtextured)
        {
            texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);

            if (rw_render_range == R_RANGE_MID && bottomtexture) {
                if (rw_x < rw_stopx - 4) {
                    rw_render_range = R_RANGE_MID;
                }
            }
#ifdef NOT_YET
            if (0 && frontsector) {
                mobj_t *t = frontsector->thinglist;
                fixed_t m_dist;
                while (t)
                {
                    if (t->flags2 & MOBJ_LIGHT_SRC_BM && t->data) {
                        m_dist = P_AproxDistance
                                    (curline->v1->x - t->x,
                                     curline->v1->y - t->y);
                        if (m_dist < R_DISTANCE_MID) {
                            extra = true;
                            break;
                        }
                    }
                    t = t->snext;
                }
            }
#endif
            ST_StartLight(hyp, 0, -1, LT_FOG);

            texturecolumn >>= FRACBITS;
            // calculate lighting
            index = rw_scale>>LIGHTSCALESHIFT;

            if (index >=  MAXLIGHTSCALE )
                index = MAXLIGHTSCALE-1;

            dc_colormap = walllights[index];
            dc_x = rw_x;
            dc_iscale = 0xffffffffu / (unsigned)rw_scale;
        }
        else
        {
            // purely to shut up the compiler

            texturecolumn = 0;
        }
        // draw the wall tiers
        if (midtexture)
        {
            // single sided line
            dc_yl = yl;
            dc_yh = yh;
            dc_texturemid = rw_midtexturemid;
            dc_source = R_GetColumn(midtexture,texturecolumn);
            colfunc ();
            ceilingclip[rw_x] = viewheight;
            floorclip[rw_x] = -1;
        } else {
            // two sided line
            if (toptexture) {
                // top wall
                mid = pixhigh>>HEIGHTBITS;
                pixhigh += pixhighstep;

                if (mid >= temp_floor + 1)
                    mid = temp_floor;

                if (mid >= yl)
                {
                    dc_yl = yl;
                    dc_yh = mid;
                    dc_texturemid = rw_toptexturemid;
                    dc_source = R_GetColumn(toptexture,texturecolumn);
                    colfunc ();
                    ceilingclip[rw_x] = mid;
                }
                else
                    ceilingclip[rw_x] = yl-1;
            } else {
                // no top wall
                if (markceiling)
                    ceilingclip[rw_x] = yl-1;
            }
            if (bottomtexture) {
                // bottom wall
                mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
                pixlow += pixlowstep;

                // no space above wall?
                if (mid <= ceilingclip[rw_x])
                    mid = ceilingclip[rw_x]+1;

                if (mid <= yh)
                {
                    dc_yl = mid;
                    dc_yh = yh;
                    dc_texturemid = rw_bottomtexturemid;
                    dc_source = R_GetColumn(bottomtexture,
                                texturecolumn);
                    colfunc ();
                    floorclip[rw_x] = mid;
                }
                else
                    floorclip[rw_x] = yh+1;
            } else {
                // no bottom wall
                if (markfloor)
                    floorclip[rw_x] = yh+1;
            }
            if (maskedtexture) {
                // save texturecol
                //  for backdrawing of masked mid texture
                maskedtexturecol[rw_x] = texturecolumn;
            }
        }
        R_CopySegRange();
        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;
    }
}


extern visplane_t*		lastvisplane;
extern visplane_t*		floorplane;
extern visplane_t*		ceilingplane;

static inline visplane_t*
R_CheckPlane
( visplane_t*	pl,
  int		start,
  int		stop )
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;
	
    if (start < pl->minx)
    {
	intrl = pl->minx;
	unionl = start;
    }
    else
    {
	unionl = pl->minx;
	intrl = start;
    }
	
    if (stop > pl->maxx)
    {
	intrh = pl->maxx;
	unionh = stop;
    }
    else
    {
	unionh = pl->maxx;
	intrh = stop;
    }

    for (x=intrl ; x<= intrh ; x++)
	if (pl->top[x] != 0xff)
	    break;

    if (x > intrh)
    {
	pl->minx = unionl;
	pl->maxx = unionh;

	// use the same one
	return pl;		
    }
	
    // make a new visplane
    lastvisplane->height = pl->height;
    lastvisplane->picnum = pl->picnum;
    lastvisplane->lightlevel = pl->lightlevel;
    
    pl = lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    memset (pl->top,0xff,sizeof(pl->top));
		
    return pl;
}


extern int st_palette;
//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( int	start,
  int	stop )
{
    fixed_t		hyp;
    fixed_t		sineval;
    angle_t		distangle, offsetangle;
    fixed_t		vtop;
    int			lightnum;

    // don't overflow and crash
    if (ds_p == &drawsegs[MAXDRAWSEGS])
	return;		
		
#ifdef RANGECHECK
    if (start >=viewwidth || start > stop)
	I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    sidedef = curline->sidedef;
    linedef = curline->linedef;

    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;
    
    // calculate rw_distance for scale calculation
    rw_normalangle = curline->angle + ANG90;
    offsetangle = abs((int)rw_normalangle-(int)rw_angle1);
    
    if (offsetangle > ANG90)
	offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    rw_distance = FixedMul (hyp, sineval);//
    project_rw_dist = FixedDiv(projection, rw_distance);
		
	
    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;
    
    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale = 
	R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);
    
    if (stop > start )
    {
	ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
	ds_p->scalestep = rw_scalestep = 
	    (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
	ds_p->scale2 = ds_p->scale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;
	
    midtexture = toptexture = bottomtexture = maskedtexture = 0;
    ds_p->maskedtexturecol = NULL;
	
    if (!backsector)
    {
	// single sided line
	midtexture = texturetranslation[sidedef->midtexture];
	// a single sided line is terminal, so it must mark ends
	markfloor = markceiling = true;
	if (linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = frontsector->floorheight +
		textureheight[sidedef->midtexture];
	    // bottom of texture at bottom
	    rw_midtexturemid = vtop - viewz;	
	}
	else
	{
	    // top of texture at top
	    rw_midtexturemid = worldtop;
	}
	rw_midtexturemid += sidedef->rowoffset;

	ds_p->silhouette = SIL_BOTH;
	ds_p->sprtopclip = screenheightarray;
	ds_p->sprbottomclip = negonearray;
	ds_p->bsilheight = INT_MAX;
	ds_p->tsilheight = INT_MIN;
    }
    else
    {
	// two sided line
	ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
	ds_p->silhouette = 0;
	
	if (frontsector->floorheight > backsector->floorheight)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = frontsector->floorheight;
	}
	else if (backsector->floorheight > viewz)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = INT_MAX;
	    // ds_p->sprbottomclip = negonearray;
	}
	
	if (frontsector->ceilingheight < backsector->ceilingheight)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = frontsector->ceilingheight;
	}
	else if (backsector->ceilingheight < viewz)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = INT_MIN;
	    // ds_p->sprtopclip = screenheightarray;
	}
		
	if (backsector->ceilingheight <= frontsector->floorheight)
	{
	    ds_p->sprbottomclip = negonearray;
	    ds_p->bsilheight = INT_MAX;
	    ds_p->silhouette |= SIL_BOTTOM;
	}
	
	if (backsector->floorheight >= frontsector->ceilingheight)
	{
	    ds_p->sprtopclip = screenheightarray;
	    ds_p->tsilheight = INT_MIN;
	    ds_p->silhouette |= SIL_TOP;
	}
	
	worldhigh = backsector->ceilingheight - viewz;
	worldlow = backsector->floorheight - viewz;
		
	// hack to allow height changes in outdoor areas
	if (frontsector->ceilingpic == skyflatnum 
	    && backsector->ceilingpic == skyflatnum)
	{
	    worldtop = worldhigh;
	} else if (worldhigh < worldtop)
	{
	    // top texture
	    toptexture = texturetranslation[sidedef->toptexture];
	    if (linedef->flags & ML_DONTPEGTOP)
	    {
		// top of texture at top
		rw_toptexturemid = worldtop;
	    }
	    else
	    {
		vtop =
		    backsector->ceilingheight
		    + textureheight[sidedef->toptexture];
		
		// bottom of texture
		rw_toptexturemid = vtop - viewz;	
	    }
	}
	
			
	if (worldlow != worldbottom 
	    || backsector->floorpic != frontsector->floorpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    markfloor = true;
	}
	else
	{
	    // same plane on both sides
	    markfloor = false;
	}
	
			
	if (worldhigh != worldtop 
	    || backsector->ceilingpic != frontsector->ceilingpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    markceiling = true;
	}
	else
	{
	    // same plane on both sides
	    markceiling = false;
	}
	
	if (backsector->ceilingheight <= frontsector->floorheight
	    || backsector->floorheight >= frontsector->ceilingheight)
	{
	    // closed door
	    markceiling = markfloor = true;
	}
	
	if (worldlow > worldbottom)
	{
	    // bottom texture
	    bottomtexture = texturetranslation[sidedef->bottomtexture];

	    if (linedef->flags & ML_DONTPEGBOTTOM )
	    {
		// bottom of texture at bottom
		// top of texture at top
		rw_bottomtexturemid = worldtop;
	    }
	    else	// top of texture at top
		rw_bottomtexturemid = worldlow;
	}
	rw_toptexturemid += sidedef->rowoffset;
	rw_bottomtexturemid += sidedef->rowoffset;
	
	// allocate space for masked texture tables
	if (sidedef->midtexture)
	{
	    // masked midtexture
	    maskedtexture = true;
	    ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
	    lastopening += rw_stopx - rw_x;
	}
    }
    
    // calculate rw_offset (only needed for textured lines)
    segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

    if (segtextured)
    {
	offsetangle = rw_normalangle-rw_angle1;
	
	if (offsetangle > ANG180)
	    offsetangle = -offsetangle;

	if (offsetangle > ANG90)
	    offsetangle = ANG90;

	sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
	rw_offset = FixedMul (hyp, sineval);

	if (rw_normalangle-rw_angle1 < ANG180)
	    rw_offset = -rw_offset;

	rw_offset += sidedef->textureoffset + curline->offset;
	rw_centerangle = ANG90 + viewangle - rw_normalangle;
	
	// calculate light table
	//  use different light tables
	//  for horizontal / vertical / diagonal
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	if (!fixedcolormap)
	{
	    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight + 1;

	    if (curline->v1->y == curline->v2->y)
		lightnum--;
	    else if (curline->v1->x == curline->v2->x)
		lightnum++;

	    if (lightnum < 0)		
		walllights = scalelight[0];
	    else if (lightnum >= LIGHTLEVELS)
		walllights = scalelight[LIGHTLEVELS-1];
	    else
		walllights = scalelight[lightnum];
	}
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
  
    if (frontsector->floorheight >= viewz)
    {
	// above view plane
	markfloor = false;
    }
    
    if (frontsector->ceilingheight <= viewz 
	&& frontsector->ceilingpic != skyflatnum)
    {
	// below view plane
	markceiling = false;
    }

    
    // calculate incremental stepping values for texture edges
    worldtop >>= 4;
    worldbottom >>= 4;
	
    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);
	
    if (backsector)
    {	
	worldhigh >>= 4;
	worldlow >>= 4;

	if (worldhigh < worldtop)
	{
	    pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
	    pixhighstep = -FixedMul (rw_scalestep,worldhigh);
	}
	
	if (worldlow > worldbottom)
	{
	    pixlow = (centeryfrac>>4) - FixedMul (worldlow, rw_scale);
	    pixlowstep = -FixedMul (rw_scalestep,worldlow);
	}
    }
    
    // render it
    if (markceiling)
	ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
    
    if (markfloor)
	floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
    R_RenderSegLoop ();

    
    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
	 && !ds_p->sprtopclip)
    {
	d_memcpy (lastopening, ceilingclip+start, sizeof(*lastopening)*(rw_stopx-start));
	ds_p->sprtopclip = lastopening - start;
	lastopening += rw_stopx - start;
    }
    
    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
	 && !ds_p->sprbottomclip)
    {
	d_memcpy (lastopening, floorclip+start, sizeof(*lastopening)*(rw_stopx-start));
	ds_p->sprbottomclip = lastopening - start;
	lastopening += rw_stopx - start;	
    }

    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
	ds_p->silhouette |= SIL_TOP;
	ds_p->tsilheight = INT_MIN;
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
	ds_p->silhouette |= SIL_BOTTOM;
	ds_p->bsilheight = INT_MAX;
    }
    ds_p++;
    ST_StopLight();
}

