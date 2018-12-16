#include "audio_main.h"
#include "stm32f769i_discovery_audio.h"
#include "wm8994.h"

#define COMPRESSION 1
#define USE_FLOAT 1

extern void music_tickle (void);
extern snd_sample_t *music_get_next_chunk (int32_t *size);
extern uint8_t music_get_volume (void);
extern int music_init (void);

#define AUDIO_TIMEOUT_MAX 1000 /*2 s*/
#define MAX_2BAND_VOL ((MAX_VOL) | (MAX_VOL << 8))
#define MAX_4BAND_VOL ((MAX_2BAND_VOL) | (MAX_2BAND_VOL << 16))

#define chan_desc_head_t struct chan_desc_head
#define chan_desc_t struct chan_desc

#define DEFINE_LLIST_NEXT() \
    chan_desc_t *next

struct chan_desc {
    chan_desc_t *next;
    chan_desc_t *prev;
    
    audio_channel_t inst;
#if USE_STEREO
    uint8_t left, right;
#endif
    uint8_t effect;
};

struct chan_desc_head {
    chan_desc_t       *first,
                      *last;
    uint16_t size;
    void (*empty_handle) (struct chan_desc_head *head);
    void (*first_link_handle) (struct chan_desc_head *head);
    void (*remove_handle) (struct chan_desc_head *head, chan_desc_t *rem);
};

#define chan_len(chan) \
    ((chan)->inst.chunk.alen)

#define chan_buf(chan) \
    ((chan)->inst.chunk.abuf)

#define chan_vol(chan) \
    ((chan)->inst.chunk.volume)

#define chan_is_play(chan) \
    ((chan)->inst.is_playing)

#define chan_foreach(head, cur) \
for (chan_desc_t *cur = (head)->first,\
    *__next = cur->next;               \
     cur;                              \
     cur = __next,                     \
     __next = __next->next)



static int chan_link (chan_desc_head_t *head,
                        chan_desc_t *link,
                        uint8_t sort)
{
    head->size++;
    if (head->first == NULL) {
        head->first = link;
        head->last = link;
        link->next = NULL;
        link->prev = NULL;
        if (head->first_link_handle)
            head->first_link_handle(head);
        return head->size;
    }
    chan_foreach(head, cur) {
        if ((chan_len(link) < chan_len(cur))) {
            if (cur->prev != NULL) {
                cur->prev->next = link;
                link->next = cur;
                link->prev = cur->prev;
                cur->prev = link;
                return head->size;
            }
            link->next = cur;
            link->prev = NULL;
            cur->prev = link;
            head->first = link;
            return head->size;
         }
    }
    link->prev = head->last;
    link->next = NULL;
    head->last->next = link;
    head->last = link;
    return head->size;                 
}

static int
chan_unlink (chan_desc_head_t *head,
                chan_desc_t *node)
{
    if (!head->size) {
        return 0;
    }
    head->size--;
    chan_desc_t *prev = node->prev, *next = node->next;
    if (!head->size) {
        head->first = NULL;
        head->last = NULL;

        if (head->remove_handle)
            head->remove_handle(head, node);

        if (head->empty_handle)
            head->empty_handle(head);
        return 0;
    }
    if (!prev) {
        head->first = next;
        next->prev = NULL;
    } else if (next) {
        prev->next = next;
        next->prev = prev;
    } else if (!next) {
        prev->next = NULL;                    
        head->last = prev;
    }
    if (head->remove_handle)
            head->remove_handle(head, node);

    return head->size;
}

static void clear_buf (uint8_t idx);

extern uint32_t systime;

static chan_desc_t channels[AUDIO_MAX_CHANS];
chan_desc_head_t chan_llist_ready;
chan_desc_head_t chan_llist_susp;

static uint16_t audio_raw_buf[2][AUDIO_OUT_BUFFER_SIZE];

static int8_t audio_need_update = 0;
static uint8_t audio_need_stop  = 0;
static uint32_t timeout         = 0;
static uint32_t play_enabled    = 0;
static uint8_t tr_state_changed = 1;
static uint8_t bufclr[2] = {0, 0};
#if COMPRESSION
uint8_t comp_weight = 0;
#endif

static void AUDIO_InitApplication(void);

#if USE_REVERB

static int reverb_life_time;/*1s*/

static void process_reverb (uint8_t idx, uint8_t force)
{
    int16_t *dest, *src;
    int16_t samples_cnt;
    int16_t delay_start_sample = REVERB_DELAY;

    if (force) {
        reverb_life_time = 1000;
    } else {
        reverb_life_time -= AUDIO_BUFFER_MS;
    }

    if (reverb_life_time < 0)
        return;
    
    while (delay_start_sample > AUDIO_OUT_BUFFER_SIZE) {
        delay_start_sample -= AUDIO_OUT_BUFFER_SIZE;
        idx ^= 1;
    }
    samples_cnt = AUDIO_OUT_BUFFER_SIZE - delay_start_sample;
    src = (int16_t *)audio_raw_buf[idx];
    dest = dest + delay_start_sample;

    for (int i = 0; i < samples_cnt; i++) {
        dest[i] += ((int32_t)src[i] * REVERB_DECAY) / REVERB_DECAY_MAX;
    }
    src += samples_cnt;
    dest = (int16_t *)audio_raw_buf[idx ^ 1];
    samples_cnt = AUDIO_OUT_BUFFER_SIZE - samples_cnt;

    for (int i = 0; i < samples_cnt; i++ ) {
        dest[i] += ((int32_t)src[i] * REVERB_DECAY) / REVERB_DECAY_MAX;
    }

}
static int reverb_done ()
{
    return 1;
}
#endif

static void chan_remove_helper (chan_desc_t *desc);
static void chan_invalidate (chan_desc_t *desc);
static void chan_proc_all_to_buf (uint8_t idx);


static void error_handle (void)
{
    for (;;) {}
}

static void ll_stop (void)
{
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
    play_enabled = 0;
}

static void ll_play (void)
{
    BSP_AUDIO_OUT_Play(audio_raw_buf[0], AUDIO_OUT_BUFFER_SIZE * 4);
    play_enabled = 1;
}

static void ll_pause ()
{
    //BSP_AUDIO_OUT_Pause();
}

static void ll_resume ()
{
    //BSP_AUDIO_OUT_Resume();
}

static void ll_ready_empty_handle (struct chan_desc_head *head)
{
    if (music_playing())
        return;

    ll_pause();
    audio_need_stop = 1;

}

static void ll_ready_first_link_handle (struct chan_desc_head *head)
{
    if (music_playing())
        return;

    audio_need_update = 1;
}

void ll_ready_remove_handle (struct chan_desc_head *head, chan_desc_t *rem)
{
    chan_invalidate(rem);
}

static void ll_init ()
{
    memset(&chan_llist_ready, 0, sizeof(chan_llist_ready));
    chan_llist_ready.empty_handle = ll_ready_empty_handle;
    chan_llist_ready.first_link_handle = ll_ready_first_link_handle;
    chan_llist_ready.remove_handle = ll_ready_remove_handle;
    AUDIO_InitApplication();
}

static void
audio_clr_buf_all ()
{
    clear_buf(0);
    clear_buf(1);
}

static void chan_invalidate (chan_desc_t *desc)
{
    memset(desc, 0, sizeof(*desc));
}

static void mute_all ()
{
    ll_stop();
    chan_foreach(&chan_llist_ready, cur) {
        chan_remove_helper(cur);
    }
}


static void audio_state_control ()
{
    if (chan_llist_ready.size) {
        if (tr_state_changed == 1) {
            tr_state_changed = 0;
            timeout = systime;
        }
        if (systime - timeout >= AUDIO_TIMEOUT_MAX) {
            if (play_enabled)
                error_handle();
        }
    }
}

static audio_channel_t *chan_add_helper (Mix_Chunk *chunk, int channel)
{
    chan_desc_t *desc = &channels[channel];
    if (desc->inst.is_playing == 0) {

        desc->inst.is_playing = 1;
        desc->inst.chunk = *chunk;
        chan_link(&chan_llist_ready, desc, 0);
    } else {
        error_handle();
    }

    desc->inst.chunk         = *chunk;
    desc->inst.chunk.alen    /= 2;/*len coming in bytes*/
    desc->inst.id            = channel;
    
    return &desc->inst;
}

static void
chan_remove_helper (chan_desc_t *desc)
{
    chan_unlink(&chan_llist_ready, desc);
}

static void
chan_move_win (chan_desc_t *desc,
                           int size,
                           uint16_t **pbuf,
                           int *psize)
{
    Mix_Chunk *chunk = &desc->inst.chunk;

    if (chan_is_play(desc)== 0) {
        return;
    }

    *pbuf = chunk->abuf;
    *psize = size;

    if (chunk->alen < size)
        *psize = chunk->alen;

    chunk->abuf += *psize;
    chunk->alen -= *psize;
}

static inline uint8_t
chan_try_reject (chan_desc_t *desc)
{
    if (chan_len(desc) <= 0) {
        chan_remove_helper(desc);
        return 1;
    }
    return 0;
}


static inline uint8_t
chan_move_win_all ( int size,
                        uint16_t **pbuf,
                        int *psize)
{
    int i = 0;
    chan_foreach(&chan_llist_ready, cur) {
        chan_move_win(cur, size, &pbuf[i], &psize[i]);
        i++;
    }
    return chan_llist_ready.size;
}

static inline uint8_t
chan_try_reject_all ()
{
    chan_foreach(&chan_llist_ready, cur) {
        if (chan_is_play(cur))
            chan_try_reject(cur);
    }
    return chan_llist_ready.size;
}


static void
chan_mix_all_helper (uint8_t idx)
{
    //BSP_LED_On(LED1);

    tr_state_changed = 1;
    clear_buf(idx);
    chan_proc_all_to_buf(idx);

    //BSP_LED_Off(LED1);
}

void audio_init (void)
{
    ll_init();
    ll_play();
    ll_pause();
    music_init();
}

audio_channel_t *audio_play_channel (Mix_Chunk *chunk, int channel)
{
    audio_channel_t *ch = NULL;
    if (channel >= AUDIO_MAX_CHANS) {
        return NULL;
    }
    HAL_NVIC_DisableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    ch = chan_add_helper(chunk, channel);
    HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    return ch;
}

audio_channel_t *audio_stop_channel (int channel)
{
    return NULL;
}

void audio_pause (int channel)
{
    if (channel >= AUDIO_MAX_CHANS) {
        return;
    }
    HAL_NVIC_DisableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    if (chan_is_play(&channels[channel])) {
        chan_remove_helper(&channels[channel]);
    }
    HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
}
void audio_sdown (int dev)
{

}
int audio_is_playing (int handle)
{
    if (handle >= AUDIO_MAX_CHANS) {
        return 1;
    }
    return chan_is_play(&channels[handle]);
}
void audio_set_pan (int handle, int l, int r)
{
   if (handle >= AUDIO_MAX_CHANS) {
       return;
   }
   if (chan_is_play(&channels[handle])) {
#if USE_STEREO
        chan_vol(&channels[handle]) = ((l + r)) & MAX_VOL;
        channels[handle].left = (uint8_t)l << 1;
        channels[handle].right = (uint8_t)r << 1;
#else
        chan_vol(&channels[handle]) = ((l + r)) & MAX_VOL;
#endif
   }
}

static void AUDIO_InitApplication(void)
{
  BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, 60, AUDIO_SAMPLE_RATE);
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{   
    chan_mix_all_helper(0);
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    chan_mix_all_helper(1);
}

void BSP_AUDIO_OUT_Error_CallBack(void)
{
    mute_all();
    error_handle();
}

void audio_update (void)
{
    HAL_NVIC_DisableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    audio_state_control();
    if (audio_need_stop) {
        audio_clr_buf_all();
        audio_need_stop = 0;
    }
    
    if (audio_need_update == 0)
        goto exit;

    chan_mix_all_helper(0);
    chan_mix_all_helper(1);
    ll_resume();
    audio_need_update = 0;
exit:
    HAL_NVIC_EnableIRQ(AUDIO_OUT_SAIx_DMAx_IRQ);
    music_tickle();
}

static inline uint8_t get_vol_interp (uint8_t vol)
{
    
if (vol > 224) {
        return 0;
     } else if (vol > 196) {
        return 1;
     } else if (vol > 160) {
        return 2;
     } else if (vol > 128) {
        return 3;
     } else if (vol > 96) {
        return 4;
     } else if (vol > 64) {
        return 5;
     } else if (vol > 32) {
        return 6;
     } else {
        return 7;
     }
}

#define AMP(x, vol) (((int16_t)x * vol) / MAX_VOL)
#define COMP(x, vol, comp) (((int16_t)x * vol) / comp)

#define AMP_FLT(x, vol) (int16_t)((float)(x) * (float)vol)

static inline void
mix_to_master_raw1 (uint16_t *dest,
                          uint16_t **ps,
                          uint8_t *vol,
                          int min_size)
{
    int16_t *pdest = (int16_t *)dest;
    int16_t *psrc = (int16_t *)ps[0];
    uint8_t _vol = vol[0];
#if USE_FLOAT
    float vol_flt;
#endif
#if COMPRESSION
    int16_t weight;
#endif

    if (_vol == 0)
        return;

#if (COMPRESSION == 0)

    if (_vol == MAX_VOL) {
        for (int i = 0; i < min_size; i++) {
            pdest[i] = psrc[i] / 2 + pdest[i] / 2;
        }
    } else {
#if USE_FLOAT
        vol_flt = (float)(_vol) / (float)(MAX_VOL);
        for (int i = 0; i < min_size; i++) {
            pdest[i] = AMP_FLT(psrc[i], vol_flt) + pdest[i];
        }
#else
        for (int i = 0; i < min_size; i++) {
            pdest[i] = AMP(psrc[i], _vol) + pdest[i];
        }
#endif
    }
#else

    if (_vol == MAX_VOL) {
        for (int i = 0; i < min_size; i++) {
            pdest[i] = psrc[i] / comp_weight  + pdest[i];
        }
    } else {
        weight = comp_weight * MAX_VOL;
#if USE_FLOAT
        vol_flt = (float)(_vol) / (float)(weight);
        for (int i = 0; i < min_size; i++) {
            pdest[i] = AMP_FLT(psrc[i], vol_flt) + pdest[i];
        }
#else
        for (int i = 0; i < min_size; i++) {
            pdest[i] = COMP(psrc[i], _vol, weight) + pdest[i];
        }
#endif
    }
#endif
}

static inline void
mix_to_master_raw2 (uint16_t *dest,
                          uint16_t **ps,
                          uint8_t *vol,
                          int min_size)
{
    int16_t *pdest = (int16_t *)dest;
    int16_t *psrc0 = (int16_t *)ps[0];
    int16_t *psrc1 = (int16_t *)ps[1];
    uint16_t band_vol = *((uint32_t *)vol);
    int16_t t = 0;

#if (COMPRESSION == 0)
    
    if (band_vol == MAX_2BAND_VOL) {
        for (int i = 0; i < min_size; i++) {
            t = psrc0[i] / 2 + psrc1[i] / 2;
            pdest[i] = t + pdest[i] / 2;
        }
    } else {
        mix_to_master_raw1(dest, ps, vol, min_size);
        mix_to_master_raw1(dest, ps + 1, vol + 1, min_size);
    }
#else
    if (band_vol == MAX_2BAND_VOL) {
        for (int i = 0; i < min_size; i++) {
            t = psrc0[i] / comp_weight + psrc1[i] / comp_weight;
            pdest[i] = t + pdest[i];
        }
    } else {
        mix_to_master_raw1(dest, ps, vol, min_size);
        mix_to_master_raw1(dest, ps + 1, vol + 1, min_size);
    }
#endif

}

static inline void
mix_to_master_raw4 (uint16_t *dest,
                          uint16_t **ps,
                          uint8_t *vol,
                          int min_size)
{
    int16_t *pdest = (int16_t *)dest;
    int16_t *psrc0 = (int16_t *)ps[0];
    int16_t *psrc1 = (int16_t *)ps[1];
    int16_t *psrc2 = (int16_t *)ps[2];
    int16_t *psrc3 = (int16_t *)ps[3];
    uint32_t band_vol = *((uint32_t *)vol);
    int16_t t = 0;
#if (COMPRESSION == 0)

    if (band_vol == MAX_4BAND_VOL) {
        for (int i = 0; i < min_size; i++) {
            t = psrc0[i] / 2 + psrc1[i] / 2;
            t += psrc2[i] / 2 + psrc3[i] / 2;
            pdest[i] = t + pdest[i] / 2;
        }
    } else {
        mix_to_master_raw2(dest, ps, vol, min_size);
        mix_to_master_raw2(dest, ps + 2, vol + 2, min_size);
    }
#else

    if (band_vol == MAX_4BAND_VOL) {
        for (int i = 0; i < min_size; i++) {
            t = psrc0[i] / comp_weight + psrc1[i] / comp_weight;
            t += psrc2[i] / comp_weight + psrc3[i] / comp_weight;
            pdest[i] = t + pdest[i] / comp_weight;
        }
    } else {
        mix_to_master_raw2(dest, ps, vol, min_size);
        mix_to_master_raw2(dest, ps + 2, vol + 2, min_size);
    }

#endif
}

static void chunk_proc_raw_all (uint16_t *dest,
                                     uint16_t **ps,
                                     uint8_t *vol,
                                     int cnt,
                                     int data_size)
{
    switch (cnt) {

        case 1:
            mix_to_master_raw1(dest, ps, vol, data_size);
        break;

        case 2:
            mix_to_master_raw2(dest, ps, vol, data_size);
        break;

        case 3:
            mix_to_master_raw2(dest, ps, vol, data_size);
            mix_to_master_raw1(dest, ps + 2, vol + 2, data_size);
        break;

        case 4:
            mix_to_master_raw4(dest, ps, vol, data_size);
        break;

        case 5:
            mix_to_master_raw4(dest, ps, vol, data_size);
            mix_to_master_raw1(dest, ps + 4, vol + 4, data_size);
        break;

        case 6:
            mix_to_master_raw4(dest, ps, vol, data_size);
            mix_to_master_raw2(dest, ps + 4, vol + 4, data_size);
        break;

        case 7:
            mix_to_master_raw4(dest, ps, vol, data_size);
            mix_to_master_raw2(dest, ps + 4, vol + 4, data_size);
            mix_to_master_raw1(dest, ps + 6, vol + 6, data_size);
        break;

        case 8:
            mix_to_master_raw4(dest, ps, vol, data_size);
            mix_to_master_raw4(dest, ps + 4, vol + 4, data_size);
        break;

        default: error_handle();
    }
}

static uint8_t
chan_to_vol_arr (uint8_t *vol)
{
    int16_t i = 0;
    Mix_Chunk *chunk;
    chan_foreach(&chan_llist_ready, cur) {
        chunk = &cur->inst.chunk;
        vol[i] = chunk->volume;
        i++;
    }
    return chan_llist_ready.size;
}

static uint8_t
chan_to_buf_arr (uint16_t **ps)
{
    int16_t i = 0;
    Mix_Chunk *chunk;
    chan_foreach(&chan_llist_ready, cur) {
        chunk = &cur->inst.chunk;
        ps[i] = chunk->abuf;
        i++;
    }
    return chan_llist_ready.size;
} 


static void chan_proc_raw_all_ex (uint16_t *dest)
{
    int32_t size;
    uint16_t *ps;
    uint8_t vol;
    chan_foreach(&chan_llist_ready, cur) {
        chan_move_win(cur, AUDIO_OUT_BUFFER_SIZE, &ps, &size);
        vol = chan_vol(cur);
        mix_to_master_raw1(dest, &ps, &vol, size);
    }
}

static inline void
mix_mus_to_master (uint16_t *dest,
                          uint16_t **ps,
                          uint8_t *vol,
                          int min_size)
{
    int16_t *pdest = (int16_t *)dest;
    int16_t *psrc = (int16_t *)ps[0];
#if USE_FLOAT
    float vol_flt;
#endif
    if (vol[0] == 0)
        return;
 
    if (vol[0] == MAX_VOL) {
        for (int i = 0; i < min_size; i++) {
            pdest[i] = psrc[i] / 2 + pdest[i] / 2;
        }
    } else {
#if USE_FLOAT
        vol_flt = (float)vol[0] / (float)MAX_VOL;
        for (int i = 0; i < min_size; i++) {  
            pdest[i] = AMP_FLT(psrc[i] / 2, vol_flt) + pdest[i] / 2;
        }
#else
        for (int i = 0; i < min_size; i++) {  
            pdest[i] = AMP(psrc[i] / 2, vol[0]) + pdest[i] / 2;
        }
#endif
    }
}


static void mus_play (uint8_t idx)
{
    uint16_t *dest = audio_raw_buf[idx];
    int32_t size = 0;
    uint8_t vol = music_get_volume() << 1;
    uint16_t *buf = (uint16_t *)music_get_next_chunk(&size);
    if (buf == NULL)
        return;

    mix_mus_to_master(dest, &buf, &vol, size);
    bufclr[idx] = 0;
}

static void chan_proc_all_to_buf (uint8_t idx)
{
    uint16_t *ps[AUDIO_MAX_CHANS];
    uint8_t  vol[AUDIO_MAX_CHANS];
    int32_t psize[AUDIO_MAX_CHANS];
    int32_t size = 0;
    uint16_t *dest = audio_raw_buf[idx];
#if COMPRESSION
    comp_weight = chan_llist_ready.size + 2;
#endif
#if 1
    while (chan_llist_ready.size) {

        bufclr[idx] = 0;
        if (chan_try_reject_all() == 0) {
            break;
        }

        chan_proc_raw_all_ex(dest);
        break;

        if (chan_len(chan_llist_ready.first) < AUDIO_OUT_BUFFER_SIZE) {
            chan_proc_raw_all_ex(dest);
        }

        chan_move_win_all(AUDIO_OUT_BUFFER_SIZE, ps, psize);
        chan_to_vol_arr(vol);

        chunk_proc_raw_all(dest,
                ps,
                vol,
                chan_llist_ready.size,
                AUDIO_OUT_BUFFER_SIZE);

        break;
    }
#else
    if (chan_try_reject_all()) {
        chan_proc_raw_all_ex(dest);
    }
#endif
    mus_play(idx);
}

static void clear_buf (uint8_t idx)
{
    uint32_t *p_buf = (uint32_t *)audio_raw_buf[idx];
    if (bufclr[idx])
        return;

    for (int i = 0; i < AUDIO_OUT_BUFFER_SIZE / 2; i++) {
        p_buf[i] = 0;
    }
    bufclr[idx] = 1;
}


void audio_resume (void)
{
    if (chan_llist_ready.size)
        return;
    audio_need_update = 1;
}

/*External*/
/*return : sound num*/

#define SND_NUM_CACHE_MAX 128
#define SND_LUMP_NAME_LENGTH 8

typedef struct {
    char name[SND_LUMP_NAME_LENGTH + 1];
    int size;
    int16_t lumpnum;    
} snd_cache_t;

static snd_cache_t snd_num_cache[SND_NUM_CACHE_MAX];
static uint16_t last_free_snd_cache_slot = 0;

static const char *snd_dir_path =
"doom/sound/";


#define GET_WAV_PATH(buf, path, name) \
    snprintf(buf, sizeof(buf), "%s%s.WAV", path, name);

static int
snd_ext_alloc_slot ()
{
    for (int i = 0; i < SND_NUM_CACHE_MAX; i++) {
        if (snd_num_cache[i].size == 0) {
            return i;
        }
    }
    return -1;
}

static int
snd_ext_get_slot (int lumnum)
{
    for (int i = 0; i < SND_NUM_CACHE_MAX; i++) {
        if (snd_num_cache[i].lumpnum == lumnum) {
            return i;
        }
    }
    return -1;
}


static void
snd_cache_set_name (int slot, char *name)
{
    strncpy(snd_num_cache[slot].name, name, SND_LUMP_NAME_LENGTH);
    snd_num_cache[slot].name[SND_LUMP_NAME_LENGTH] = 0;
}

int
search_ext_sound (char *_name, int num)
{
    FRESULT fr;     /* Return value */
    int cache_num = -1;
    static FIL file;

    char path[64];
    char name[SND_LUMP_NAME_LENGTH + 1] = {[SND_LUMP_NAME_LENGTH] = 0};

    cache_num = snd_ext_get_slot(num);
    if (cache_num >= 0) {
        return cache_num;
    }

    strncpy(name, _name, SND_LUMP_NAME_LENGTH);
    
    GET_WAV_PATH(path, snd_dir_path, name);

    fr = f_open(&file, path, FA_READ);

    if (fr != FR_OK) {
        return cache_num;
    }
    cache_num = snd_ext_alloc_slot();
    if (cache_num < 0) {
        error_handle();
    }
    snd_cache_set_name(cache_num, name);
    snd_num_cache[cache_num].lumpnum = num;
    snd_num_cache[cache_num].size = f_size(&file);
    f_close(&file);
    return cache_num;
}

int
get_ext_snd_size (int num)
{
    if (num >= SND_NUM_CACHE_MAX) {
        return -1;
    }
    if (snd_num_cache[num].size > 0) {
        return snd_num_cache[num].size;
    }
    error_handle();
}

int
cache_ext_sound (int num, uint8_t *dest, int size)
{
    char path[64];
    FRESULT fr;
    int fsize = 0;
    uint32_t btr;
    static FIL file;

    GET_WAV_PATH(path, snd_dir_path, snd_num_cache[num].name);
    fr = f_open(&file, path, FA_READ);
    if (fr != FR_OK) {
        return -1;
    }

    fr = f_read(&file, dest, size, &btr);
    if ((fr != FR_OK) || (btr != size)) {
        error_handle();
    }
    
    f_close(&file);
    return size;
}


