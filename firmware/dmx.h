/*
 * Hardware-based DMX engine
 *
 * Outputs DMX on a single channel using 
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

#ifndef DMX_H
#define DMX_H

// Initialize the DMX engine
void dmxSetup();

// Change the system brightness
// @param brightness Display brightness scale, from 0 (off) to 255 (fully on)
void dmxSetBrightness(uint8_t newBrightness);

// Update a single pixel in the array
// @param pixel int Pixel address
// @param r uint8_t New red value for the pixel (0 - 255)
// @param g uint8_t New red value for the pixel (0 - 255)
// @param b uint8_t New red value for the pixel (0 - 255)
void dmxSetPixel(int pixel, uint8_t r, uint8_t g, uint8_t b);


// Emit the DMX signal
void dmxShow();

// Get the display pixel buffer
// @return Pointer to the pixel display buffer, a uint8_t array of size LED_COUNT
uint8_t* dmxGetPixels();

bool dmxWaiting();

#endif
