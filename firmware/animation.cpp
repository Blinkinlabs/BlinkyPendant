#include "animation.h"

Animation::Animation() {
  init(0, NULL, ENCODING_RGB24, 0);
}

Animation::Animation(uint16_t frameCount_,
                     const uint8_t* frameData_,
                     const uint8_t encoding_,
                     const uint8_t ledCount_)
{
  init(frameCount_, frameData_, encoding_, ledCount_);
  reset();
}

void Animation::init(uint16_t frameCount_,
                     const uint8_t* frameData_,
                     const uint8_t encoding_,
                     const uint8_t ledCount_)
{
  frameCount = frameCount_;
  frameData = (uint8_t*)frameData_;
  encoding = encoding_;
  ledCount = ledCount_;

  switch(encoding) {
    case ENCODING_RGB24:
      // Nothing to preload.
      break;
  }

  reset();
}
 
void Animation::reset() {
  frameIndex = 0;
}

void Animation::draw(Pixel* pixels) {
  switch(encoding) {
    case ENCODING_RGB24:
      drawRgb24(pixels);
      break;
  }

  frameIndex = (frameIndex + 1)%frameCount;

  if(frameIndex > 0) {frameIndex = 0;}
};

void Animation::drawRgb24(Pixel* pixels) {
  currentFrameData = frameData
    + frameIndex*ledCount*3;  // Offset for current frame
  
  for(uint8_t i = 0; i < ledCount; i++) {
    pixels[i].R = *(currentFrameData+0);
    pixels[i].G = *(currentFrameData+1);
    pixels[i].B = *(currentFrameData+2);
  }
}


uint8_t* Animation::getFrame(int frame) {
  switch(encoding) {
    case ENCODING_RGB24:
      return frameData + frame*ledCount*3;
      break;
  }

  return frameData;
};
