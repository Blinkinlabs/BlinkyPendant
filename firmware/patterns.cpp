#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"

void count_up_loop() {

    static int pixel = 0;
    const int slowdown = 100;

    const int cols = 4;
    const int rows = 2;
   
    for (uint16_t col = 0; col < cols; col++) {
      for (uint16_t row = 0; row < rows; row++) {
        if((pixel/slowdown) == (row*cols + col)) {
            setPixel(col, row, 255,255,255);
        }
        else {
            setPixel(col,row,0,0,0);
        }
      }
    }
    pixel = (pixel+1)%(slowdown*rows*cols);

/* 
    pixel = (pixel+1)%(slowdown*LED_ROWS*LED_COLS);
    for (uint16_t x = 0; x < LED_COLS; x+=1) {
      for (uint16_t y = 0; y < LED_ROWS; y+=1) {
        if((pixel/slowdown) == (y*LED_ROWS+x)) {
            setPixel(x, y, 0,0,255);
        }
        else {
            setPixel(x,y,0,0,0);
        }
      }
    }
    
    pixel = (pixel+1)%(slowdown*LED_ROWS*LED_COLS);
*/
}
