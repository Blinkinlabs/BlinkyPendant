/*
 * Remote control for the Fadecandy firmware, using the ARM debug interface.
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

#pragma once
#include "arm_kinetis_debug.h"

class FcRemote
{
public:
    FcRemote(ARMKinetisDebug &target) : target(target) {}

    bool installFirmware();
    bool boot();

    // Set remote LED
    bool setLED(bool on);

    // Set control flags
    bool setFlags(uint8_t cflag);

    // test if the accelerometer is present 
    bool testAccelerometer();
    
    bool setLEDData(bool* data);
    bool showColor(int color);
    bool testLEDOutputs();

    // Test that the flash chip works
//    bool testExternalFlash();
    
//    bool testUserButtons();


private:
    ARMKinetisDebug &target;
};

