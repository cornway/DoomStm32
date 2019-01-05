#ifndef _AUDIO_MAIN_H
#define _AUDIO_MAIN_H

#include "main.h"

#define USE_STEREO 0
#define USE_REVERB 0


#define AUDIO_SIZE_TO_MS(rate, size) (((long long)(size) * 1000) / (rate))
#define AUDIO_MS_TO_SIZE(rate, ms) (((((rate) << 2) / 1000) * (ms)) >> 2)

#define AUDIO_SAMPLE_RATE I2S_AUDIOFREQ_22K
#define AUDIO_MAX_CHANS 12
#define AUDIO_MUS_CHAN_START AUDIO_MAX_CHANS
#define AUDIO_OUT_BUFFER_SIZE 0x400
#define AUDIO_BUFFER_MS AUDIO_SIZE_TO_MS(AUDIO_SAMPLE_RATE, AUDIO_OUT_BUFFER_SIZE)

#define REVERB_DELAY 5/*Ms*/
#define REVERB_SAMPLE_DELAY AUDIO_MS_TO_SIZE(AUDIO_SAMPLE_RATE, REVERB_DELAY)
#define REVERB_DECAY_MAX 255
#define REVERB_DECAY 128
#define REVERB_LIFE_TIME 1000

#define MAX_VOL (0x7f)

typedef struct {
  uint32_t ChunkID;       /* 0 */ 
  uint32_t FileSize;      /* 4 */
  uint32_t FileFormat;    /* 8 */
  uint32_t SubChunk1ID;   /* 12 */
  uint32_t SubChunk1Size; /* 16*/  
  uint16_t AudioFormat;   /* 20 */ 
  uint16_t NbrChannels;   /* 22 */   
  uint32_t SampleRate;    /* 24 */
  
  uint32_t ByteRate;      /* 28 */
  uint16_t BlockAlign;    /* 32 */  
  uint16_t BitPerSample;  /* 34 */  
  uint32_t SubChunk2ID;   /* 36 */   
  uint32_t SubChunk2Size; /* 40 */    
} wave_t;


typedef int16_t snd_sample_t;

typedef struct Mix_Chunk {
    int allocated;
    snd_sample_t *abuf;
    int32_t alen;
    uint8_t volume;     /* Per-sample volume, 0-128 */
} Mix_Chunk;


typedef struct {
    Mix_Chunk chunk;
    int8_t is_playing;
    uint8_t id;
    int (*complete) (int);
} audio_channel_t;

void audio_init (void);

audio_channel_t *audio_play_channel (Mix_Chunk *chunk, int channel);
audio_channel_t *audio_stop_channel (int channel);
void audio_pause (int channel);
void audio_sdown (int dev);
int audio_is_playing (int handle);
void audio_set_pan (int handle, int l, int r);


int music_play_song_num (int num, int repeat);
int music_play_song_name (const char *name);
int music_pause (void);
int music_resume (void);
int music_stop (void);
int music_set_vol (uint8_t vol);
int music_playing (void);


#endif /*_AUDIO_MAIN_H*/
