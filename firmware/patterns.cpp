#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"
#include "usb_serial.h"
#include "animation.h"
#include "ftf2015.h"
#include <cstdio>

#define cols 5
#define rows 2


struct systemStep {
    int accX;
    int jerkX;
    int velocityX;
    int posX;
};


// Data block for debugging
#define currentStepMax 250
systemStep steps[currentStepMax];
int currentStep;




void POV::setup() {
    currentStep = 0;
    reset();
}

void POV::reset() {
    playbackPos = 0;
    accXlast = 0;
    velocityX = 0;
    posX = 0;
}

void POV::computeStep(int accX, int accY, int accZ) {
    int jerkX = accX - accXlast;

    velocityX += accX;
    posX += velocityX;


#define letterbox 10
#define totalFrames (pattern.frameCount + letterbox*2)
#define letterboxing (playbackPos < letterbox || playbackPos > pattern.frameCount + letterbox)
#define framePosition (playbackPos - letterbox)



    if( jerkX > 0) {
        playbackPos--;
    }
    else if (jerkX < 0) {
        playbackPos++;
    }

    if(accX == 127 || accX == -127) {
        reset();
    }


    // Wrap
    while(playbackPos < 0) {
        playbackPos += totalFrames;
    }

    if(playbackPos > totalFrames) {
        playbackPos = (playbackPos + 1) % totalFrames;
    }


    uint8_t* frameData = pattern.getFrame(framePosition);

    for (uint16_t col = 0; col < cols; col++) {
        for (uint16_t row = 0; row < rows; row++) {
            if(!letterboxing) {
                setPixel(col, row,
                    frameData[(row*cols + col)*3 + 0],
                    frameData[(row*cols + col)*3 + 1],
                    frameData[(row*cols + col)*3 + 2]);
            }
            else {
                setPixel(col, row, 0,0,0);
            }
        }
    }

    accXlast = accX;

    // Record the system state
    steps[currentStep].accX = accX;
    steps[currentStep].jerkX = jerkX;
    steps[currentStep].velocityX = velocityX;
    steps[currentStep].posX = posX;

    currentStep++;
    if(currentStep > currentStepMax) {
/*
        char dataBuffer[40];

        for(int step = 0; step < currentStepMax; step++) {
            int len = snprintf(dataBuffer, 40, "%i,%i,%i\n", step,
                steps[step].X,
//                steps[step].Y,
//                steps[step].Z,
                steps[step].playbackPos);
    
            usb_serial_write(dataBuffer, len);
        }
*/
        currentStep = 0;
    }
}
