/*
 * main.c
 *
 *  Created on: 13.05.2014
 *      Author: Florian
 */

/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "button.h"
#include "debug.h"
#include "ff.h"
#include "i2c.h"
#include "jpeg.h"
#include "led.h"
#include "main.h"
#include "sdram.h"
#include "spi.h"
#include "images.h"
#include "touch.h"
extern void D_DoomMain (void);

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int d_main(void)
{
    // show title as soon as possible
    //show_image (img_loading);
    touch_init ();
    button_init ();
    D_DoomMain ();
    return 0;
}

uint32_t sec_time = 0;
uint32_t frames_count;
uint32_t fps_prev;
uint32_t msec_per_frame;
uint32_t msec_per_frame_start;
#define MS_PER_SEC 1000
extern uint32_t systime;
void fps_update (void)
{
    if (systime - sec_time * MS_PER_SEC >= MS_PER_SEC) {
        sec_time++;
        fps_prev = frames_count;
        frames_count = 0;
    } else {
        frames_count++;
    }
}

void frame_start ()
{
    msec_per_frame_start = systime;
}

void frame_end ()
{
    msec_per_frame = systime - msec_per_frame_start;
}


/*
 * Show fatal error message and stop in endless loop
 */
void fatal_error (const char* message)
{
	while (1)
	{
	}
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
