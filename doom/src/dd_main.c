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
#include "button.h"
#include "debug.h"
#include "ff.h"
#include "i2c.h"
#include "jpeg.h"
#include "led.h"
#include "main.h"
#include "sdram.h"
#include "spi.h"
#include "images.h"
#include "touch.h"
#include "doom.h"
#include "d_main.h"
#include "d_mode.h"
#include "w_wad.h"
#include "info.h"
#include "p_mobj.h"
#include "v_video.h"
#include "z_zone.h"

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int d_main(void)
{
    // show title as soon as possible
    //show_image (img_loading);
    touch_init ();
    button_init ();
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


/*
 * Show fatal error message and stop in endless loop
 */
void fatal_error (const char* message)
{
	while (1)
	{
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
    }
}

void DD_UpdateNoBlit (void)
{
    if (alt_gameaction == ga_cachelevel) {
        patch_t *ld = W_CacheLumpName("LOADING", PU_CACHE);
        V_DrawPatchC(ld, 0);
    }
}
/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
