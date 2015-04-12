#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"


void count_up_loop() {

    static int pixel = 0;
    const int slowdown = 500;

    const int cols = 4;
    const int rows = 2;
   
    for (uint16_t col = 0; col < cols; col++) {
      for (uint16_t row = 0; row < rows; row++) {
        if(pixel < slowdown*rows*cols) {
          if((pixel/slowdown) > (row*cols + col)) {
              setPixel(col,row,0,128,0);
          }
          else if((pixel/slowdown) == (row*cols + col)) {
              setPixel(col, row, 0,0,255);
          }
          else {
              setPixel(col,row,128,0,0);
          }
        }
        else {
          if((rows*cols*2 -1 - pixel/slowdown) > (row*cols + col)) {
              setPixel(col,row,0,128,0);
          }
          else if((rows*cols*2 -1 - pixel/slowdown) == (row*cols + col)) {
              setPixel(col, row, 0,0,255);
          }
          else {
              setPixel(col,row,128,0,0);
          }

        }
      }
    }
    pixel = (pixel+1)%(slowdown*rows*cols*2);
}
