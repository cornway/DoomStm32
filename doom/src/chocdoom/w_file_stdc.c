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
//	WAD I/O functions.
//

#include <stdio.h>
#include "string.h"

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"
#include "i_system.h"
#include <dev_io.h>

typedef struct
{
    wad_file_t wad;
    int fstream;
} stdc_wad_file_t;

extern wad_file_class_t stdc_wad_file;

static wad_file_t *W_StdC_OpenFile(char *path)
{
#if ORIGCODE
    stdc_wad_file_t *result;
    FILE *fstream;

    fstream = fopen(path, "rb");

    if (fstream == NULL)
    {
        return NULL;
    }

    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = M_FileLength(fstream);
    result->fstream = fstream;

    return &result->wad;
#else
    stdc_wad_file_t *result;
    int file, length;

    length = d_open(path, &file, "r");
    if (file < 0)
    {
    	return NULL;
    }

    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
	result->wad.file_class = &stdc_wad_file;
	result->wad.mapped = NULL;
    result->wad.length = length;
	result->fstream = file;

	return &result->wad;
#endif
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
#if ORIGCODE
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    fclose(stdc_wad->fstream);
    Z_Free(stdc_wad);
#else
	stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    d_close(stdc_wad->fstream);
    stdc_wad->fstream = -1;
    Z_Free(stdc_wad);	
#endif
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
#if ORIGCODE
    stdc_wad_file_t *stdc_wad;
    size_t result;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

    fseek(stdc_wad->fstream, offset, SEEK_SET);

    // Read into the buffer.

    result = fread(buffer, 1, buffer_len, stdc_wad->fstream);

    return result;
#else
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

	d_seek (stdc_wad->fstream, offset, DSEEK_SET);

    // Read into the buffer.

    return d_read(stdc_wad->fstream, buffer, buffer_len);
#endif
}

static wad_file_t *W_StdC_MMapFile(char *path)
{
    stdc_wad_file_t *result;
    unsigned int length;

    // Create a new stdc_wad_file_t to hold the file handle.
    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    
    result->wad.length = length;

    d_open (path, &result->fstream, "r");
    if (result->fstream < 0)
    {
        Z_Free(result);
        return NULL;
    }

    length = M_FileLength(result->fstream);
    result->wad.mapped = Z_Malloc(length, PU_STATIC, 0);

    if (d_read (result->fstream, result->wad.mapped, length) < 0)
    {
        I_Error("Ooops!");
    }
    d_close(result->fstream);
    return &result->wad;

}

typedef void (*w_handle_t)(void *);

static w_handle_t w_handle = NULL;
static char *w_path = NULL;

int W_StdC_ForeachHandle (char *name, ftype_t type)
{
    char buf[128] = {0};
    char path_to_file[128] = {0};
    char *ext;

    strncpy(buf, name, sizeof(buf));

    ext = buf + strlen(buf) - sizeof(WAD_EXT) + 1;
    strupr(ext);
    if (0 == strncmp(ext, WAD_EXT, sizeof(WAD_EXT))) {
        M_snprintf(path_to_file, sizeof(path_to_file),
            "%s/%s", w_path, name);

        w_handle(path_to_file);
        if (path_to_file[0] == 0) {
            return 1; /*successfully handled*/
        }
    }
    return 0;
}


static void W_StdC_Foreach(char *path, w_handle_t handle)
{
    flist_t flist = {W_StdC_ForeachHandle, NULL};
    w_path = path;
    w_handle = handle;
    d_dirlist(path, &flist);
}


wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
    W_StdC_MMapFile,
    W_StdC_Foreach,
};


