/*
 * Fadecandy Firmware - USB Support
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

#include "fc_usb.h"
#include "blinkytile.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include <algorithm>

// USB protocol definitions

#define TYPE_BITS           0xC0
#define FINAL_BIT           0x20
#define INDEX_BITS          0x1F

#define TYPE_FRAMEBUFFER    0x00
#define TYPE_LUT            0x40
#define TYPE_CONFIG         0x80

static usb_packet_t *rx_packet=NULL;

bool fcBuffers::finalizeFrame()
{
    bool newFrame = false;

    // Called in main loop context.
    // Finalize any frames received during the course of this loop iteration,
    // and update the status LED.

    if (flags & CFLAG_NO_ACTIVITY_LED) {
        // LED under manual control
        digitalWriteFast(STATUS_LED_PIN, flags & CFLAG_LED_CONTROL);
    } else {
        // Use the built-in LED as a USB activity indicator.
        digitalWriteFast(STATUS_LED_PIN, handledAnyPacketsThisFrame);
    }
    handledAnyPacketsThisFrame = false;

    if (pendingFinalizeFrame) {
        finalizeFramebuffer();
        pendingFinalizeFrame = false;
        active = true;
        newFrame = true;
    }
/*
    if (pendingFinalizeLUT) {
        finalizeLUT();
        pendingFinalizeLUT = false;
    }
*/

    // Let the USB driver know we may be able to process buffers that were previously deferred
    //process_fc_buffer();

    return newFrame;
}


int fcBuffers::handleUSB()
{
    if (!rx_packet) {
        if (!usb_configuration) return -1;
	rx_packet = usb_rx_no_int(FC_OUT_ENDPOINT);
	if (!rx_packet) return -1;
    }

    unsigned control = rx_packet->buf[0];
    unsigned type = control & TYPE_BITS;
    unsigned final = control & FINAL_BIT;
    unsigned index = control & INDEX_BITS;

    switch (type) {
        case TYPE_FRAMEBUFFER:

            // Framebuffer updates are synchronized; if we're waiting to finalize fbNew,
            // don't accept any new packets until that buffer becomes available.
            if (pendingFinalizeFrame) {
                return false;
            }

            fbNew->store(index, rx_packet);
	    rx_packet = NULL;
            if (final) {
                pendingFinalizeFrame = true;
            }
            break;
/*
        case TYPE_LUT:
            // LUT accesses are not synchronized
            lutNew.store(index, rx_packet);
	    rx_packet = NULL;

            if (final) {
                // Finalize the LUT on the main thread, it's less async than doing it in the ISR.
                pendingFinalizeLUT = true;
            }
            break;
*/
        case TYPE_CONFIG:
            // Config changes take effect immediately.
            flags = rx_packet->buf[1];
            usb_free(rx_packet);
	    rx_packet = NULL;
            break;

        default:
            usb_free(rx_packet);
	    rx_packet = NULL;
            break;
    }

    // Handled this packet
    handledAnyPacketsThisFrame = true;
    return true;
}

void fcBuffers::finalizeFramebuffer()
{
    fcFramebuffer *recycle = fbPrev;
    fbNew->timestamp = millis();
    fbPrev = fbNext;
    fbNext = fbNew;
    fbNew = recycle;
}

bool fcBuffers::isActive()
{
    return active;
}

void fcBuffers::finalizeLUT()
{
    /*
     * To keep LUT lookups super-fast, we copy the LUT into a linear array at this point.
     * LUT changes are intended to be infrequent (initialization or configuration-time only),
     * so this isn't a performance bottleneck.
     *
     * Note the right shift by 1. See lutInterpolate() for an explanation.
     */

    for (unsigned i = 0; i < LUT_TOTAL_SIZE; ++i) {
        lutCurrent.entries[i] = lutNew.entry(i) >> 1;
    }
}
