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
#include "WProgram.h"
#include "usb_serial.h"
#include "usb_dev.h"

#include "blinkypendant.h"
#include "animation.h"
#include "animations.h"
#include "pov.h"
#include "serialloop.h"
#include "buttons.h"
#include "matrix.h"

#include "animations/blinkinlabs.h"


// Button inputs
Buttons userButtons;

// 1-frame animation for showing serial data
extern Animation serialAnimation;

// Bad idea animation
Animation flashAnimation;

uint8_t displayMode;

// Token to signal that the animation loop should be restarted
bool reloadAnimations;

int currentAnimation;

class TimedPlayer {
private:
    Animation* animation;

    uint32_t nextTime;           // Time to display next frame
    int frame;

public:
    void setup();

    void setAnimation(Animation *newAnimation);

    // Calculate the next step based on accelerometer data
    void computeStep();
};

void TimedPlayer::setup() {
}

void TimedPlayer::setAnimation(Animation *newAnimation) {
    animation = newAnimation;
    nextTime = millis();
    frame = 0;
}

void TimedPlayer::computeStep() {
    if(millis() < nextTime) {
        return;
    }

    uint8_t* frameData = animation->getFrame(frame);

    for (uint16_t col = 0; col < LED_COLS; col++) {
        for (uint16_t row = 0; row < LED_ROWS; row++) {
            setPixel(col, row,
                frameData[(row*LED_COLS + col)*3 + 0],
                frameData[(row*LED_COLS + col)*3 + 1],
                frameData[(row*LED_COLS + col)*3 + 2]);
        }
    }

    frame = (frame + 1) % animation->frameCount;
    
    nextTime += animation->frameDelay;
    
    // If we've gotten too far ahead of ourselves, reset the counter
    if(millis() > nextTime) {
        nextTime = millis() + animation->frameDelay;
    }
    
    show();
}


TimedPlayer timedPlayer;

void setAnimation(unsigned int newAnimation) {
    Animation* animation;

    if(getAnimationCount() == 0) {
        animation = &blinkinlabsAnimation;
    }
    else {
        currentAnimation = newAnimation%getAnimationCount();

        loadAnimation(currentAnimation, &flashAnimation);
        animation = &flashAnimation;
    }

    pov.setAnimation(animation);
    timedPlayer.setAnimation(animation);
}

extern "C" int main()
{

    initBoard();

    userButtons.setup();

    serialReset();

    pov.setup();
    timedPlayer.setup();

    matrixSetup();

    reloadAnimations = true;

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();
       
        if(reloadAnimations) {
            // If this was called after writing, the matrix refresh might
            // have been interrupted- call setup again to reset it.
            matrixSetup();

            displayMode = getDisplayMode();

            reloadAnimations = false;
            setAnimation(0);
        }

        userButtons.buttonTask();

        switch(displayMode) {
        case DISPLAYMODE_SERIALLOOP:
            break;

        case DISPLAYMODE_TIMED:
            timedPlayer.computeStep();
            break;

        case DISPLAYMODE_POV:
        default:
            // Use the POV engine to determine the current mode
            pov.computeStep();
            show();
            break;
        }

        // Check for serial data
        if(usb_serial_available() > 0) {
            displayMode = DISPLAYMODE_SERIALLOOP;
            while(usb_serial_available() > 0) {
                serialLoop();
                watchdog_refresh();
            }
        }

        // Finally, check for user button change
        if(userButtons.isPressed()) {
            uint8_t button = userButtons.getPressed();
    
            if(button == BUTTON_A) {
                setAnimation(currentAnimation+1);
            }
        }

    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
