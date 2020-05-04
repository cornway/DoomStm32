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
#include <bsp_sys.h>
#include <heap.h>

extern int DD_DoomMain(void);
extern void dev_main (void);
extern const char *game_dir_path;

static bsp_user_api_t bsp_user_api =
{
    .heap =
    {
        .malloc = heap_malloc,
        .free = heap_free,
    },
};

void VID_PreConfig (void)
{
    screen_conf_t conf;
    int hwaccel = 0, p;

    p = bsp_argv_check("-gfxmod");
    if (p >= 0) {
        const char *str = bsp_argv_get(p + 1);
        hwaccel = atoi(str);
    }

    conf.res_x = SCREENWIDTH;
    conf.res_y = SCREENHEIGHT;
    conf.alloc.malloc = heap_alloc_shared;
    conf.alloc.free = heap_free;
    conf.colormode = GFX_COLOR_MODE_CLUT;
    conf.laynum = 2;
    conf.hwaccel = hwaccel;
    conf.clockpresc = 1;
    conf.filter = 1;
    vid_config(&conf);
}

extern void app_main (void);

extern int myargc;
extern char **myargv;

int mainloop (int argc, const char **argv)
{
#if defined(BSP_DRIVER)
    static const char *_argv[] =
    {
        "doom",
        "-iwad", "doom2.wad",
        "-decor", "psx",
        "-vol", "64",
    };
    myargc = arrlen(_argv);
    myargv = _argv;
#else
    myargc = argc;
    myargv = (char **)argv;
#endif
    assert(argc > 0);
    game_dir_path = argv[0];
    sys_user_attach(&bsp_user_api);
    DD_DoomMain();
    return 0;
}

int main(void)
{
    app_main();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
