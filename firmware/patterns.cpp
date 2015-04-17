#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"
#include "usb_serial.h"
#include "animation.h"
#include "hi.h"
#include <cstdio>

struct systemStep {
    int X;
//    int Y;
//    int Z;
    int playbackPos;
};


// Data block for debugging
#define currentStepMax 250
systemStep steps[currentStepMax];
int currentStep;


void patternsSetup() {
    currentStep = 0;
}


void count_up_loop(int X, int Y, int Z) {
    const int cols = 5;
    const int rows = 2;

    static int playbackPos = 0;     // Position of the playback head within the frame

    static int Xlast;

    int jerk = X - Xlast;

    bool disable = false;

    if( jerk > 0) {
        playbackPos--;
    }
    else if (jerk < 0) {
        playbackPos++;
    }

    if(X == 127 || X == -127) {
        playbackPos = 0;
        disable = true;
    }


    // Wrap
    if(playbackPos > pattern.frameCount) {
        playbackPos = (playbackPos + 1) % pattern.frameCount;
    }
    while(playbackPos < 0) {
        playbackPos += pattern.frameCount;
    }

    for (uint16_t col = 0; col < cols; col++) {
        for (uint16_t row = 0; row < rows; row++) {
            if(!disable) {
                setPixel(col, row,
                    hiData[playbackPos*rows*cols*3 + (row*cols + col)*3 + 0],
                    hiData[playbackPos*rows*cols*3 + (row*cols + col)*3 + 1],
                    hiData[playbackPos*rows*cols*3 + (row*cols + col)*3 + 2]);
            }
            else {
                setPixel(col, row, 0,0,0);
            }
        }
    }

    Xlast = X;

    // Record the system state
    steps[currentStep].X = X;
//    steps[currentStep].Y = Y;
//    steps[currentStep].Z = Z;
    steps[currentStep].playbackPos = playbackPos;


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
