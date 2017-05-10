#include "WProgram.h"
#include "blinkypendant.h"
#include "pov.h"
#include "matrix.h"
#include "mma8653.h"
#include "usb_serial.h"
#include <cstdio>

extern "C" {
#include "SampleFilter.h"
};

#define cols 5
#define rows 2

// accelerometer
MMA8653 mma8653;
POV pov;

SampleFilter filter;


#define playbackScale 120


float newAccX, newAccY, newAccZ;
bool newAcc = false;

// watermark generates this interrupt
void readISR()
{
    // TODO: This is slow?
    mma8653.getXYZ(newAccX,newAccY,newAccZ);
    newAcc = true;
} 

void POV::setAnimation(Animation *newAnimation) {
    animation = newAnimation;
}

void POV::setup() {
    velocityX = 0;
    posX = 0;

    SampleFilter_init(&filter);

    // Set up FTM1 to act as a timer for our model
    SIM_SCGC6 |= SIM_SCGC6_FTM1;    // Enable FTM1 clock
    FTM1_MODE = FTM_MODE_WPDIS;    // Disable Write Protect

    FTM1_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?
    FTM1_MOD = 0xFFFF;             // Period register
    FTM1_SC |= FTM_SC_CLKS(1) | FTM_SC_PS(7);

    FTM1_MODE |= FTM_MODE_INIT;         // Enable FTM0
    FTM1_SYNC |= 0x80;        // set PWM value update

    // Configure the accelerometer interrupt
    pinMode(ACCELEROMETER_INT, INPUT);

    // And turn on an interrupt to get notification when accelerometer data is ready
    attachInterrupt(ACCELEROMETER_INT, readISR, FALLING);

    // Give the interrupt lowest priority
    NVIC_SET_PRIORITY(IRQ_PORTC, 240);
    
    mma8653.setup();
}

static float accXavgLast;
static int dirLast;


void POV::computeStep() {

    // TODO: fix the units here...
    float delta = FTM1_CNT * (0.00000417);
    FTM1_CNT = 0;   // reset the counter

    if(newAcc) {
        accX = newAccX;
        newAcc = false;

        SampleFilter_put(&filter, accX);
        float accXavg = SampleFilter_get(&filter);
        if(accXavg - accXavgLast > 0) {
            dir = 1;
        }
        else if(accXavg - accXavgLast < 0) {
            dir = -1;
        }
        else {
            dir=dirLast;
        }

        accXavgLast = accXavg;
    }


    if(dir != dirLast) {
        velocityX = 0;

        if(dir < 0) {
            posX = 0;
        }
        else {
            posX = animation->frameCount/playbackScale;
        }
    }
    dirLast = dir;

    velocityX += (accX)*delta;

    posX += velocityX*delta;

    int playbackPos = posX*playbackScale;

    if(playbackPos > -1 && playbackPos < animation->frameCount) {
        uint8_t* frameData = animation->getFrame(playbackPos);

        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col, row,
                    frameData[(row*cols + col)*3 + 0],
                    frameData[(row*cols + col)*3 + 1],
                    frameData[(row*cols + col)*3 + 2]);
            }
        }
    }
    else {
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col, row, 0,0,0);
            }
        }
    }
}
