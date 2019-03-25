#ifndef __DEV_CONF_H__
#define __DEV_CONF_H__

#define NVIC_IRQ_MAX 32

#define DEBUG_SERIAL 1

#define TX_STREAM_USE_DMA           (1)
#define MAX_UARTS                   (1)
#define TX_STREAM_BUFERIZED         (1)

#define AUDIO_MODULE_PRESENT        (1)
#define MUSIC_MODULE_PRESENT        (1)

#define GFX_COLOR_MODE GFX_COLOR_MODE_CLUT

#define HEAP_CACHE_SIZE 0

#define AUDIO_SAMPLE_RATE 22050U

#endif /*__DEV_CONF_H__*/
