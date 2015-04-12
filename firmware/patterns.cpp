#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"

void count_up_loop() {
    static int pixel = 0;
    const int slowdown = 10000;
   
    for (uint16_t x = 0; x < LED_COLS; x+=1) {
      for (uint16_t y = 0; y < LED_ROWS; y+=1) {
        if((pixel/slowdown) == (y*LED_ROWS+x)) {
            setPixel(x, y, 255,255,255);
        }
        else {
            setPixel(x,y,0,0,0);
        }
      }
    }
    
    pixel = (pixel+1)%(slowdown*LED_ROWS*LED_COLS);
}

