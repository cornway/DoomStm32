#include "main.h"
#include "gfx.h"
#include "lcd_main.h"

extern DMA2D_HandleTypeDef hdma2d_discovery;

void gfx_clear_screen (uint16_t color)
{
	BSP_LCD_Clear(color);
}



void gfx_draw_img_rgb565 (gfx_image_t* img, gfx_coord_t* coord)
{
    DMA2D_InitTypeDef init;
    DMA2D_LayerCfgTypeDef layer;
    uint32_t dest_address;
    uint32_t source_address;
    uint32_t offset;
    uint16_t picture_width;
    uint16_t picture_height;
    
    picture_width = img->width;
    picture_height = img->height;

    // check for dimensions
    if (coord->source_w == 0) return;
    if (coord->source_h == 0) return;
    if (coord->source_x + coord->source_w > picture_width) return;
    if (coord->source_y + coord->source_h > picture_height) return;
    if (coord->dest_x + coord->source_w > GFX_MAX_WIDTH) return;
    if (coord->dest_y + coord->source_h > GFX_MAX_HEIGHT) return;
    
    /* Configure the DMA2D Mode, Color Mode and output offset */
    init.Mode         = DMA2D_M2M_PFC;
#if (DOOM_PAL == RGB565_PAL) 
    init.ColorMode    = DMA2D_OUTPUT_RGB565; /* Output color out of PFC */
#else
    init.ColorMode    = DMA2D_OUTPUT_ARGB8888; /* Output color out of PFC */
#endif
    init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
    init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */   
    
    /* Output offset in pixels == nb of pixels to be added at end of line to come to the  */
    /* first pixel of the next line : on the output side of the DMA2D computation         */
    init.OutputOffset = (GFX_MAX_WIDTH - coord->source_w);
    
    /* Foreground Configuration */
    layer.AlphaMode = DMA2D_NO_MODIF_ALPHA;
    layer.InputAlpha = 0xFF; /* fully opaque */
    layer.InputColorMode = DMA2D_INPUT_RGB565;
    layer.InputOffset = 0;
    layer.RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
    layer.AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

    // target address in display RAM
    dest_address = lcd_frame_buffer + 2 * (GFX_MAX_WIDTH * coord->dest_y + coord->dest_x);

    // source address in image
    offset = 2 * (picture_width * coord->source_y + coord->source_x);
    source_address = (uint32_t)&img->pixel_data[offset];

    CopyImageToLcdFrameBufferParam(&init, 
                                                                        &layer, 
                                                                        source_address,
                                                                        dest_address,
                                                                        picture_width,
                                                                        picture_height);
}


void gfx_draw_img_argb8888 (gfx_image_t* img, gfx_coord_t* coord)
{
    DMA2D_InitTypeDef init;
    DMA2D_LayerCfgTypeDef layers[2];
    uint32_t dest_address;
    uint32_t source_address;
    uint32_t offset;
    uint16_t picture_width;
    uint16_t picture_height;

    // check for dimensions
    if (coord->source_w == 0) return;
    if (coord->source_h == 0) return;
    if (coord->source_x + coord->source_w > picture_width) return;
    if (coord->source_y + coord->source_h > picture_height) return;
    if (coord->dest_x + coord->source_w > GFX_MAX_WIDTH) return;
    if (coord->dest_y + coord->source_h > GFX_MAX_HEIGHT) return;
    
    /* Configure the DMA2D Mode, Color Mode and output offset */
    init.Mode         = DMA2D_M2M_PFC;
#if (DOOM_PAL == RGB565_PAL) 
    init.ColorMode    = DMA2D_OUTPUT_RGB565; /* Output color out of PFC */
#else
    init.ColorMode    = DMA2D_OUTPUT_ARGB8888; /* Output color out of PFC */
#endif
    init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
    init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */   
    
    /* Output offset in pixels == nb of pixels to be added at end of line to come to the  */
    /* first pixel of the next line : on the output side of the DMA2D computation         */
    init.OutputOffset = (GFX_MAX_WIDTH - coord->source_w);
    
    /* Foreground Configuration */
    layers[LCD_BACKGROUND].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    layers[LCD_BACKGROUND].InputAlpha = 0xFF; /* fully opaque */
    layers[LCD_BACKGROUND].InputColorMode = DMA2D_INPUT_RGB565;
    layers[LCD_BACKGROUND].InputOffset = 0;
    layers[LCD_BACKGROUND].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
    layers[LCD_BACKGROUND].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

    /* Foreground Configuration */
    layers[LCD_FOREGROUND].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    layers[LCD_FOREGROUND].InputAlpha = 0xFF; /* fully opaque */
    layers[LCD_FOREGROUND].InputColorMode = DMA2D_INPUT_ARGB8888;
    layers[LCD_FOREGROUND].InputOffset = 0;
    layers[LCD_FOREGROUND].RedBlueSwap = DMA2D_RB_REGULAR; /* No ForeGround Red/Blue swap */
    layers[LCD_FOREGROUND].AlphaInverted = DMA2D_REGULAR_ALPHA; /* No ForeGround Alpha inversion */

    picture_width = img->width;
    picture_height = img->height;
    // target address in display RAM
    dest_address = lcd_frame_buffer + 2 * (GFX_MAX_WIDTH * coord->dest_y + coord->dest_x);

    // source address in image
    offset = 4 * (picture_width * coord->source_y + coord->source_x);
    source_address = (uint32_t)&img->pixel_data[offset];

    CopyImageToLcdFrameBufferParamBlend(&init, 
                                                                        layers, 
                                                                        source_address,
                                                                        dest_address,
                                                                        picture_width,
                                                                        picture_height);
}

void gfx_fill_rect (gfx_image_t* img, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
	DMA2D_InitTypeDef init;
    DMA2D_LayerCfgTypeDef layer;
	uint32_t offset;
	uint32_t address;
         HAL_StatusTypeDef ret = HAL_OK;

	if (x + width > img->width) return;
	if (y + height > img->height) return;

	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
		case GFX_PIXEL_FORMAT_ARGB8888:
			break;

		default:
			fatal_error ("unknown pixel format");
			return;
	}

	offset = gfx_bytes_per_pixel (img) * (img->width * y + x);
	address = (uint32_t)&img->pixel_data[offset];

        init.Mode         = DMA2D_R2M;
        init.ColorMode    = gfx_convert_colormode (img->pixel_format); /* Output color out of PFC */
        init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /* No Output Alpha Inversion*/  
        init.RedBlueSwap   = DMA2D_RB_REGULAR;     /* No Output Red & Blue swap */   
        
        /* Output offset in pixels == nb of pixels to be added at end of line to come to the  */
        /* first pixel of the next line : on the output side of the DMA2D computation         */
        init.OutputOffset = img->width - width;
	CopyImageToLcdFrameBufferParam (&init, &layer, color, address, width, height);
}


/*
 * Convert pixel format into color mode
 *
 * @param[in]	pixel_format	Pixel format to convert
 * @return		Converted color mode
 */
uint32_t gfx_convert_colormode (gfx_pixel_format_t pixel_format)
{
	switch (pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			return DMA2D_RGB565;

		case GFX_PIXEL_FORMAT_ARGB8888:
			return DMA2D_ARGB8888;

		default:
			fatal_error ("unknown pixel format");
			return 0;
	}
}

uint8_t gfx_bytes_per_pixel (gfx_image_t* img)
{
	switch (img->pixel_format)
	{
		case GFX_PIXEL_FORMAT_RGB565:
			return 2;

		case GFX_PIXEL_FORMAT_ARGB8888:
			return 4;

		default:
			fatal_error ("unknown pixel format");
			return 0;
	}
}



