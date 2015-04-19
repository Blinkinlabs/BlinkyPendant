#include "WProgram.h"
#include "blinkytile.h"
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

struct systemStep {
    float accX;
    float velocityX;
    float posX;
    int playbackPos;
    float dir;
    float accXavg;
};

#define playbackScale 300

// Data block for debugging
#define currentStepMax 100
systemStep steps[currentStepMax];
int currentStep;



float newAccX, newAccY, newAccZ;
bool newAcc = false;

// watermark generates this interrupt
void readISR()
{
    // TODO: This is slow?
    mma8653.getXYZ(newAccX,newAccY,newAccZ);
    newAcc = true;
} 

void POV::setup(Animation *newAnimation) {

    animation = newAnimation;

    currentStep = 0;
    velocityX = 0;
    posX = 0;

    SampleFilter_init(&filter);

/*
    // Set up FTM1 to act as a timer for our model
    SIM_SCGC6 |= SIM_SCGC6_FTM1;    // Enable FTM1 clock
    FTM0_MODE = FTM_MODE_WPDIS;    // Disable Write Protect

    FTM0_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?
    FTM0_MOD = 0xFFFF;             // Period register
    FTM0_SC |= FTM_SC_CLKS(1) | FTM_SC_PS(1);

    FTM0_MODE |= FTM_MODE_INIT;         // Enable FTM0

    FTM0_SYNC |= 0x80;        // set PWM value update
*/

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


void POV::computeStep(float delta) {

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

    // Only update the position if our velocity is high enough
    float velocityGuardBand = 0;
    if(velocityX > velocityGuardBand || velocityX < velocityGuardBand) {
        posX += velocityX*delta;
    }

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


/*
    // Record the system state
    steps[currentStep].accX = accX;
    steps[currentStep].velocityX = velocityX;
    steps[currentStep].posX = posX;
    steps[currentStep].playbackPos = playbackPos;
    steps[currentStep].dir = dir;
    steps[currentStep].accXavg = accXavg;

    currentStep++;
    if(currentStep > currentStepMax) {
        char dataBuffer[80];

        usb_serial_write("count, accX (m/s^2), velocityX (m/s), posX (m)\n", 47);
        for(int step = 0; step < currentStepMax; step++) {
            int len = snprintf(dataBuffer, 80, "%i,%i,%i,%i,%i,%i,%i\n",
                step,
                (int)(steps[step].accX*1000),
                (int)(steps[step].velocityX*100000),
                (int)(steps[step].posX*100000),
                steps[step].playbackPos,
                (int)(steps[step].dir),
                (int)(steps[step].accXavg)
                );
    
            usb_serial_write(dataBuffer, len);
        }
        currentStep = 0;
    }
*/
}
