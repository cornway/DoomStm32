#include "audio_main.h"
#include "ff.h"
#include <stdio.h>

#define USE_QSPI 0

extern void audio_resume (void);
extern int32_t systime;

static void error_handle (void)
{

}

#define N(x) (sizeof(x) / sizeof(x[0]))

static const char mus_dir_path[] =
"/doom/music";

#if 0
static const char *mus_names[] = 
{
    "song_1.wav",
    "song_2.wav",
    "song_3.wav",
    "song_4.wav",
    "song_5.wav",
    "song_6.wav",
    "song_7.wav",
    "song_8.wav",
    "song_9.wav",
    "song_10.wav",
    "song_11.wav",
    "song_12.wav",
    "song_13.wav",
    "song_14.wav",
    "song_15.wav",
    "song_16.wav",
    "song_17.wav",
    "song_18.wav",
};

#define MUS_SONGS_MAX (N(mus_names))

#endif

#define MUS_BUF_GUARD_MS (AUDIO_MS_TO_SIZE(AUDIO_SAMPLE_RATE, AUDIO_OUT_BUFFER_SIZE) / 2)
#define MUS_CHK_GUARD(mus) ((systime - (mus)->last_upd_tsf) > MUS_BUF_GUARD_MS)

#define MUS_BUF_LEN_MS 1000

#define MUS_RAM_BUF_SIZE AUDIO_OUT_BUFFER_SIZE * 10

static snd_sample_t mus_ram_buf[2][MUS_RAM_BUF_SIZE];

struct song {
    int16_t num;
    char path[64];
    FIL *file;
    int32_t rem;
    int32_t rem_buf;
    snd_sample_t *buf;
    wave_t info;
    uint8_t ready;
    uint32_t mem_offset;
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
    int8_t buf_in_use_idx;
    mus_repeat_t repeat;
    mus_state_t state;
    int32_t last_upd_tsf;
#if USE_QSPI
    uint32_t qspi_mem;
#endif
} mus;


#if !USE_QSPI
static void song_fill_next_buf (struct song *song, struct mus *mus);
#endif

FIL song_file;
struct mus mus;
struct song song_cur;

static void song_reset (struct song *song)
{
    song->num       = -1;
    song->file      = NULL;
    song->buf       = NULL;
    song->rem       = 0;
    song->rem_buf   = 0;
    song->ready     = 0;
    song->mem_offset = 0;
    memset(&song->info, 0, sizeof(song->info));
    memset(song->path, 0, sizeof(song->path));
}

static void mus_reset (struct mus *mus)
{
    mus->volume = 0;
    mus->buf_in_use_idx = 0;
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
    int32_t cnt = song->rem / sizeof(mus_ram_buf) + 1;
    FRESULT res = FR_OK;
    uint32_t btr;
    uint32_t addr = mus->qspi_mem;
#if 0
    qspi_erase(addr, song->rem);
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
    int32_t rem = song->rem;

    dest = mus_ram_buf[mus->buf_in_use_idx ^ 1];

    if (rem > MUS_RAM_BUF_SIZE)
        rem = MUS_RAM_BUF_SIZE;
    
    qspi_flash_read((uint8_t *)dest, song->qspi_addr, rem * 2);
    song->qspi_addr += rem;
    
}

#endif /*USE_QSPI*/

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
    song->rem = song->info.FileSize / sizeof(snd_sample_t);
    song->ready = 1;
    song->mem_offset = sizeof(song->info);
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
    song_fill_next_buf(song, &mus);
#endif
    return 0;
}

int music_play_song_num (int num, int repeat)
{
    audio_resume();   
    return music_play_helper(&song_cur, num - 1, repeat);
}

int music_play_song_name (const char *name)
{
    audio_resume();
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
    mus.volume = vol & MAX_VOL;
    return 0;
}


int music_init (void)
{
    mus_reset(&mus);
    song_reset(&song_cur);
    mus.volume = MAX_VOL / 2;
    return 0;
}

uint8_t music_get_volume (void)
{
    return mus.volume;
}

static void song_next_ram (struct mus *mus, struct song *song)
{
    snd_sample_t *ram;
    int32_t rem = song->rem;

    mus->buf_in_use_idx ^= 1;
    ram = mus_ram_buf[mus->buf_in_use_idx];

    if (rem > MUS_RAM_BUF_SIZE) {
        rem = MUS_RAM_BUF_SIZE;
    }
    song->buf = ram;
    song->rem -= rem;
    song->rem_buf = rem;
    mus->state = MUS_UPDATING;
}

static inline snd_sample_t *
song_advance_to_next (struct mus *mus, struct song *song, int32_t *size)
{
    snd_sample_t *buf = NULL;

    if (song->rem_buf <= 0) {
        song_next_ram(mus, song);
    }

    buf = song->buf;
    *size = song->rem_buf;

    if (*size > AUDIO_OUT_BUFFER_SIZE)
        *size = AUDIO_OUT_BUFFER_SIZE;

    song->buf += *size;
    song->rem_buf -= *size;
    return buf;
}

snd_sample_t *music_get_next_chunk (int32_t *size)
{
    if (!mus.volume) {
        return NULL;
    }

    if ((mus.state != MUS_PLAYING) &&
        (mus.state != MUS_UPDATING)) {
        return NULL;
    }
    if (song_cur.rem <= 0) {
        if (mus.repeat != SINGLE_LOOP) {
            mus.state = MUS_STOP;
            return NULL;
        }
        mus.state = MUS_REPEAT;
        return NULL;
    }
    mus.last_upd_tsf = systime;
    return song_advance_to_next(&mus, &song_cur, size);
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
        song_fill_next_buf(&song_cur, &mus);
#endif
        mus.state = MUS_PLAYING;
    } else if (mus.state == MUS_STOP) {

        mus_close_wav(&song_cur);
        mus_reset(&mus);
    } else if (mus.state == MUS_REPEAT) {

        song_repeat(&mus, &song_cur);
    }
}

#if USE_QSPI
#else
static void song_fill_next_buf (struct song *song, struct mus *mus)
{
    snd_sample_t *dest;
    uint32_t btr = 0;
    FRESULT res;
    int32_t rem = song->rem;

    dest = mus_ram_buf[mus->buf_in_use_idx ^ 1];

    if (rem > MUS_RAM_BUF_SIZE)
        rem = MUS_RAM_BUF_SIZE;

    res = f_read(song->file, dest, rem * sizeof(snd_sample_t), &btr);
    if (res != FR_OK) {
        error_handle();
    }
    song->mem_offset += btr / sizeof(snd_sample_t);
}
#endif /*USE_QSPI*/

int music_playing (void)
{
    return (    (mus.state == MUS_PLAYING) || 
                (mus.state == MUS_UPDATING)
            ) && mus.volume ? 1 : 0;
}



