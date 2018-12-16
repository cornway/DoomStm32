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
#include "gfx.h"
#if 0
#include "RGB565_240x130_1.h"
#include "RGB565_240x130_2.h"
#endif

extern LTDC_HandleTypeDef  hltdc_discovery;
extern LCD_DrawPropTypeDef DrawProp[LTDC_MAX_LAYER_NUMBER];

extern volatile pal_t *__lcd_frame_buf_raw;
uint32_t lcd_frame_buffer;
lcd_layers_t lcd_layer = LCD_BACKGROUND;
uint8_t lcd_vsync = 0;
volatile uint8_t lcd_refreshed;

static void DMA2D_Config(void);



void lcd_attach_buf (void)
{
    lcd_frame_buffer = (uint32_t)__lcd_frame_buf_raw;
}


static void _BSP_LCD_LayerDefaultInit(
    uint16_t LayerIndex,
    int x,
    int y,
    int w,
    int h,
    uint32_t FB_Address,
    int format)
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

void lcd_set_addr (uint32_t addr, int layer)
{
    HAL_LTDC_SetAddress(&hltdc_discovery, addr, layer);
}

void lcd_config_layer (
        uint32_t buffer,
        int x,
        int y,
        int w,
        int h,
        int layer,
        int format)
{
    _BSP_LCD_LayerDefaultInit(layer, x, y, w, h, buffer, format);
}

void lcd_load_clut (void *_buf, int size, int layer)
{
    HAL_LTDC_ConfigCLUT(&hltdc_discovery, (uint32_t *)_buf, size, layer);
    HAL_LTDC_EnableCLUT(&hltdc_discovery, layer);
}

#if 1
void Display_Init (void)
{
    if(BSP_LCD_Init())
    {
        while (1) {}
    }
    BSP_LCD_SetLayerVisible(0, ENABLE);
    BSP_LCD_SetLayerVisible(1, ENABLE);
}

#else
void Display_Init (void)
{
    int w, h, format;
    if(BSP_LCD_Init())
    {
        while (1) {}
    }
#if (DOOM_PAL == RGB565_PAL)
    format = LTDC_PIXEL_FORMAT_RGB565;
#else
    format = LTDC_PIXEL_FORMAT_ARGB8888;
#endif
    w = BSP_LCD_GetXSize();
    h = BSP_LCD_GetYSize();
    _BSP_LCD_LayerDefaultInit(0, 0, 0, w, h,
        (uint32_t)lcd_frame_buffer, format);     
    _BSP_LCD_LayerDefaultInit(1, 0, 0, w, h,
        (uint32_t)lcd_frame_buffer + LCD_FRAME_SIZE, format);
    BSP_LCD_SetLayerVisible(1, DISABLE);
    BSP_LCD_SetLayerVisible(0, ENABLE);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE); 
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    //DMA2D_Config();
}
#endif

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/** @addtogroup DMA2D_MemToMemWithBlending
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern DMA2D_HandleTypeDef hdma2d_discovery;

uint8_t CopyImageToLcdFrameBufferParam(
    DMA2D_InitTypeDef *init,
    DMA2D_LayerCfgTypeDef *layer,
    uint32_t pSrc,
    uint32_t pDst,
    uint32_t xSize,
    uint32_t ySize)
{
    uint8_t lcd_status = LCD_ERROR;
    HAL_StatusTypeDef hal_status = HAL_OK;

    hdma2d_discovery.Init = *init;
    hdma2d_discovery.LayerCfg[1] = *layer;
    
    if(HAL_DMA2D_Init(&hdma2d_discovery) == HAL_OK)
    {
    if(HAL_DMA2D_ConfigLayer(&hdma2d_discovery, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hdma2d_discovery, (uint32_t)pSrc, (uint32_t)pDst, xSize, ySize) == HAL_OK)
      {
        /* Polling For DMA transfer */
        hal_status = HAL_DMA2D_PollForTransfer(&hdma2d_discovery, 10);
        if(hal_status == HAL_OK)
        {
          /* return good status on exit */
          lcd_status = LCD_OK;
        }
      }
    }
  }
  return(lcd_status);
}

uint8_t CopyImageToLcdFrameBufferParamBlend(
    DMA2D_InitTypeDef *init,
    DMA2D_LayerCfgTypeDef *layers,
    uint32_t pSrc,
    uint32_t pDst,
    uint32_t xSize,
    uint32_t ySize)
{
    uint8_t lcd_status = LCD_ERROR;
    HAL_StatusTypeDef hal_status = HAL_OK;

    hdma2d_discovery.Init = *init;
    hdma2d_discovery.LayerCfg[0] = layers[0];
    hdma2d_discovery.LayerCfg[1] = layers[1];
    
    
    if(HAL_DMA2D_Init(&hdma2d_discovery) == HAL_OK)
    {
    if(HAL_DMA2D_ConfigLayer(&hdma2d_discovery, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hdma2d_discovery, (uint32_t)pSrc, (uint32_t)pDst, xSize, ySize) == HAL_OK)
      {
        /* Polling For DMA transfer */
        hal_status = HAL_DMA2D_PollForTransfer(&hdma2d_discovery, 10);
        if(hal_status == HAL_OK)
        {
          /* return good status on exit */
          lcd_status = LCD_OK;
        }
      }
    }
  }
  return(lcd_status);
}





/*
 * Set the layer to draw to
 *
 * This has no effect on the LCD itself, only to drawing routines
 *
 * @param[in]	layer	layer to change to
 */
void lcd_set_layer (lcd_layers_t layer)
{
    lcd_layer = layer;
	switch (layer)
	{
		case LCD_BACKGROUND:
			lcd_frame_buffer = (uint32_t)__lcd_frame_buf_raw + LCD_FRAME_SIZE;
            BSP_LCD_SetTransparency(LCD_FOREGROUND, GFX_TRANSPARENT);
			break;

		case LCD_FOREGROUND:
			lcd_frame_buffer = (uint32_t)__lcd_frame_buf_raw;
            BSP_LCD_SetTransparency(LCD_FOREGROUND, GFX_OPAQUE);
			break;
		default:
			break;
	}
    //BSP_LCD_SetLayerVisible(layer, DISABLE);
    //BSP_LCD_SetLayerVisible(layer ^ 1, ENABLE);
}

static const lcd_layers_t opaq[] = {GFX_TRANSPARENT, GFX_OPAQUE, GFX_TRANSPARENT};
void lcd_enable_layer (lcd_layers_t layer)
{
    lcd_layer = layer;
    lcd_set_transparency (LCD_FOREGROUND, opaq[layer]);
}



void lcd_wait_ready ()
{
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
}

void lcd_refresh (void)
{
	switch (lcd_layer)
	{
		case LCD_BACKGROUND:
			lcd_set_layer (LCD_FOREGROUND);
			break;
		case LCD_FOREGROUND:
			lcd_set_layer (LCD_BACKGROUND);
			break;

		default:
			break;
	}
}

uint32_t lcd_get_ready_layer (void)
{
    switch (lcd_layer)
    {
        case LCD_FOREGROUND:
            return (uint32_t)__lcd_frame_buf_raw + LCD_FRAME_SIZE;
        break;

        case LCD_BACKGROUND:
            return (uint32_t)__lcd_frame_buf_raw;
        break;
        default:
        break;
    }
    return 0;
}

uint32_t lcd_get_layer_addr (int layer)
{
    switch (layer)
    {
        case LCD_FOREGROUND:
            return (uint32_t)__lcd_frame_buf_raw + LCD_FRAME_SIZE;
        break;

        case LCD_BACKGROUND:
            return (uint32_t)__lcd_frame_buf_raw;
        break;
        default:
        break;
    }
    return 0;
}

int lcd_get_ready_layer_idx (void)
{
    switch (lcd_layer)
    {
        case LCD_FOREGROUND:
            return LCD_BACKGROUND;
        break;

        case LCD_BACKGROUND:
            return LCD_FOREGROUND;
        break;
        default:
        break;
    }
    return 0;
}




/*
 * Set transparency of layer
 *
 * @param[in]	layer			layer to change
 * @param[in]	transparency	0x00 is transparent, 0xFF is opaque
 */
void lcd_set_transparency (lcd_layers_t layer, uint8_t transparency)
{
	BSP_LCD_SetTransparency((int)layer, transparency);
}

DMA2D_HandleTypeDef dma2d_hdl;


void lcd_copy_image_clut (void *dest, void *src, int x, int y)
{
    if (HAL_DMA2D_Start(&dma2d_hdl,
                            (uint32_t)src,
                            (uint32_t)dest,
                            x,
                            y) == HAL_OK)
    {
        /* Polling For DMA transfer */
        HAL_DMA2D_PollForTransfer(&dma2d_hdl, 10);
    }
}

static void (*lcd_hdl) (uint32_t) = NULL;
void lcd_dma_reg_handle (void (*hdl) (uint32_t))
{
    lcd_hdl = hdl;
}

/**
  * @brief  DMA2D Transfer completed callback
  * @param  hdma2d: DMA2D handle.
  * @note   This example shows a simple way to report end of DMA2D transfer, and
  *         you can add your own implementation.
  * @retval None
  */
static void TransferComplete(DMA2D_HandleTypeDef *hdma2d)
{
    if (lcd_hdl) (*lcd_hdl)(1);
}

/**
  * @brief  DMA2D error callbacks
  * @param  hdma2d: DMA2D handle
  * @note   This example shows a simple way to report DMA2D transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
static void TransferError(DMA2D_HandleTypeDef *hdma2d)
{
  while(1)
  {
    /* Toggles LED1 every 200ms */
    BSP_LED_Toggle(LED1);
    
    HAL_Delay(200);
  }
}


static void DMA2D_Config(void)
{
  HAL_StatusTypeDef hal_status = HAL_OK;

  /* Configure the DMA2D Mode, Color Mode and output offset */
  dma2d_hdl.Init.Mode         = DMA2D_M2M_PFC; /* DMA2D mode Memory to Memory with Blending */
  dma2d_hdl.Init.ColorMode    = DMA2D_INPUT_RGB565; /* output format of DMA2D */
  dma2d_hdl.Init.OutputOffset = 0;//800 - 320;  /* No offset in output */
  dma2d_hdl.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
  dma2d_hdl.Init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */   

  /* DMA2D Callbacks Configuration */
  dma2d_hdl.XferCpltCallback  = TransferComplete;
  dma2d_hdl.XferErrorCallback = TransferError;

  /* Foreground layer Configuration */
  dma2d_hdl.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
  dma2d_hdl.LayerCfg[1].InputAlpha = 0xFF;
  dma2d_hdl.LayerCfg[1].InputColorMode = DMA2D_INPUT_L8;
  dma2d_hdl.LayerCfg[1].InputOffset = 0;//800 - 320; /* No offset in input */
  dma2d_hdl.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
  dma2d_hdl.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */
  
  /* Background layer Configuration */
  dma2d_hdl.LayerCfg[0].AlphaMode = DMA2D_REPLACE_ALPHA;
  dma2d_hdl.LayerCfg[0].InputAlpha = 0xFF;
  dma2d_hdl.LayerCfg[0].InputColorMode = DMA2D_INPUT_L8;
  dma2d_hdl.LayerCfg[0].InputOffset = 0x0; /* No offset in input */
  dma2d_hdl.LayerCfg[0].RedBlueSwap = DMA2D_RB_REGULAR; /* No BackGround Red/Blue swap */
  dma2d_hdl.LayerCfg[0].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No BackGround Alpha inversion */  

  dma2d_hdl.Instance = DMA2D;

  /* DMA2D Initialization */
  hal_status = HAL_DMA2D_Init(&dma2d_hdl);

  /* Apply DMA2D Foreground configuration */
  HAL_DMA2D_ConfigLayer(&dma2d_hdl, 1);

  /* Apply DMA2D Background configuration */
  HAL_DMA2D_ConfigLayer(&dma2d_hdl, 0);
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

