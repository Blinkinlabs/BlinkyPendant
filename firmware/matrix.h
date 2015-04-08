#ifndef MATRIX_H
#define MATRIX_H

#include "WProgram.h"
#include "pins_arduino.h"

//Display Geometry
#define LED_COLS 5        // Number of columns that the LED matrix has (actually the number of LEDs in a group)
#define LED_ROWS 6        // Number of rows that the LED matrix has (actually the number of groups in the line)
#define ROWS_PER_OUTPUT 1  // Output rows per scan (must be 1)
#define BIT_DEPTH 8       // Color bits per channel


// Output assignments
#define LED_DAT  15   // Port C, output 0
#define LED_CLK  22   // Port C, output 1
#define LED_STB  21   // Port D, output 6
#define LED_OE   3    // FTM1 channel 0 / Port B, output 0

// Note: Alternate DAT/CLK assignment are MOSI (11) and SCK (13)

#define S0     2    // Port D, output 0
#define S1    14    // Port D, output 1
#define S2     7    // Port D, output 2
#define S3     8    // Port D, output 3
#define S4     6    // Port D, output 4
#define S5    20    // Port D, output 5

// Set up the matrix and start running the DMA loop
extern void matrixSetup();

// Update the matrix using the data in the Pixels[] array
extern void updateMatrix();

// RGB pixel type
struct pixel {
  uint16_t R;
  uint16_t G;
  uint16_t B;
};

// pixel buffer to draw into
extern pixel Pixels[];

#endif
