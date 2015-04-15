#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "matrix.h"


void count_up_loop(int X, int Y, int Z) {
    const int cols = 4;
    const int rows = 2;

/*
    static int pixel = 0;
    const int slowdown = 3;

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
*/

    const int slowdown = 1;
    const int playbackMax = rows*cols*slowdown;
    static int playbackPos = 0;

#define band 2
#define filterLength 3

    static int last[filterLength];

    for(int i = 0; i < filterLength-1; i++) {
        last[i+1] = last[i];
    }
    last[0] = X;


    int filtered = 0;
    for(int i = 0; i < filterLength-1; i++) {
        filtered += last[i+1] - last[i];
    }

    if( filtered> band) {
        playbackPos = (playbackPos + 1)%playbackMax;
/*
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,128,0);
            }
        }
*/
    }
    
    else if (filtered < -band) {
        playbackPos = playbackPos - 1;
        if(playbackPos < 0) {playbackPos = playbackMax - 1;}
/*
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,0,128);
            }
        }
*/
    }
    else {
        playbackPos = 0;
/*
        for (uint16_t col = 0; col < cols; col++) {
            for (uint16_t row = 0; row < rows; row++) {
                setPixel(col,row,0,0,0);
            }
        }
*/
    }


    for (uint16_t col = 0; col < cols; col++) {
        for (uint16_t row = 0; row < rows; row++) {
            if((playbackPos/slowdown) > (row*cols + col)) {
                setPixel(col,row,100,0,0);
            }
            else if((playbackPos/slowdown) == (row*cols + col)) {
                setPixel(col, row, 0,0,255);
            }
            else {
                setPixel(col,row,0,100,0);
            }
        }
    }

}
