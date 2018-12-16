#include "input_main.h"
#include "main.h"
#include "stm32f769i_discovery_ts.h"
#include <stdlib.h>

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



