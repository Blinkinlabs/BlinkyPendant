/*
 * LED Animation loader/player
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

#include "animation.h"
//#include "matrix.h"
#include "mk20dn64.h"
 #include "blinkytile.h"

void Animation::init() {
//    uint8_t buffer[ANIMATION_HEADER_LENGTH];

    ledCount = 0;
    frameCount = 0;
    speed = 10;
    type = 0;
}


void Animation::getFrame(uint32_t frame, uint8_t* buffer) {
    int readLength = ledCount;
    if(readLength > LED_COUNT)
        readLength = LED_COUNT;
}


bool Animations::isInitialized() {
    return initialized;
}

void Animations::begin() {
    initialized = false;

    // Look through the file storage, and make an animation for any animation files
    animationCount = 0;

    initialized = true;
}

uint32_t Animations::getCount() {
    return animationCount;
}

Animation* Animations::getAnimation(uint32_t animation) {
    return &(animations[animation]);
}
