#ifndef _INPUT_MAIN_H
#define _INPUT_MAIN_H

#include "main.h"
#include "touch.h"
#include "doomkeys.h"

#define GAMEPAD_USE_FLYLOOK 1

enum {
    TS_IDLE,
    TS_PRESSED,
    TS_CLICK,
    TS_RELEASED,
};

enum {
    TS_PRESS_OFF,
    TS_PRESS_ON,
};

#define PAD_FREQ_LOW   0x1
#define PAD_FUNCTION     0x2
#define PAD_SET_FLYLOOK   0x4
#define PAD_LOOK_CONTROL 0x8
#define PAD_LOOK_UP     0x10
#define PAD_LOOK_DOWN  0x20
#define PAD_LOOK_CENTRE 0x40

struct usb_gamepad_to_kbd_map {
    uint8_t key;
    uint8_t flags;
    uint8_t hit_cnt;
    uint8_t lo_trig: 4,
            hi_trig: 4;
};

void I_GetEvent (void);

int gamepad_read (int8_t *pads);

const struct usb_gamepad_to_kbd_map *
get_gamepad_to_kbd_map (uint8_t *keys_cnt);


#endif /*_INPUT_MAIN_H*/
