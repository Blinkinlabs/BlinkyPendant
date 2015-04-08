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

#define LED_COUNT           170  // Number of LEDs we are controlling
//#define LED_COUNT           40 // Number of LEDs we are controlling
#define BYTES_PER_PIXEL     3

#define BUTTON_COUNT        2   // Two input buttons
#define BUTTON_A            0   // First button
#define BUTTON_B            1   // First button

#define BUTTON_A_PIN        5   // Button A: Port D7
#define BUTTON_B_PIN        20  // Button B: Port D5
#define STATUS_LED_PIN      21  // Status LED: Port D6
#define POWER_ENABLE_PIN    6   // Output power enable resistor: Port D4
#define ADDRESS_PIN         9   // Address program pin: Port C3
#define DATA_PIN            10  // Data output pin: Port C4


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

// Set the brightness of the status LED
// @param value LED brightness 0 = off, 255 = on
extern void setStatusLed(uint8_t value);

// Turn on the LED power output
extern void enableOutputPower();

// Turn off the LED power output
extern void disableOutputPower();

// Refresh the watchdog, so that the board doesn't reset
static inline void watchdog_refresh(void)
{
    __disable_irq();
    WDOG_REFRESH = 0xA602;
    WDOG_REFRESH = 0xB480;
    __enable_irq();
}

#endif

