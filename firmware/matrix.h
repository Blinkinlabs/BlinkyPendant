/*
 * DMA Matrix Driver
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

#ifndef MATRIX_H
#define MATRIX_H

#include "WProgram.h"
#include "pins_arduino.h"
#include "blinkytile.h"

//Display Geometry
#define BIT_DEPTH 11       // Color bits per channel (Note: input is always 8 bit)

// Output assignments
// Note: These can't be changed arbitrarily- the GPIOs are actually
// referred to in the library by their port assignments.
#define LED_DAT  11   // Port C, output 6
#define LED_CLK  13   // Port C, output 5
#define LED_STB  21   // Port D, output 6
#define LED_OE   33   // FTM0 channel 1 / Port A, output 4
#define S0        6   // Port D, output 4
#define S1       20   // Port D, output 5

// Set up the matrix and start running it's display loop
extern void matrixSetup();

// Change the system brightness
// @param brightness Display brightness scale, from 0 (off) to 1 (fully on)
extern void setBrightness(float brightness);

// Update a single pixel in the array
// @param column int Pixel column (0 to LED_COLS - 1)
// @param row int Pixel row (0 to LED_ROWS - 1)
// @param value uint8_t New value for the pixel (0 - 255)
extern void setPixel(int column, int row, uint8_t value);

// Update the matrix using the data in the Pixels[] array
extern void show();

// Get the display pixel buffer
// @return Pointer to the pixel display buffer, a uint8_t array of size
// LED_ROWS*LED_COLS
extern uint8_t* getPixels();

// The display is double-buffered internally. This function returns
// true if there is already an update waiting.
extern bool bufferWaiting();

#endif
