#include "input_main.h"
#include "main.h"
#include "stm32f769i_discovery_ts.h"
#include <stdlib.h>
#include "doomkeys.h"
#include "d_event.h"
#include "i_timer.h"

#define TS_DEF_CD_COUNT 0
touch_state_t touch_state;
static TS_StateTypeDef  TS_State = {0};

static uint8_t touch_state_tbl[4][2];
static uint8_t ts_state = TS_IDLE;
static uint8_t ts_cd_count = 0;
uint8_t int_received = 0;
uint32_t int_timestamp = 0;

void touch_state_tbl_init (void)
{
    touch_state_tbl[TS_IDLE][TS_PRESS_ON] = TS_CLICK;
    touch_state_tbl[TS_IDLE][TS_PRESS_OFF] = TS_IDLE;
    touch_state_tbl[TS_CLICK][TS_PRESS_ON] = TS_PRESSED;
    touch_state_tbl[TS_CLICK][TS_PRESS_OFF] = TS_RELEASED;
    touch_state_tbl[TS_PRESSED][TS_PRESS_ON] = TS_PRESSED;
    touch_state_tbl[TS_PRESSED][TS_PRESS_OFF] = TS_RELEASED;
    touch_state_tbl[TS_RELEASED][TS_PRESS_ON] = TS_IDLE;
    touch_state_tbl[TS_RELEASED][TS_PRESS_OFF] = TS_IDLE;
}

void touch_init (void)
{
    BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
    touch_state_tbl_init();
}

extern uint32_t systime;
void touch_main (void)
{
    touch_read ();
}

static inline void touch_clear ()
{
    touch_state.status = TOUCH_IDLE;
    memset(&TS_State, 0, sizeof(TS_State));
}

void touch_read (void)
{
    touch_clear();
    uint32_t ts_status = BSP_TS_GetState(&TS_State);
    uint8_t state = 0;
    state = touch_state_tbl[ts_state][TS_State.touchDetected ? TS_PRESS_ON : TS_PRESS_OFF];
    switch (state) {
        case TS_IDLE:
            if (ts_cd_count) {
                ts_cd_count--;
                return;
            }
            break;
        case TS_PRESSED:
            break;
        case TS_CLICK:
            touch_state.status = TOUCH_PRESSED;
            touch_state.x = TS_State.touchX[0];
            touch_state.y = TS_State.touchY[0];
            break;
        case TS_RELEASED:
            touch_state.status = TOUCH_RELEASED;
            ts_cd_count = TS_DEF_CD_COUNT;
            break;
        default:
            break;
    };
    ts_state = state;
}

uint8_t touch_read_temperature (void)
{
    return (uint8_t)(((float)30.0f * 3.0f) / 75.1f);
}

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

int usemouse = 0;


/*---1--- 2--- 3--- 4---*/
/*
--1   up      ent    esc   sr
    |
--2  left     sl     use     right
    |
--3  down  fire  wpn r   map
*/
static const int key_zones[2][3] =
{
        {200, 400, 600},
        {160, 320, 0},
};


static const uint8_t key_map[3][4] =
{
    [0][0] = KEY_UPARROW,
    [0][1] = KEY_ENTER,
    [0][2] = KEY_ESCAPE,
    [0][3] = KEY_STRAFE_R,
    [1][0] = KEY_LEFTARROW,
    [1][1] = KEY_STRAFE_L,
    [1][2] = KEY_USE,
    [1][3] = KEY_RIGHTARROW,
    [2][0] = KEY_DOWNARROW,
    [2][1] = KEY_FIRE,
    [2][2] = '>',/*weapon right*/
    [2][3] = KEY_TAB,
};

static int key_map_get_key (int x, int y)
{
    int row = 2, col = 3;
    for (int i = 0; i < 3; i++) {
        if (x < key_zones[0][i]) {
            col = i;
            break;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (y < key_zones[1][i]) {
            row = i;
            break;
        }
    }
    return key_map[row][col];
}

extern boolean menuactive;

#define TSENS_SLEEP_TIME 250
#define JOY_FREEZE_TIME 150/*ms*/

static uint32_t joy_freeze_tstamp = 0;
static uint8_t touch_sensor_freeze_cnt = 0;


uint8_t special_toggle = 0;
uint8_t special_event = 0;
uint8_t special_key = 0;
uint8_t lookfly = 0;
uint8_t lookfly_key = 0;

#define PAD_FREEZE_BM   0x1
#define PAD_SPEC_BM     0x2
#define PAD_SPEC_LOOK   0x4
#define PAD_LOOK_CONTROL 0x8
#define PAD_LOOK_UP     0x10
#define PAD_LOOK_LEFT  0x20


#define USE_LOOK 1

static struct pads_map_s {
    uint8_t key;
    uint8_t flags;
    uint8_t hit_cnt;
    uint8_t lo_trig: 4,
            hi_trig: 4;
} pads_map[] =
{
    {KEY_UPARROW,       PAD_LOOK_CONTROL | PAD_LOOK_UP, 0, 0, 0},
    {KEY_DOWNARROW,     PAD_LOOK_CONTROL, 0, 0, 0},
    {KEY_LEFTARROW,     PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    {KEY_RIGHTARROW,    PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    {KEY_TAB,           PAD_FREEZE_BM, 0, 0, 0},
    {KEY_USE,           0, 0, 0, 0},
    {KEY_FIRE,          0, 0, 0, 0},
    {KEY_RSHIFT,        PAD_FREEZE_BM | PAD_SPEC_BM, 0, 0, 0},
    {KEY_STRAFE_L,      0, 0, 0, 0},
#if USE_LOOK
    {'<',               PAD_SPEC_LOOK, 0, 0, 0},
#else
    {'<',               PAD_FREEZE_BM, 0, 0, 0},
#endif
    {KEY_STRAFE_R,      0, 0, 0, 0},
    {'>',               PAD_FREEZE_BM, 0, 0, 0},
    {KEY_ENTER,         0, 0, 0, 0},
    {KEY_ESCAPE,        PAD_FREEZE_BM, 0, 0, 0},
    {0,                 0, 0, 0, 0},
    {0,                 0, 0, 0, 0},
};

static inline int
is_joy_freezed ()
{
    if (I_GetTimeMS() > joy_freeze_tstamp)
        return 0;
    return 1;
}

/*TODO : remove ?*/
#if 0
static inline int8_t
filter_pad (uint8_t i, int8_t action)
{
    if (pads_map[i].lo_trig && pads_map[i].hi_trig) {
        if (action > 0) {
            if (pads_map[i].hit_cnt < pads_map[i].hi_trig) {
                pads_map[i].hit_cnt++;
                action = -1;
            }
        } else {
            if (pads_map[i].hit_cnt >= pads_map[i].lo_trig) {
                pads_map[i].hit_cnt--;
                action = -2;
            } else if (action == -2) {
                action = 0;
                pads_map[i].hit_cnt = 0;
            }
        }
    }
    return action;
}
#endif

static inline void
post_key_up (uint8_t key)
{
    event_t event = {ev_keyup, key, -1, -1, -1};
    D_PostEvent(&event);
}

static inline void
post_event (event_t *event, uint8_t i, int8_t action)
{
    uint8_t key = pads_map[i].key;
    uint8_t control = pads_map[i].flags;
    evtype_t type = ev_keyup;

    //action = filter_pad(i, action);
    if (action) {
        if (action < 0) {
            if (touch_sensor_freeze_cnt)
                touch_sensor_freeze_cnt--;
            return;
        } else if (control & PAD_SPEC_BM) {
            special_toggle = 1 - special_toggle;
            special_event = 1 + special_toggle;
            special_key = key;
            goto skip_post;
        } else if (control & PAD_SPEC_LOOK) {
            lookfly = 1;
            lookfly_key = 0;
            goto skip_post;
        }
        type = ev_keydown;
    } else {
        post_key_up(lookfly_key);
        lookfly_key = 0;
    }
    if (lookfly && (control & PAD_LOOK_CONTROL)) {
        if (!action) {
            post_key_up(key);
        }
        if (control & PAD_LOOK_UP) {
            key = KEY_PGDN;
        } else if (control & PAD_LOOK_LEFT) {
            key = KEY_END;
        } else {
            key = KEY_DEL;
        }
        lookfly_key = key;
        lookfly = 0;
    }

    event->data1 = key;
    event->type = type;
    D_PostEvent(event);
skip_post:
    touch_sensor_freeze_cnt = TSENS_SLEEP_TIME;
    if ((control & PAD_FREEZE_BM) || menuactive) {
        joy_freeze_tstamp = I_GetTimeMS() + JOY_FREEZE_TIME;
    }
}

static inline void
post_special ()
{
    event_t event;
    event.type = ev_keydown;
    event.data1 = special_key;

    if (special_toggle) {
        D_PostEvent(&event);
    } else if (special_event) {
        event.type = ev_keyup;
        special_key = 0;
        D_PostEvent(&event);
    }
}

extern int gamepad_read (int8_t *pads, uint8_t *pad_cnt);
void I_GetEvent (void)
{
    event_t event;
    int8_t joy_pads[16];
    int joy_ret = 0;

    event.data2 = -1;
    event.data3 = -1;
    if (!touch_sensor_freeze_cnt) {
        /*Skip sensor processing while gamepad active*/
        touch_main ();
    }
    if (touch_state.status)
    {
        event.type = (touch_state.status == TOUCH_PRESSED) ? ev_keydown : ev_keyup;
        event.data1 = key_map_get_key(touch_state.x, touch_state.y);
        D_PostEvent (&event);
    } else {
        uint8_t pad_cnt = 0;
        joy_ret = gamepad_read(joy_pads, &pad_cnt);

        if (joy_ret <= 0 || is_joy_freezed()) {
            return;
        }
        for (int i = 0; i < pad_cnt; i++) {
            post_event(&event, i, joy_pads[i]);
        }
    }
    post_special();
}



