#include "audio_main.h"
#include "ff.h"
#include <stdio.h>

#define USE_QSPI 0

extern int32_t systime;

static void error_handle (void)
{
    for (;;) {}
}

#define N(x) (sizeof(x) / sizeof(x[0]))

static const char mus_dir_path[] =
"/doom/music";

#define MUS_BUF_GUARD_MS (AUDIO_MS_TO_SIZE(AUDIO_SAMPLE_RATE, AUDIO_OUT_BUFFER_SIZE) / 2)
#define MUS_CHK_GUARD(mus) ((systime - (mus)->last_upd_tsf) > MUS_BUF_GUARD_MS)

#define MUS_BUF_LEN_MS 1000

#define MUS_RAM_BUF_SIZE AUDIO_OUT_BUFFER_SIZE * 10

static snd_sample_t mus_ram_buf[2][MUS_RAM_BUF_SIZE];

struct song {
    int16_t num;
    char path[64];
    FIL *file;
    int32_t track_rem;
    int32_t rem_buf;
    snd_sample_t *buf;
    wave_t info;
    uint8_t ready;
    uint32_t mem_offset;
    audio_channel_t *channel;
};

typedef enum {
    MUS_IDLE,
    MUS_UPDATING,
    MUS_PLAYING,
    MUS_REPEAT,
    MUS_PAUSE,
    MUS_STOP,
} mus_state_t;

typedef enum {
    NONE,
    SINGLE_LOOP,
} mus_repeat_t;

struct mus {
    uint8_t volume;
    int8_t rd_idx;
    mus_repeat_t repeat;
    mus_state_t state;
    int32_t last_upd_tsf;
#if USE_QSPI
    uint32_t qspi_mem;
#endif
} mus;


#if !USE_QSPI
static void song_update_next_buf (struct song *song, struct mus *mus);
#endif

FIL song_file;
struct mus mus;
struct song song_cur;

static void song_reset (struct song *song)
{
    song->num       = -1;
    song->file      = NULL;
    song->buf       = NULL;
    song->track_rem       = 0;
    song->rem_buf   = 0;
    song->ready     = 0;
    memset(&song->info, 0, sizeof(song->info));
    memset(song->path, 0, sizeof(song->path));
}

static void mus_reset (struct mus *mus)
{
    mus->volume = 0;
    mus->rd_idx = 0;
    mus->repeat = SINGLE_LOOP;
    mus->state = MUS_IDLE;
}

static inline void
song_alloc_file (struct song *song)
{
    song->file = &song_file;
}

#if USE_QSPI

extern int qspi_flash_read (uint8_t *buf, int addr, int size);
extern int qspi_flash_write (uint8_t *buf, int addr, int size);
extern void qspi_erase (int addr, int size);
extern void qspi_erase_all (void);

static void mus_qspi_load_file (struct mus *mus, struct song *song)
{
    int32_t cnt = song->track_rem / sizeof(mus_ram_buf) + 1;
    FRESULT res = FR_OK;
    uint32_t btr;
    uint32_t addr = mus->qspi_mem;
#if 0
    qspi_erase(addr, song->track_rem);
#else
    qspi_erase_all();
#endif
    while (cnt > 0) {
        res = f_read(song->file, &mus_ram_buf[0], sizeof(mus_ram_buf), &btr);
        if (res != FR_OK) {
            break;
        }
        qspi_flash_write((uint8_t *)&mus_ram_buf[0], addr, btr);
        addr += sizeof(mus_ram_buf);
        cnt--;
    }
}

static void mus_qspi_read (struct song *song, struct mus *mus)
{
    snd_sample_t *dest;
    int32_t rem = song->track_rem;

    dest = mus_ram_buf[mus->rd_idx ^ 1];

    if (rem > MUS_RAM_BUF_SIZE)
        rem = MUS_RAM_BUF_SIZE;
    
    qspi_flash_read((uint8_t *)dest, song->qspi_addr, rem * 2);
    song->qspi_addr += rem;
    
}

#endif /*USE_QSPI*/

static void song_setup (struct mus *mus, struct song *song);

static int
chunk_setup (
    struct mus *mus,
    struct song *song,
    Mix_Chunk *chunk
    );

static int music_scan_dir_for_num (int _num, char *name)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    static int songs_max = 0;
    int cnt = 0;
    int num = _num;
    int cnt_max = 256;
    if (songs_max) {
        num = num % songs_max;
    }

    res = f_opendir(&dir, mus_dir_path);
    if (res == FR_OK) {
        for (;;) {
            cnt++;
            if (num) {
                 num--;
            }
            res = f_readdir(&dir, &fno); 
            if (res != FR_OK || fno.fname[0] == 0) {
                songs_max = cnt;
                num = num % cnt;
                cnt = 0;
                f_closedir(&dir);
                f_opendir(&dir, mus_dir_path);
            } else if ((fno.fattrib & AM_DIR) == 0) {
                if (!num) {
                    sprintf(name, "%s", fno.fname);
                    break;
                }
            }
            if (cnt_max) {
                cnt_max--;
            } else {
                return -1;
            }
        }
        f_closedir(&dir);
    } else {
        return -1;
    }
    return 0;
}

static int mus_cplt_hdlr (int complete)
{
    if (complete == 2) {
        if (song_cur.track_rem <=  0) {
            if (mus.repeat) {
                mus.state = MUS_REPEAT;
            }
            return 1;
        }
        if (mus.state == MUS_PAUSE ||
            mus.state == MUS_STOP ||
            mus.state == MUS_REPEAT ||
            !mus.volume) {
            /*This will cause 'fake channel' - it will won't play, but still in 'ready'*/
            return 0;
        }
        song_setup(&mus, &song_cur);
    } else if (complete == 1) {
        if (mus.state == MUS_PAUSE ||
            mus.state == MUS_STOP ||
            mus.state == MUS_REPEAT ||
            !mus.volume) {
            /*Skip this*/
            return 1;
        }
    }
    return 0;
}

static void mus_open_wav (struct song *song, const char *name)
{
    uint32_t btr;
    FRESULT res = FR_OK;
    if (song->file == NULL) {
        song_alloc_file(song);
    }
    snprintf(song->path, sizeof(song->path), "%s/%s", mus_dir_path, name);
    res = f_open(song->file, song->path, FA_READ | FA_OPEN_EXISTING);
    if (res != FR_OK)
        return;
    res = f_read(song->file, &song->info, sizeof(song->info), &btr);
    if (res != FR_OK || btr != sizeof(song->info)) {
        error_handle();
    }
    if (song->info.BitPerSample != 16) {
        error_handle();
    }
    if (song->info.NbrChannels != 2) {
        error_handle();
    }
    if (song->info.SampleRate != AUDIO_SAMPLE_RATE) {
        error_handle();
    }
    song->track_rem = song->info.FileSize / sizeof(snd_sample_t);
    song->ready = 1;
#if USE_QSPI
    mus_qspi_load_file(&mus, song);
#endif
}

static void mus_close_wav (struct song *song)
{
    f_close(song->file);
    song_reset(song);
}

static int music_play_helper (struct song *song, int num, int repeat)
{
    char name[64];
    Mix_Chunk mixchunk;
    if (music_scan_dir_for_num(num, name) < 0) {
        mus.state = MUS_STOP;
        return -1;
    }
#if 0
    num = (num) % MUS_SONGS_MAX;
#endif
    if (mus.state != MUS_IDLE) {
        if ((mus.state == MUS_PLAYING || mus.state == MUS_UPDATING) &&
            num == song->num) { 
            return 0;
        }
        mus_close_wav(song);
    }
#if 0
    mus_open_wav(song, mus_names[num]);
#endif
    mus_open_wav(song, name);
    mus.state = MUS_PLAYING;
    mus.repeat = repeat ? SINGLE_LOOP : NONE;
    song->num = num;
#if USE_QSPI
    mus_qspi_read(song, &mus);
#else
    song_update_next_buf(song, &mus);
#endif
    /*Little hack - force channel reject to update it through callback*/
    mixchunk.alen = 0;
    if (audio_is_playing(AUDIO_MUS_CHAN_START)) {
        audio_stop_channel(AUDIO_MUS_CHAN_START);
    }
    song->channel = audio_play_channel(&mixchunk, AUDIO_MUS_CHAN_START);
    if (!song->channel) {
        error_handle();
    }
    song->channel->complete = mus_cplt_hdlr;
    return 0;
}

uint8_t music_get_volume (void)
{
    return mus.volume;
}

static int
chunk_setup (
    struct mus *mus,
    struct song *song,
    Mix_Chunk *chunk
    )
{
    snd_sample_t *ram;
    int32_t rem = song->track_rem;

    ram = mus_ram_buf[mus->rd_idx];

    if (rem > MUS_RAM_BUF_SIZE) {
        rem = MUS_RAM_BUF_SIZE;
    }
    chunk->abuf = ram;
    chunk->alen = rem;
    chunk->volume = mus->volume;
    return rem;
}

static void song_setup (struct mus *mus, struct song *song)
{
    int32_t rem = song->track_rem;

    if (song->channel) {
        mus->rd_idx ^= 1;
        rem = chunk_setup(mus, song, &song->channel->chunk);
        song->track_rem -= rem;
        mus->state = MUS_UPDATING;
    }
}

static void song_repeat (struct mus *mus, struct song *song)
{
    music_play_helper(song, song->num, mus->repeat);
}

void music_tickle (void)
{
    if (mus.state == MUS_PLAYING) {

    } else if (mus.state == MUS_UPDATING) {
#if USE_QSPI
        mus_qspi_read(&song_cur, &mus);
#else
        song_update_next_buf(&song_cur, &mus);
#endif
        mus.state = MUS_PLAYING;
    } else if (mus.state == MUS_STOP) {

        audio_stop_channel(AUDIO_MUS_CHAN_START);
        mus_close_wav(&song_cur);
        mus_reset(&mus);
    } else if (mus.state == MUS_REPEAT) {

        song_repeat(&mus, &song_cur);
    }
}

#if USE_QSPI
#else
static void song_update_next_buf (struct song *song, struct mus *mus)
{
    snd_sample_t *dest;
    uint32_t btr = 0;
    FRESULT res;
    int32_t rem = song->track_rem;

    dest = mus_ram_buf[mus->rd_idx ^ 1];

    if (rem > MUS_RAM_BUF_SIZE)
        rem = MUS_RAM_BUF_SIZE;

    res = f_read(song->file, dest, rem * sizeof(snd_sample_t), &btr);
    if (res != FR_OK) {
        error_handle();
    }
}
#endif /*USE_QSPI*/

int music_playing (void)
{
    return (    (mus.state == MUS_PLAYING) || 
                (mus.state == MUS_UPDATING)
            ) && mus.volume ? 1 : 0;
}

int music_play_song_num (int num, int repeat)
{
    return music_play_helper(&song_cur, num - 1, repeat);
}

int music_play_song_name (const char *name)
{
    return music_play_helper(&song_cur, 0, 1);
}

int music_pause (void)
{
    if (mus.state == MUS_PLAYING ||
        mus.state == MUS_UPDATING) {
        mus.state = MUS_PAUSE;
        return 0;
    }
    return 1;
}

int music_resume (void)
{
    if (mus.state == MUS_PAUSE) {
        mus.state = MUS_PLAYING;
        return 0;
    }
    return 1;
}

int music_stop (void)
{
    if (mus.state != MUS_IDLE)
        mus.state = MUS_STOP;
    return 0;
}

int music_set_vol (uint8_t vol)
{
    mus.volume = vol;
    return 0;
}


int music_init (void)
{
    mus_reset(&mus);
    song_reset(&song_cur);
    mus.volume = MAX_VOL / 2;
    return 0;
}


