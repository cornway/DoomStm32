#include "main.h"
#include "usbh_def.h"
#include "usbh_conf.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "input_main.h"

#define UP_DOWN_MASK    0x0000ff00
#define UP_DOWN_GPOS    8
#define LR_MASK         0x000000ff
#define LR_GPOS         0

#define KEYS_MASK       0x0000f000
#define KEYS_GPOS       12
#define SHIFT_MASK      0x000f0000
#define SHIFT_GPOS      16
#define CTL_MASK        0x00f00000
#define CTL_GPOS        20

#define M_UP     0
#define M_DOWN   0xff
#define M_LEFT   0
#define M_RIGHT  0xff

#define M_K1     0x1
#define M_K2     0x2
#define M_K3     0x4
#define M_K4     0x8
#define M_BL     1
#define M_TL     2
#define M_BR     4
#define M_TR     8
#define M_START  1
#define M_SELECT 2

enum {
    K_UP = 0,
    K_DOWN = 1,
    K_LEFT = 2,
    K_RIGHT = 3,

    K_K1 = 4,
    K_K2 = 5,
    K_K3 = 6,
    K_K4 = 7,
    K_BL = 8,
    K_BR = 9,
    K_TL = 10,
    K_TR = 11,
    K_START = 12,
    K_SELECT = 13,
    K_MAX = 14,
};

static const struct usb_gamepad_to_kbd_map gamepad_to_kbd_map[] =
{
    [K_UP]      = {KEY_UPARROW,       PAD_LOOK_CONTROL | PAD_LOOK_UP, 0, 0, 0},
    [K_DOWN]    = {KEY_DOWNARROW,     PAD_LOOK_CONTROL, 0, 0, 0},
    [K_LEFT]    = {KEY_LEFTARROW,     PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    [K_RIGHT]   = {KEY_RIGHTARROW,    PAD_LOOK_CONTROL | PAD_LOOK_LEFT, 0, 0, 0},
    [K_K1]      = {KEY_TAB,           PAD_FREQ_LOW, 0, 0, 0},
    [K_K2]      = {KEY_USE,           0, 0, 0, 0},
    [K_K3]      = {KEY_FIRE,          0, 0, 0, 0},
    [K_K4]      = {KEY_RSHIFT,        PAD_FREQ_LOW | PAD_SPEC_BM, 0, 0, 0},
    [K_BL]      = {KEY_STRAFE_L,      0, 0, 0, 0},
#if GAMEPAD_USE_FLYLOOK
    [K_BR]      = {KEY_WEAPON_ROL,    PAD_SET_FLYLOOK, 0, 0, 0},
#else
    [K_BR]      = {KEY_WEAPON_ROL,    PAD_FREQ_LOW, 0, 0, 0},
#endif
    [K_TL]      = {KEY_STRAFE_R,      0, 0, 0, 0},
    [K_TR]      = {KEY_WEAPON_ROR,    PAD_FREQ_LOW, 0, 0, 0},
    [K_START]   = {KEY_ENTER,         0, 0, 0, 0},
    [K_SELECT]  = {KEY_ESCAPE,        PAD_FREQ_LOW, 0, 0, 0},
};

const struct usb_gamepad_to_kbd_map *
get_gamepad_to_kbd_map (uint8_t *keys_cnt)
{
    *keys_cnt = sizeof(gamepad_to_kbd_map) / sizeof(gamepad_to_kbd_map[0]);
    return gamepad_to_kbd_map;
}

static USBH_HandleTypeDef hUSBHost;
static uint32_t gamepad_data[2];
uint64_t gamepad_data_ev;
int8_t gamepad_data_ready = 0;
static int8_t keypads[K_MAX];
static uint8_t needs_handle_cnt;

static inline int
set_pad_state (int pad_idx, int state)
{
    if (state) {
        keypads[pad_idx] = 1;
    } else if (keypads[pad_idx] >= 0) {
        keypads[pad_idx]--;
    }
    if (keypads[pad_idx]) {
        return 1;
    }
    return 0;
}

static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id);
USBH_StatusTypeDef USBH_HID_GamepadInit(USBH_HandleTypeDef *phost);



void gamepad_init (void)
{
    USBH_Init(&hUSBHost, USBH_UserProcess, 0);
    USBH_RegisterClass(&hUSBHost, USBH_HID_CLASS);
    USBH_Start(&hUSBHost);
    for (int i = 0; i < K_MAX; i++) {
        keypads[i] = -1;
    }
    needs_handle_cnt = 0;
}

void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{

}

int gamepad_read (int8_t *pads)
{
    uint32_t nav_keys;
    uint32_t act_keys;

    uint8_t up_down_pads =0;
    uint8_t left_right_pads = 0;
    uint8_t key_pads = 0;
    uint8_t shift_pads = 0;
    uint8_t ctls_pads = 0;

    /*Skip each first 'frame'*/
    if (gamepad_data_ready < 2) {
        return -1;
    }
    if ((gamepad_data_ev == 0) && (needs_handle_cnt == 0)) {
        return -1;
    }
    nav_keys = gamepad_data_ev & 0xffffffff;
    act_keys = (gamepad_data_ev >> 32) & 0xffffffff;

    up_down_pads        = (nav_keys & UP_DOWN_MASK) >> UP_DOWN_GPOS;
    left_right_pads     = (nav_keys & LR_MASK)      >> LR_GPOS;

    key_pads            = (act_keys & KEYS_MASK)    >> KEYS_GPOS;
    shift_pads          = (act_keys & SHIFT_MASK)   >> SHIFT_GPOS;
    ctls_pads           = (act_keys & CTL_MASK)     >> CTL_GPOS;

    needs_handle_cnt += set_pad_state(K_UP, up_down_pads == M_UP);
    needs_handle_cnt += set_pad_state(K_DOWN, up_down_pads == M_DOWN);

    needs_handle_cnt += set_pad_state(K_LEFT, left_right_pads == M_LEFT);
    needs_handle_cnt += set_pad_state(K_RIGHT, left_right_pads == M_RIGHT);

    needs_handle_cnt += set_pad_state(K_K1, key_pads & M_K1);
    needs_handle_cnt += set_pad_state(K_K2, key_pads & M_K2);
    needs_handle_cnt += set_pad_state(K_K3, key_pads & M_K3);
    needs_handle_cnt += set_pad_state(K_K4, key_pads & M_K4);

    needs_handle_cnt += set_pad_state(K_BL, shift_pads & M_BL);
    needs_handle_cnt += set_pad_state(K_TL, shift_pads & M_TL);
    needs_handle_cnt += set_pad_state(K_BR, shift_pads & M_BR);
    needs_handle_cnt += set_pad_state(K_TR, shift_pads & M_TR);

    needs_handle_cnt += set_pad_state(K_START, ctls_pads & M_START);
    needs_handle_cnt += set_pad_state(K_SELECT, ctls_pads & M_SELECT);

    memcpy(pads, keypads, sizeof(keypads));
    gamepad_data_ready = 0;

    return needs_handle_cnt;
}


void gamepad_process (void)
{
    USBH_Process(&hUSBHost);
}

static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
{

}

USBH_StatusTypeDef USBH_HID_GamepadInit(USBH_HandleTypeDef *phost)
{
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
  
  if(HID_Handle->length > sizeof(gamepad_data))
  {
    HID_Handle->length = sizeof(gamepad_data);
  }
  HID_Handle->pData = (uint8_t *)gamepad_data;
  fifo_init(&HID_Handle->fifo, phost->device.Data, /*HID_QUEUE_SIZE * */sizeof(gamepad_data));

  return USBH_OK;  
}


