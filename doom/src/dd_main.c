/*
 * main.c
 *
 *  Created on: 13.05.2014
 *      Author: Florian
 */

/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "main.h"
#include "touch.h"
#include "doom.h"
#include "d_main.h"
#include "d_mode.h"
#include "w_wad.h"
#include "info.h"
#include "p_mobj.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int d_main(void)
{
    D_DoomMain ();
    return 0;
}

uint32_t sec_time = 0;
uint32_t frames_count;
uint32_t fps_prev;
uint32_t msec_per_frame;
uint32_t msec_per_frame_start;
#define MS_PER_SEC 1000
extern uint32_t systime;
void fps_update (void)
{
    if (systime - sec_time * MS_PER_SEC >= MS_PER_SEC) {
        sec_time++;
        fps_prev = frames_count;
        frames_count = 0;
    } else {
        frames_count++;
    }
}

void frame_start ()
{
    msec_per_frame_start = systime;
}

void frame_end ()
{
    msec_per_frame = systime - msec_per_frame_start;
}

extern gamestate_t gamestate;


typedef enum {
    anim_none,
    anim_start,
    anim_proc,
    anim_end,
} anim_state_t;

typedef struct {
    int start;
    int end;
    int frame;
    int elapse;
    int delay;
    anim_state_t state;
} anim_t;

static anim_t fire_anim = {-1, -1, -1, -1, -1, anim_none};

static void DD_LoadAnim (anim_t *anim, char *start, char *end, int delay)
{
    anim->start = W_CheckNumForName(start);
    anim->end = W_CheckNumForName(end);
    anim->frame = anim->start;
    anim->state = anim_proc;
    anim->delay = delay;
    anim->elapse = delay;
}

static void DD_ProcAnim (anim_t *anim)
{
    switch (anim->state) {
        case anim_none:
            break;
        case anim_start:
            break;
        case anim_proc:
            if (anim->elapse> 0) {
                anim->elapse--;
                break;
            }
            anim->elapse = anim->delay;
            anim->frame++;
            if (anim->frame > anim->end) {
                anim->state = anim_end;
            }
            break;
        case anim_end:
            anim->frame = -1;
            anim->state = anim_none;
            break;
    }
}

static void DD_DrawAnimTileW (anim_t *anim)
{
    int x, y, w, h, f_num, i;
    if (anim->frame < 0)
        return;

    patch_t *patch = W_CacheLumpNum(anim->frame, PU_CACHE);

    x = 0;
    w = READ_LE_U16(patch->width);
    h = READ_LE_U16(patch->height);
    f_num = SCREENWIDTH / w;
    y = SCREENHEIGHT - h;

    for (i = 0; i < f_num; i++) {
        V_DrawPatch(x, y, patch);
        x += w;
    }
}



static gameaction_t alt_gameaction = ga_nothing;

void DD_SetGameAct (gameaction_t action)
{
    alt_gameaction = action;
}

void DD_ProcGameAct (void)
{
    alt_gameaction = ga_nothing;
}

void DD_LoadAltPkgGame (void)
{
    if (game_alt_pkg == pkg_psx_final) {
        mobjinfo_t *info = &mobjinfo[MT_SHADOWS];

        info->spawnstate    = S_SPEC_STND;
        info->spawnhealth   = 240;
        info->seestate      = S_SPEC_RUN1;
        info->painstate     = S_SPEC_PAIN;
        info->meleestate    = S_SPEC_ATK1;
        info->deathstate    = S_SPEC_DIE1;
        info->flags         &= ~MF_SHADOW;
        info->raisestate    = S_SPEC_RAISE1;

        DD_LoadAnim(&fire_anim, "FIRE001", "FIRE317", -1);
    }
}

void DD_UpdateNoBlit (void)
{
    if (alt_gameaction == ga_cachelevel) {
        patch_t *ld = W_CacheLumpName("LOADING", PU_CACHE);
        V_DrawPatchC(ld, 0);
        goto done;
    }
    if (gamestate == GS_DEMOSCREEN) {
        DD_ProcAnim(&fire_anim);
        DD_DrawAnimTileW(&fire_anim);
    }
done:
    return;
}
/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
