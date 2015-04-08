/*
 * Fadecandy Firmware
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
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
#include "fc_usb.h"
#include "fc_defs.h"
#include "HardwareSerial.h"
#include "arm_math.h"

#include "matrix.h"
#include "flash.h"

// USB data buffers
static fcBuffers buffers;
fcLinearLUT fcBuffers::lutCurrent;


// Reserved RAM area for signalling entry to bootloader
extern uint32_t boot_token;

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

extern "C" int usb_rx_handler(usb_packet_t *packet)
{
    // USB packet interrupt handler. Invoked by the ISR dispatch code in usb_dev.c
    return buffers.handleUSB(packet);
}



extern "C" int main()
{
    matrixSetup();
    flashSetup();

    int pixelPosition = 0;

    int R = 0xFFF;
    int G = 0xFFF;
    int B = 0xFFF;

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();

        // Draw some sort of colors on the matrix
        for(int i = 0; i < LED_ROWS*LED_COLS; i++) {
            if(i == pixelPosition/1000) {
                Pixels[i].R = R;
                Pixels[i].G = G;
                Pixels[i].B = B;
            }
            else {
                Pixels[i].R = 0x000;
                Pixels[i].G = 0x000;
                Pixels[i].B = 0x000;
            }
        }

        // And tell the matrix to re-load
        updateMatrix();

        pixelPosition += 1;
        while(pixelPosition >= LED_ROWS*LED_COLS*1000) {
            pixelPosition -= LED_ROWS*LED_COLS*1000;
        }
    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
