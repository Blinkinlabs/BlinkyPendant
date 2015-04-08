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

#ifndef BLINKYTILE_H
#define BLINKYTILE_H


#define LED_COLS 4          // Number of columns that the LED matrix has
#define LED_ROWS 2          // Number of rows that the LED matrix has

#define LED_COUNT           (LED_COLS*LED_ROWS)  // Number of LEDs we are controlling
#define BYTES_PER_PIXEL     3

#define BUTTON_COUNT        1   // Two input buttons
#define BUTTON_A            0   // First button

#define BUTTON_A_PIN        12   // Button A: Port A3  //TODO


// File types
#define FILETYPE_ANIMATION      0x12


// Fadecandy interface defines
#define LUT_CH_SIZE             257
#define LUT_TOTAL_SIZE          (LUT_CH_SIZE * 3)

// USB packet layout
#define PIXELS_PER_PACKET       21  // 63 / 3
#define LUTENTRIES_PER_PACKET   31
#define PACKETS_PER_FRAME        9  // 170 / 21
#define PACKETS_PER_LUT          1  // originally 25


// Initialize the board hardware (buttons, status led, LED control pins)
extern void initBoard();


// Refresh the watchdog, so that the board doesn't reset
static inline void watchdog_refresh(void)
{
    __disable_irq();
    WDOG_REFRESH = 0xA602;
    WDOG_REFRESH = 0xB480;
    __enable_irq();
}

#endif

