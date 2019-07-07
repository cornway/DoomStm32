/**
  ******************************************************************************
  * @file    JPEG/JPEG_DecodingUsingFs_DMA/Src/main.c
  * @author  MCD Application Team
  * @brief   This sample code shows how to use the HW JPEG to Decode a JPEG file with DMA method.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd_main.h"
#include "i_video.h"
#include <bsp_api.h>
#include <debug.h>
#include <audio_main.h>
#include <input_main.h>
#include <misc_utils.h>
#include <heap.h>

const char *mus_dir_path_psx = "/doom/music/psx";
const char *mus_dir_path_3do = "/doom/music/3do";
const char *snd_dir_path = "/doom/sound";
const char *game_dir_path = "/doom";

extern int d_main(void);
extern void dev_main (void);

static void *__vid_alloc (uint32_t size)
{
    return heap_alloc_shared(size);
}

void VID_PreConfig (void)
{
    screen_t screen;
    screen.buf = NULL;
    screen.width = SCREENWIDTH;
    screen.height = SCREENHEIGHT;
    vid_config(__vid_alloc, NULL, &screen, GFX_COLOR_MODE_CLUT, 2);
}

#endif


extern void dev_main (void);



int mainloop (int argc, const char **argv)
{
#if defined(BSP_DRIVER)
    static const char *_argv[] =
    {
        "doom",
        "-iwad", "doom2.wad",
        "-decor", "psx",
    };
    myargc = arrlen(_argv);
    myargv = _argv;
#else
    myargc = argc;
    myargv = (char **)argv;
#endif
    audio_conf("samplerate=22050, volume=100");
    d_main();
    return 0;
}

int main(void)
{
    dev_main();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
