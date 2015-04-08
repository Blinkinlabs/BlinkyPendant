#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"

void count_up_loop() {
    static int pixel = 0;
    
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        if(pixel == (i%12)) {
            //dmxSetPixel(i, 255, 255, 255);
        }
        else {
            //dmxSetPixel(i, 0,0,0);
        }
    }
    
    pixel = (pixel+1)%12;
}

