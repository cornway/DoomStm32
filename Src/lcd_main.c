/**
  ******************************************************************************
  * @file    DMA2D/DMA2D_MemToMemWithBlending/Src/main.c
  * @author  MCD Application Team
  * @brief   This example provides a description of how to configure
  *          DMA2D peripheral in Memory to Memory with Blending transfer mode
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
#include "lcd_main.h"

extern LTDC_HandleTypeDef  hltdc_discovery;

extern volatile pal_t *__lcd_frame_buf_raw;
uint32_t lcd_frame_buffer;
lcd_layers_t lcd_layer = LCD_BACKGROUND;

static const lcd_layers_t layer_switch[] =
{
    [LCD_BACKGROUND] = LCD_FOREGROUND,
    [LCD_FOREGROUND] = LCD_BACKGROUND,
};

static const uint8_t layer_transparency[] =
{
    [LCD_BACKGROUND] = GFX_TRANSPARENT,
    [LCD_FOREGROUND] = GFX_OPAQUE,
};

static uint32_t layer_addr [LCD_MAX_LAYER];

void lcd_load_clut (void *_buf, int size, int layer)
{
    HAL_LTDC_ConfigCLUT(&hltdc_discovery, (uint32_t *)_buf, size, layer);
    HAL_LTDC_EnableCLUT(&hltdc_discovery, layer);
}

void lcd_init (void)
{
    if(BSP_LCD_Init())
    {
        while (1) {}
    }
    BSP_LCD_SetLayerVisible(LCD_FOREGROUND, ENABLE);
    BSP_LCD_SetLayerVisible(LCD_BACKGROUND, ENABLE);

    lcd_frame_buffer = (uint32_t)__lcd_frame_buf_raw;
    layer_addr[LCD_BACKGROUND] = (uint32_t)__lcd_frame_buf_raw + LCD_FRAME_SIZE;
    layer_addr[LCD_FOREGROUND] = (uint32_t)__lcd_frame_buf_raw;
}

/*
 * Set the layer to draw to
 *
 * This has no effect on the LCD itself, only to drawing routines
 *
 * @param[in]	layer	layer to change to
 */
static inline void lcd_set_layer (lcd_layers_t layer)
{
    lcd_frame_buffer = layer_addr[layer];
    BSP_LCD_SetTransparency(LCD_FOREGROUND, layer_transparency[layer]);
}

static inline void lcd_wait_ready ()
{
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
}

void lcd_refresh (void)
{
    lcd_wait_ready();
    lcd_layer = layer_switch[lcd_layer];
    lcd_set_layer(lcd_layer);
}

uint32_t lcd_get_ready_layer_addr (void)
{
    return layer_addr[layer_switch[lcd_layer]];
}

static void _BSP_LCD_LayerDefaultInit(
    lcd_layers_t LayerIndex,
    uint32_t FB_Address,
    uint32_t format,
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h
)
{
  LCD_LayerCfgTypeDef  Layercfg;

  /* Layer Init */
  Layercfg.WindowX0 = x;
  Layercfg.WindowX1 = x + w;
  Layercfg.WindowY0 = y;
  Layercfg.WindowY1 = y + h;
  Layercfg.PixelFormat = format;
  Layercfg.FBStartAdress = FB_Address;
  Layercfg.Alpha = 255;
  Layercfg.Alpha0 = 255;
  Layercfg.Backcolor.Blue = 0;
  Layercfg.Backcolor.Green = 0;
  Layercfg.Backcolor.Red = 0;
  Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR1_CA;
  Layercfg.ImageWidth = w;
  Layercfg.ImageHeight = h;

  HAL_LTDC_ConfigLayer(&hltdc_discovery, &Layercfg, LayerIndex);
}


void lcd_load_palette (pal_t *palette, uint32_t pal_size, uint32_t width, uint32_t height)
{
    uint32_t scale_w = (GFX_MAX_WIDTH / width);
    uint32_t scale_h = (GFX_MAX_HEIGHT / height);
    uint32_t w = width * scale_w;
    uint32_t h = height * scale_h;
    uint32_t x = (GFX_MAX_WIDTH - w) / scale_w;
    uint32_t y = (GFX_MAX_HEIGHT - h) / scale_h;
    uint32_t format = LTDC_PIXEL_FORMAT_L8;
    uint32_t layer_fb;
    int layer;

    for (layer = 0; layer < (int)LCD_MAX_LAYER; layer++) {
        layer_fb = layer_addr[layer];
        _BSP_LCD_LayerDefaultInit((lcd_layers_t)layer, layer_fb, format, x, y, w, h);
        lcd_load_clut (palette, pal_size, layer);
    }
}


/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

