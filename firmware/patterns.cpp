#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"


void count_up_loop(int X, int Y, int Z) {

    static int pixel = 0;
    const int slowdown = 3;

    const int cols = 4;
    const int rows = 2;
   
    for (uint16_t col = 0; col < cols; col++) {
        for (uint16_t row = 0; row < rows; row++) {
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
    }

    pixel = (pixel+1)%(slowdown*rows*cols);

    int band = 2;

    static int last;

    if( X - last> band) {
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,128,0);
            }
        }
    }
    
    else if (X - last < -band) {
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,0,128);
            }
        }
    }
    else {
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,0,0);
            }
        }
    }

    last = X;
}
