/*
 * Blinky Controller
 *
* Copyright (c) 2014 Matt Mets
 *
 * based on Fadecandy Firmware, Copyright (c) 2013 Micah Elizabeth Scott
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

#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include "fc_usb.h"
#include "arm_math.h"
#include "HardwareSerial.h"
#include "usb_serial.h"

#include "blinkytile.h"
#include "animation.h"
#include "pov.h"
#include "serialloop.h"
#include "buttons.h"
#include "matrix.h"
#include "mma8653.h"

#include "animations/kinetisconnects.h"
#include "animations/freescale.h"
#include "animations/ftf2015.h"


// USB data buffers
static fcBuffers buffers;
fcLinearLUT fcBuffers::lutCurrent;

// accelerometer
MMA8653 mma8653;

POV pov;

// Button inputs
Buttons userButtons;

// built-in animations
#define builtinAnimationCount 3
Animation* builtinAnimations[3] = {
    &ftfAnimation,
    &freescaleAnimation,
    &kinetisAnimation
};

// Reserved RAM area for signalling entry to bootloader
extern uint32_t boot_token;

// Token to signal that the animation loop should be restarted
volatile bool reloadAnimations;

static void dfu_reboot()
{
    // Reboot to the Fadecandy Bootloader
    boot_token = 0x74624346;

    // Short delay to allow the host to receive the response to DFU_DETACH.
    uint32_t deadline = millis() + 10;
    while (millis() < deadline) {
        watchdog_refresh();
    }

    // Detach from USB, and use the watchdog to time out a 10ms USB disconnect.
    __disable_irq();
    USB0_CONTROL = 0;
    while (1);
}

extern "C" int usb_fc_rx_handler()
{
    // USB packet interrupt handler. Invoked by the ISR dispatch code in usb_dev.c
    return buffers.handleUSB();
}


void setupWatchdog() {
    // Change the watchdog timeout because the SPI access is too slow.
    const uint32_t watchdog_timeout = F_BUS / 2;  // 500ms

    WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
    WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
    asm volatile ("nop");
    asm volatile ("nop");
    WDOG_STCTRLH = WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
        WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN | WDOG_STCTRLH_CLKSRC;
    WDOG_PRESC = 0;
    WDOG_TOVALH = (watchdog_timeout >> 16) & 0xFFFF;
    WDOG_TOVALL = (watchdog_timeout)       & 0xFFFF;
}

extern "C" int main()
{
    setupWatchdog();

    initBoard();

    userButtons.setup();

    mma8653.setup();

    int currentAnimation = 0;
    pov.setup(builtinAnimations[currentAnimation]);

    matrixSetup();

    serialReset();

    uint32_t nextTime = 0;        // Time to display next frame

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();
       
        userButtons.buttonTask();

        // Hit the accelerometer every 
        if(millis() > nextTime) {
            float X;
            float Y;
            float Z;
            mma8653.getXYZ(X,Y,Z);
            pov.computeStep(X,Y,Z,(millis()-nextTime)/1000.0);

            nextTime += 1;
    
            // If we've gotten too far ahead of ourselves, reset the counter
            if(millis() > nextTime) {
                nextTime = millis() + 1;
            }
    
            show();
        }

        if(userButtons.isPressed()) {
            uint8_t button = userButtons.getPressed();
    
            if(button == BUTTON_A) {
                currentAnimation = (currentAnimation+1)%builtinAnimationCount;
                pov.setup(builtinAnimations[currentAnimation]);
            }
        }

    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
