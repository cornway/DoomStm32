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

#include <debug.h>
#include <misc_utils.h>
#include <dev_io.h>
#include <audio_main.h>
#include <bsp_sys.h>

#include "doom.h"
#include "d_main.h"
#include "d_mode.h"
#include "w_wad.h"
#include "info.h"
#include "p_mobj.h"
#include "v_video.h"
#include "i_video.h"
#include "sounds.h"
#include "z_zone.h"

#define SFX_MAX_NAME 9
#define SFX_MAX_PATH 128

#define DD_CDTRACK_PATH(path, name) \
    DD_GETPATH(path, "music/", game_subdir_ext, "/", name, ".wav")

typedef struct {
    int start;
    int end;
    int frame;
    uint32_t delay;
    uint32_t tsf;
} dd_animation_t;

extern gamestate_t gamestate;

const char *game_dir_path = "doom";
DoomDecorPkg_t game_alt_pkg = pkg_none;

uint32_t sec_time = 0;
uint32_t frames_count;
uint32_t fps_prev;
uint32_t msec_per_frame;
uint32_t msec_per_frame_start;
#define MS_PER_SEC 1000

typedef struct {
    char name[D_MAX_NAME];
} TRACKLIST;

static const TRACKLIST *dd_soundtrack_cfg_list = NULL;
static const char *game_subdir_ext = NULL;

static const char *__DD_GetDecorSubdir (void);
static void __DD_LoadAnimation (dd_animation_t *anim, char *start, char *end, int delay);
static void __DD_TickleAnimation (dd_animation_t *anim);
static void __DD_DrawAnimationTile (dd_animation_t *anim);
static void __DD_UpdateNoBlitPSX (void);
static void __DD_LoadAltPkgPSX (void);
static const TRACKLIST *__DD_SetupSoundtrackListDefault (int *cnt);
static const TRACKLIST *__DD_SetupSoundtrackList (int f, int *cnt);
static gameaction_t alt_gameaction = ga_nothing;
static dd_animation_t fire_anim = {-1, -1, -1, 0, 0};

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int DD_DoomMain(void)
{
    D_DoomMain ();
    return 0;
}

void DD_FpsUpdate (void)
{
    if (d_time() - sec_time * MS_PER_SEC >= MS_PER_SEC) {
        sec_time++;
        fps_prev = frames_count;
        frames_count = 0;
    } else {
        frames_count++;
    }
}

void DD_FrameBegin (void)
{
    msec_per_frame_start = d_time();
}

void DD_FrameEnd (void)
{
    msec_per_frame = d_time() - msec_per_frame_start;
}

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
    if (D_PKG_PSX()) {
        __DD_LoadAltPkgPSX();
    }
}

void DD_UpdateNoBlit (void)
{
    if (D_PKG_PSX()) {
        __DD_UpdateNoBlitPSX();
    }
}

int DD_PlaySoundtrackNum (void *_cd, int num, int volume)
{
    cd_track_t *cd = (cd_track_t *)_cd;
    char path[D_MAX_PATH];

    assert(dd_soundtrack_cfg_list);
    assert(num < NUMMUSIC);
    DD_CDTRACK_PATH(path, dd_soundtrack_cfg_list[num].name);

    cd_play_name(cd, path);
    return cd_volume(cd, volume);
}

void DD_SetupSoundtrackList (int *cnt)
{
    char buf[D_MAX_PATH], *bufptr = NULL, *pathptr = NULL;
    const char **sndlist = NULL;
    int f, i;

    *cnt = 0;
    game_subdir_ext = __DD_GetDecorSubdir();
    if (game_subdir_ext == NULL) {
        return;
    }

    DD_GETPATH(buf, "music/", game_subdir_ext, "/music.cfg");
    pathptr = buf;

    d_open(pathptr, &f, "r");
    if (f < 0) {
        dd_soundtrack_cfg_list = __DD_SetupSoundtrackListDefault(cnt);
    } else {
        dd_soundtrack_cfg_list = __DD_SetupSoundtrackList(f, cnt);
        d_close(f);
    }
}

void DD_PwadAddEach (void (*handle)(void *))
{
    char path[D_MAX_PATH];

    game_subdir_ext = __DD_GetDecorSubdir();
    if (game_subdir_ext) {
        DD_GETPATH(path, game_subdir_ext);
        W_ForEach(path, handle);
    } else {
        W_ForEach(DD_DOOMPATH(), handle);
    }
}
/*---------------------------------------------------------------------*
 *  public functions end                                               *
 *---------------------------------------------------------------------*/




static const char *__DD_GetDecorSubdir (void)
{
    if (game_subdir_ext) {
        return game_subdir_ext;
    }
    switch (game_alt_pkg) {
        case D_DECOR_PSX_FINAL: game_subdir_ext = "psx";
        break;
        case D_DECOR_3DO_DOOM: game_subdir_ext = "3do";
        break;
        default: 
                game_subdir_ext = NULL;
                return NULL;
        /*break;*/
    }
    return game_subdir_ext;

}

static void __DD_UpdateNoBlitPSX (void)
{
    if (alt_gameaction == ga_cachelevel) {
        patch_t *ld = W_CacheLumpName("LOADING", PU_CACHE);
        V_DrawPatchC(ld, 0);
    }
    if (gamestate == GS_DEMOSCREEN) {
        __DD_TickleAnimation(&fire_anim);
        __DD_DrawAnimationTile(&fire_anim);
    }
}

static void __DD_LoadAnimation (dd_animation_t *anim, char *start, char *end, int delay)
{
    anim->start = W_CheckNumForName(start);
    anim->end = W_CheckNumForName(end);
    anim->frame = anim->start;
    anim->delay = delay;
}

static void __DD_DrawAnimationTile (dd_animation_t *anim)
{
    int x, y, w, h, f_num, i;
    if (anim->frame > anim->end)
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

static void __DD_TickleAnimation (dd_animation_t *anim)
{
    if (anim->frame > anim->end) {
        return;
    }
    if (anim->tsf + anim->delay < d_time()) {
        anim->tsf = d_time();
        anim->frame++;
    }
}

static void __DD_LoadAltPkgPSX (void)
{
    mobjinfo_t *info = &mobjinfo[MT_SHADOWS];

    info->spawnstate    = S_SPEC_STND;
    info->spawnhealth   = 240;
    info->seestate      = S_SPEC_RUN1;
    info->painstate     = S_SPEC_PAIN;
    info->meleestate    = S_SPEC_ATK1;
    info->deathstate    = S_SPEC_DIE1;
    info->flags         &= ~MF_SHADOW;
    info->raisestate    = S_SPEC_RAISE1;

    __DD_LoadAnimation(&fire_anim, "FIRE001", "FIRE317", 20);
}


const char *__DD_DoomPath (void)
{
    return game_dir_path;
}

char *__DD_GetPath (char *path, const char **p)
{
    int n = sprintf(path, "%s/", DD_DOOMPATH());

    while (*p) {
        n += sprintf(path + n, "%s", *p);
        p++;
    }
    return path;
}

typedef struct {
    char name[D_MAX_NAME];
    int id;
} sndtrack_map_t;

extern const sndtrack_map_t soundtrack_names[NUMMUSIC];

static int __DD_SoundtrackFindId (const char *name)
{
    int i;
    for (i = 0; i < NUMMUSIC; i++) {
        if (strcmp(name, soundtrack_names[i].name) == 0) {
            return soundtrack_names[i].id;
        }
    }
    return -1;
}

static const char *__DD_SoundtrackFindName (char *cfg, int *id)
{
    const char *keyval[2];
    int n, _id;

    n = d_wstrtok(keyval, arrlen(keyval), cfg);
    if (n < 2) {
        dprintf("%s() : fail to parse \'%s\'\n", __func__, cfg);
        return NULL;
    }
    _id = __DD_SoundtrackFindId(keyval[0]);
    if (_id < 0) {
        return NULL;
    }
    *id = _id;
    return keyval[1];
}

static int __DD_SetupListItem (const TRACKLIST *list, const char *name, int id)
{
    return snprintf((char *)list[id].name, sizeof(list[id].name), "%s", name);
}

static const TRACKLIST *__DD_AllocSoundtrackList (void)
{
    const int bufsize = NUMMUSIC * D_MAX_NAME;
    TRACKLIST *ret;

    ret = (TRACKLIST *)Z_Malloc(ROUND_UP(bufsize, sizeof(uint32_t)), PU_STATIC, NULL);
    if (!ret) {
        dprintf("%s() : fail, no free space\n", __func__);
        return NULL;
    }
    d_memset(ret, 0, bufsize);
    return ret;
}

static const TRACKLIST *__DD_SetupEmptyNames (const TRACKLIST *list, char *charbuf, int *cnt)
{
    const sndtrack_map_t *ptr;
    int i, n = 0;

    for (i = 0; i < NUMMUSIC; i++) {
        if ('\0' == list[i].name[0]) {
            ptr = &soundtrack_names[i];
            __DD_SetupListItem(list, ptr->name, i);
            n++;
        }
    }
    *cnt += n;
    return list;
}

static const TRACKLIST *__DD_SetupSoundtrackList (int f, int *cnt)
{
    char *charbuf, str[D_MAX_STRBUF], *strptr;
    const char *filename;
    const TRACKLIST *list;
    int i, id;

    list = __DD_AllocSoundtrackList();
    if (!list) {
        return NULL;
    }

    i = 0;
    while (!d_eof(f) && i < NUMMUSIC) {
        strptr = d_gets(f, str, sizeof(str));
        if (!strptr) {
            break;
        }
        filename = __DD_SoundtrackFindName(strptr, &id);
        if (filename) {
            __DD_SetupListItem(list, filename, id);
        } else {
            __DD_SetupListItem(list, soundtrack_names[id].name, id);
        }
        i++;
    }
    *cnt = i;
    if (i < NUMMUSIC - 1) {
       list =  __DD_SetupEmptyNames(list, charbuf, cnt);
    }
    assert(*cnt >= NUMMUSIC);
    return list;
}

static const TRACKLIST *__DD_SetupSoundtrackListDefault (int *cnt)
{
    const TRACKLIST *ret;
    char *charbuf;
    int i;

    ret = __DD_AllocSoundtrackList();
    if (!ret) {
        return NULL;
    }

    for (i = 0; i < NUMMUSIC; i++) {
        __DD_SetupListItem(ret, soundtrack_names[i].name, soundtrack_names[i].id);
    }
    *cnt = NUMMUSIC;
    return ret;
}

#define SNDTRACK_DECL(id) \
    {#id, id}

const sndtrack_map_t soundtrack_names[NUMMUSIC] =
{
    SNDTRACK_DECL(mus_None),
    SNDTRACK_DECL(mus_e1m1),
    SNDTRACK_DECL(mus_e1m2),
    SNDTRACK_DECL(mus_e1m3),
    SNDTRACK_DECL(mus_e1m4),
    SNDTRACK_DECL(mus_e1m5),
    SNDTRACK_DECL(mus_e1m6),
    SNDTRACK_DECL(mus_e1m7),
    SNDTRACK_DECL(mus_e1m8),
    SNDTRACK_DECL(mus_e1m9),
    SNDTRACK_DECL(mus_e2m1),
    SNDTRACK_DECL(mus_e2m2),
    SNDTRACK_DECL(mus_e2m3),
    SNDTRACK_DECL(mus_e2m4),
    SNDTRACK_DECL(mus_e2m5),
    SNDTRACK_DECL(mus_e2m6),
    SNDTRACK_DECL(mus_e2m7),
    SNDTRACK_DECL(mus_e2m8),
    SNDTRACK_DECL(mus_e2m9),
    SNDTRACK_DECL(mus_e3m1),
    SNDTRACK_DECL(mus_e3m2),
    SNDTRACK_DECL(mus_e3m3),
    SNDTRACK_DECL(mus_e3m4),
    SNDTRACK_DECL(mus_e3m5),
    SNDTRACK_DECL(mus_e3m6),
    SNDTRACK_DECL(mus_e3m7),
    SNDTRACK_DECL(mus_e3m8),
    SNDTRACK_DECL(mus_e3m9),
    SNDTRACK_DECL(mus_inter),
    SNDTRACK_DECL(mus_intro),
    SNDTRACK_DECL(mus_bunny),
    SNDTRACK_DECL(mus_victor),
    SNDTRACK_DECL(mus_introa),
    SNDTRACK_DECL(mus_runnin),
    SNDTRACK_DECL(mus_stalks),
    SNDTRACK_DECL(mus_countd),
    SNDTRACK_DECL(mus_betwee),
    SNDTRACK_DECL(mus_doom),
    SNDTRACK_DECL(mus_the_da),
    SNDTRACK_DECL(mus_shawn),
    SNDTRACK_DECL(mus_ddtblu),
    SNDTRACK_DECL(mus_in_cit),
    SNDTRACK_DECL(mus_dead),
    SNDTRACK_DECL(mus_stlks2),
    SNDTRACK_DECL(mus_theda2),
    SNDTRACK_DECL(mus_doom2),
    SNDTRACK_DECL(mus_ddtbl2),
    SNDTRACK_DECL(mus_runni2),
    SNDTRACK_DECL(mus_dead2),
    SNDTRACK_DECL(mus_stlks3),
    SNDTRACK_DECL(mus_romero),
    SNDTRACK_DECL(mus_shawn2),
    SNDTRACK_DECL(mus_messag),
    SNDTRACK_DECL(mus_count2),
    SNDTRACK_DECL(mus_ddtbl3),
    SNDTRACK_DECL(mus_ampie),
    SNDTRACK_DECL(mus_theda3),
    SNDTRACK_DECL(mus_adrian),
    SNDTRACK_DECL(mus_messg2),
    SNDTRACK_DECL(mus_romer2),
    SNDTRACK_DECL(mus_tense),
    SNDTRACK_DECL(mus_shawn3),
    SNDTRACK_DECL(mus_openin),
    SNDTRACK_DECL(mus_evil),
    SNDTRACK_DECL(mus_ultima),
    SNDTRACK_DECL(mus_read_m),
    SNDTRACK_DECL(mus_dm2ttl),
    SNDTRACK_DECL(mus_dm2int),
};

const char *DD_DoomBanner =
"\n"
"       ______ _____  ________  ___       \n"
"       |  _  \\  _  ||  _  |  \\/  |       \n"
" ______| | | | | | || | | | .  . |______ \n"
"|______| | | | | | || | | | |\\/| |______|\n"
"       | |/ /\\ \\_/ /\\ \\_/ / |  | |       \n"
"       |___/  \\___/  \\___/\\_|  |_/       \n"
"                                         \n"
"Copyright 1993 ID Software inc           \n"
"\n";

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
