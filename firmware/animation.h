#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdint.h>
#include "matrix.h"

#define ENCODING_RGB24       0

class Animation {
 public:
  uint8_t* frameData;             // Pointer to the begining of the frame data
  uint8_t ledCount;               // Number of LEDs in the strip (max 254)
  uint16_t frameCount;            // Number of frames in this animation (max 65535)
  uint16_t frameDelay;            // Delay between frames, in ms (max 65535)

 private:
  uint8_t encoding;               // Encoding type
  
  int frameIndex;                 // Current animation frame
  uint8_t* currentFrameData;      // Pointer to the current position in the frame data

  void drawRgb24(Pixel* pixels);
  void drawRgb16_RLE(Pixel* pixels);

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
  Animation(const uint16_t frameCount,
            const uint8_t* frameData,
            const uint8_t encoding,
            const uint8_t ledCount,
            const uint16_t frameDelay);

  // Re-initialize the animation with new information
  // @param frameCount Number of frames in this animation
  // @param frameData Pointer to the frame data. Format of this data is encoding-specficic
  // @param encoding Method used to encode the animation data
  // @param ledCount Number of LEDs in the strip
  void init(uint16_t frameCount,
            const uint8_t* frameData,
            const uint8_t encoding,
            const uint8_t ledCount,
            const uint16_t frameDelay);
 
  // Reset the animation, causing it to start over from frame 0.
  void reset();
  
  // Draw the next frame of the animation
  // @param strip[] LED strip to draw to.
  void draw(Pixel* pixels);

  uint8_t* getFrame(int frame);
};

#endif

