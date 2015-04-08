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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <inttypes.h>

#define ANIMATION_HEADER_LENGTH 16

// Max. number of animations that can be read from the flash (arbitrary)
#define MAX_ANIMATIONS_COUNT 100

class Animation {
  public:
    uint32_t fileNumber;         // File number containing this animation
    uint32_t ledCount;           // Number of LEDs controlled by this animation
    uint32_t frameCount;         // Number of frames in this animation
    uint32_t speed;              // Speed, in ms between frames
    uint32_t type;               // Type, 0x0000 = BGR uncompressed

    // Initialize the animation using the given file number
    void init();

    // Retrieve the animation data for the given frame
    // Reads LED_COUT*BYTES_PER_PIXEL of data.
    // @param frame Animation frame
    // @param buffer Buffer to write the data to
    void getFrame(uint32_t frame, uint8_t* buffer);
};


class Animations {
  private:
    Animation animations[MAX_ANIMATIONS_COUNT];	// Static table of aniimations
    uint32_t animationCount;    // Number of animations in this class

    bool initialized;           // True if initialized correctly

  public:
    // Initialize the animations table
    // @param storage_ Storage container to read from
    void begin();

    // True if the animations table was read correctly from flash
    bool isInitialized();

    // Read the number of animations stored in the flash
    // @return Number of animations stored in the flash
    uint32_t getCount();

    // Get the requested animation
    Animation* getAnimation(uint32_t animation);
};

#endif
