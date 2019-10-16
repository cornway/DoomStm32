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


#ifndef __D_MAIN__
#define __D_MAIN__

#include "doomdef.h"




// Read events from all input devices

void D_ProcessEvents (void); 
	

//
// BASE LEVEL
//
void D_PageTicker (void);
void D_PageDrawer (void);
void D_AdvanceDemo (void);
void D_DoAdvanceDemo (void);
void D_StartTitle (void);


void DD_PwadAddEach (void (*handle)(void *));
void DD_SetGameAct (gameaction_t action);
void DD_ProcGameAct (void);
void DD_LoadAltPkgGame(void);
void DD_UpdateNoBlit (void);

extern char *__DD_GetPath (char *path, const char **subdir);
extern const char *__DD_DoomPath (void);

#define DD_GETPATH(path, args...)            \
do {                                         \
    const char *__p[] = {args, NULL};        \
    __DD_GetPath(path, (const char **)__p);  \
} while (0)

#define DD_DOOMPATH() __DD_DoomPath()

void DD_SetupSoundtrackList (int *cnt);
int DD_PlaySoundtrackNum (void *_cd, int num, int volume);

//
// GLOBAL VARIABLES
//

extern  gameaction_t    gameaction;
//extern gameaction_t    gameaction_next;

#endif

