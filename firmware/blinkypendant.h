/*
 * Peripherals for the BlinkyTile controller
 *
 * Copyright (c) 2014 Matt Mets
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BLINKYPENDANT_H
#define BLINKYPENDANT_H

#include "buttons.h"

#define LED_COLS 5          // Number of columns that the LED matrix has
#define LED_ROWS 2          // Number of rows that the LED matrix has

#define LED_COUNT           (LED_COLS*LED_ROWS)  // Number of LEDs we are controlling
#define BYTES_PER_PIXEL     3

#define BUTTON_COUNT        1   // One input button
#define BUTTON_A            0   // First button

#define BUTTON_A_PIN        3   // Button A: Port A3
#define ACCELEROMETER_INT   9   // Accelerometer input, PTC3. Note this interrupt bank needs to be set to low priority.


#define DISPLAYMODE_POV     10  // POV mode- use acelerometer to display image
#define DISPLAYMODE_TIMED   11  // Timed mode- play back at the pattern speed


// Fadecandy interface defines (stubs)
#define LUT_CH_SIZE             1
#define LUT_TOTAL_SIZE          1
#define PIXELS_PER_PACKET        1  // 63 / 3
#define LUTENTRIES_PER_PACKET    1
#define PACKETS_PER_FRAME        1  // 170 / 21
#define PACKETS_PER_LUT          1  // originally 25


// Initialize the board hardware (buttons, status led, LED control pins)
extern void initBoard();

extern void setupWatchdog();

extern void dfu_reboot();

// Refresh the watchdog, so that the board doesn't reset
static inline void watchdog_refresh(void)
{
    __disable_irq();
    WDOG_REFRESH = 0xA602;
    WDOG_REFRESH = 0xB480;
    __enable_irq();
}

#endif

