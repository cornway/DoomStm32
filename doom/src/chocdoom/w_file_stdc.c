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

#include <misc_utils.h>
#include <debug.h>
#include <dev_io.h>
#include <bsp_sys.h>

#include <d_main.h>
#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"
#include "i_system.h"

#define W_IO_SHARED 1

typedef struct
{
    wad_file_t wad;
#if W_IO_SHARED
    char path[D_MAX_PATH];
#else
    int fstream;
#endif
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
#if W_IO_SHARED
    d_close(file);
#endif
    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
	result->wad.file_class = &stdc_wad_file;
	result->wad.mapped = NULL;
    result->wad.length = length;
#if W_IO_SHARED
    snprintf(result->path, sizeof(result->path), "%s", path);
#else
	result->fstream = file;
#endif
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

    if (stdc_wad->wad.mapped) {
        Z_Free(stdc_wad->wad.mapped);
        stdc_wad->wad.mapped = NULL;
    } else {
#if !W_IO_SHARED
        d_close(stdc_wad->fstream);
        stdc_wad->fstream = -1;
#endif
    }
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

    if (stdc_wad->wad.mapped) {
        byte *ptr = stdc_wad->wad.mapped;
        d_memcpy(buffer, ptr + offset, buffer_len);
        return buffer_len;
    }

#if W_IO_SHARED
    {
        int f, size, err;

        size = d_open(stdc_wad->path, &f, "r");
        if (size < 0) {
            return size;
        }
        if (size != stdc_wad->wad.length) {
            dprintf("%s() : file length differs; [%u] -> [%u]\n",
                __func__, size, stdc_wad->wad.length);
            assert(0);
        }
        err = d_seek (f, offset, DSEEK_SET);
        err = err < 0 ? err : d_read(f, buffer, buffer_len);
        d_close(f);
        return err;
    }
#else
    d_seek (stdc_wad->fstream, offset, DSEEK_SET);
    // Read into the buffer.

    return d_read(stdc_wad->fstream, buffer, buffer_len);
#endif /*W_IO_SHARED*/
#endif
}

static wad_file_t *W_StdC_MapFile(char *path)
{
    stdc_wad_file_t *result;
    unsigned int length;
    int f;

    // Create a new stdc_wad_file_t to hold the file handle.
    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    
    result->wad.length = length;

    length = d_open (path, &f, "r");
    if (f < 0)
    {
        Z_Free(result);
        return NULL;
    }

    result->wad.mapped = Z_Malloc(length, PU_STATIC, 0);

    if (d_read (f, result->wad.mapped, length) < 0)
    {
        I_Error("Ooops!");
    }
    d_close(f);
    return &result->wad;

}

static void W_StdC_Foreach(char *dirpath, void (*handle)(void *))
{
    char path[D_MAX_PATH];
    fobj_t obj;
    int dir, i;

    dir = d_opendir(dirpath);
    if (dir < 0) {
        return;
    }
    while (d_readdir(dir, &obj) >= 0) {
        if (0 == obj.attr.dir) {
            sprintf(path, "%s/%s", dirpath, obj.name);
            handle(path);
        }
    }
    d_closedir(dir);
}


wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
    W_StdC_MapFile,
    W_StdC_Foreach,
};


