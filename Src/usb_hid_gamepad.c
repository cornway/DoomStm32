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

#define _UP     0
#define _DOWN   0xff
#define _LEFT   0
#define _RIGHT  0xff

#define _K1     0x1
#define _K2     0x2
#define _K3     0x4
#define _K4     0x8
#define _BL     1
#define _BR     2
#define _TL     4
#define _TR     8
#define _START  1
#define _SELECT 2



static USBH_HandleTypeDef hUSBHost;
int8_t gamepad_connected = 0;
static uint32_t gamepad_data[2];
static int8_t pad_states[14];
uint64_t gamepad_data_ev;
int8_t gamepad_data_ready = 0;

static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id);
USBH_StatusTypeDef USBH_HID_GamepadInit(USBH_HandleTypeDef *phost);


void gamepad_init (void)
{
    USBH_Init(&hUSBHost, USBH_UserProcess, 0);
    USBH_RegisterClass(&hUSBHost, USBH_HID_CLASS);
    USBH_Start(&hUSBHost);
    for (int i = 0; i < sizeof(pad_states); i++) {
        pad_states[i] = -1;
    }
}

void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{

}

static inline void
set_state(num, state) \
{
    if (state) {
         pad_states[num] = 1;
    } else if (pad_states[num] >= 0) {
        pad_states[num]--;
    }
}

int gamepad_read (int8_t *pads)
{
    uint32_t nav_keys;
    uint32_t act_keys;

    uint8_t up_down_pads;
    uint8_t left_right_pads;
    uint8_t key_pads;
    uint8_t shift_pads;
    uint8_t ctls_pads;

    /*Skip each first 'frame'*/
    if (gamepad_data_ready < 2) {
        return -1;
    }
    nav_keys = gamepad_data_ev & 0xffffffff;
    act_keys = (gamepad_data_ev >> 32) & 0xffffffff;

    up_down_pads        = (nav_keys & UP_DOWN_MASK) >> UP_DOWN_GPOS;
    left_right_pads     = (nav_keys & LR_MASK)      >> LR_GPOS;
    key_pads            = (act_keys & KEYS_MASK)    >> KEYS_GPOS;
    shift_pads          = (act_keys & SHIFT_MASK)   >> SHIFT_GPOS;
    ctls_pads           = (act_keys & CTL_MASK)     >> CTL_GPOS;

    set_state(0, up_down_pads == _UP);
    set_state(1, up_down_pads == _DOWN);

    set_state(2, left_right_pads == _LEFT);
    set_state(3, left_right_pads == _RIGHT);

    set_state(4, key_pads & _K1);
    set_state(5, key_pads & _K2);
    set_state(6, key_pads & _K3);
    set_state(7, key_pads & _K4);

    set_state(8, shift_pads & _BL);
    set_state(9, shift_pads & _TL);
    set_state(10, shift_pads & _BR);
    set_state(11, shift_pads & _TR);

    set_state(12, ctls_pads & _START);
    set_state(13, ctls_pads & _SELECT);

    memcpy(pads, pad_states, sizeof(pad_states));
    gamepad_data_ready = 0;
    return sizeof(pad_states);
}


void gamepad_process (void)
{
    USBH_Process(&hUSBHost);
}

static void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
{
  switch (id)
  {
    case HOST_USER_SELECT_CONFIGURATION:
    break;

    case HOST_USER_DISCONNECTION:
        gamepad_connected = 1;
    break;

    case HOST_USER_CLASS_ACTIVE:
        gamepad_connected = 2;
    break;

    case HOST_USER_CONNECTION:      
    break;

    default:
    break;
  }
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


