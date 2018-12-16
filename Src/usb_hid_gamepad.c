#include "main.h"
#include "usbh_def.h"
#include "usbh_conf.h"
#include "usbh_core.h"
#include "usbh_hid.h"

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
#define M_BR     2
#define M_TL     4
#define M_TR     8
#define M_START  1
#define M_SELECT 2

enum {
    K_UP = 0,
    K_DOWN,
    K_LEFT,
    K_RIGHT,

    K_K1,
    K_K2,
    K_K3,
    K_K4,
    K_BL,
    K_BR,
    K_TL,
    K_TR,
    K_START,
    K_SELECT,
    K_MAX,
};

#define set_pad_state(actions_cnt, keypads, joypad_data, mask)   \
do {                                                \
    if (joypad_data & M##mask) {                    \
         keypads[K##mask] = 1;                      \
    } else if (keypads[K##mask] >= 0) {             \
        keypads[K##mask] = keypads[K##mask] - 1;    \
    }                                               \
    if (keypads[K##mask] >= 0) {                    \
        actions_cnt++;                              \
    }                                               \
} while (0)

static USBH_HandleTypeDef hUSBHost;
static uint32_t gamepad_data[2];
uint64_t gamepad_data_ev;
int8_t gamepad_data_ready = 0;
static int8_t keypads[K_MAX];
static uint8_t needs_handle_cnt;

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

    if (nav_keys) {
        up_down_pads        = (nav_keys & UP_DOWN_MASK) >> UP_DOWN_GPOS;
        left_right_pads     = (nav_keys & LR_MASK)      >> LR_GPOS;
    }
    if (act_keys) {
        key_pads            = (act_keys & KEYS_MASK)    >> KEYS_GPOS;
        shift_pads          = (act_keys & SHIFT_MASK)   >> SHIFT_GPOS;
        ctls_pads           = (act_keys & CTL_MASK)     >> CTL_GPOS;
    }
    set_pad_state(needs_handle_cnt, keypads, up_down_pads, _UP);
    set_pad_state(needs_handle_cnt, keypads, up_down_pads, _DOWN);

    set_pad_state(needs_handle_cnt, keypads, left_right_pads, _LEFT);
    set_pad_state(needs_handle_cnt, keypads, left_right_pads, _RIGHT);

    set_pad_state(needs_handle_cnt, keypads, key_pads, _K1);
    set_pad_state(needs_handle_cnt, keypads, key_pads, _K2);
    set_pad_state(needs_handle_cnt, keypads, key_pads, _K3);
    set_pad_state(needs_handle_cnt, keypads, key_pads, _K4);

    set_pad_state(needs_handle_cnt, keypads, shift_pads, _BL);
    set_pad_state(needs_handle_cnt, keypads, shift_pads, _TL);
    set_pad_state(needs_handle_cnt, keypads, shift_pads, _BR);
    set_pad_state(needs_handle_cnt, keypads, shift_pads, _TR);

    set_pad_state(needs_handle_cnt, keypads, ctls_pads, _START);
    set_pad_state(needs_handle_cnt, keypads, ctls_pads, _SELECT);

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


