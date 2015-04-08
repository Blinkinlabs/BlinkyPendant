#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"

void count_up_loop() {
    static int pixel = 0;
   
    for (uint16_t x = 0; x < LED_COLS; x+=1) {
      for (uint16_t y = 0; y < LED_ROWS; y+=1) {
        if(pixel == ((x+y)%12)) {
            setPixel(x, y, 255);
        }
        else {
            setPixel(x,y,0);
        }
      }
    }
    
    pixel = (pixel+1)%12;
}

