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
//	Put all global tate variables here.
//

#include "doomstat.h"
#include "doomdef.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t      gamemode = indetermined;
GameMission_t   gamemission = doom;
GameVersion_t   gameversion = exe_final;
char            *gamedescription;

boolean         nerve = false;
boolean         bfgedition = false;

// Set if homebrew PWAD stuff has been added.
boolean         modifiedgame;

const char *g_pwad_name = "";

boolean         MAPINFO  = false;
boolean         M_EPISOD = false;
boolean         M_LGTTL = false;
boolean         M_MSENS = false;
boolean         M_MSGOFF = false;
boolean         M_MSGON = false;
boolean         M_NEWG = false;
boolean         M_NMARE = false;
boolean         M_OPTTTL = false;
boolean         M_SGTTL = false;
boolean         M_SKILL = false;
boolean         M_SKULL1 = false;
boolean         M_SVOL = false;
boolean         STBAR    = false;
boolean         STCFN034 = false;
boolean         STCFN039 = false;
boolean         STYSNUM0 = false;
boolean         TITLEPIC = false;
boolean         WISCRT2  = false;
