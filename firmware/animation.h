#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdint.h>
#include "matrix.h"

#define ENCODING_RGB24       0
#define ENCODING_RGB565_RLE  1
#define ENCODING_INDEXED     2
#define ENCODING_INDEXED_RLE 3

class Animation {
 private:
  int ledCount;               // Number of LEDs in the strip (max 254)
  int frameCount;            // Number of frames in this animation (max 65535)
  uint8_t encoding;               // Encoding type
  uint8_t* frameData;             // Pointer to the begining of the frame data
  
  int frameIndex;            // Current animation frame
  uint8_t* currentFrameData;      // Pointer to the current position in the frame data

  int colorTableEntries;      // Number of entries in the color table, minus 1 (max 255)
  Pixel colorTable[256];        // Pointer to color table, if used by the encoder

  void drawRgb24(Pixel* pixels);
  void drawRgb16_RLE(Pixel* pixels);
  void drawIndexed(Pixel* pixels);
  void drawIndexed_RLE(Pixel* pixels);

 public:
  // Initialize the animation with no data. This is intended for the case
  // where the animation will be re-initialized from a memory structure in ROM
  // after the sketch starts.
  Animation();

  // Initialize the animation
  // @param frameCount Number of frames in this animation
  // @param frameData Pointer to the frame data. Format of this data is encoding-specficic
  // @param encoding Method used to encode the animation data
  // @param ledCount Number of LEDs in the strip
  Animation(uint16_t frameCount,
            const uint8_t* frameData,
            const uint8_t encoding,
            const uint8_t ledCount);

  // Re-initialize the animation with new information
  // @param frameCount Number of frames in this animation
  // @param frameData Pointer to the frame data. Format of this data is encoding-specficic
  // @param encoding Method used to encode the animation data
  // @param ledCount Number of LEDs in the strip
  void init(uint16_t frameCount,
            const uint8_t* frameData,
            const uint8_t encoding,
            const uint8_t ledCount);
 
  // Reset the animation, causing it to start over from frame 0.
  void reset();
  
  // Draw the next frame of the animation
  // @param strip[] LED strip to draw to.
  void draw(Pixel* pixels);
};

#endif

