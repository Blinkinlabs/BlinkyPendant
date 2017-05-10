#include "animation.h"

Animation::Animation() {
  init(0, NULL, ENCODING_RGB24, 0, 0);
}

Animation::Animation(const uint16_t frameCount_,
                     const uint8_t* frameData_,
                     const uint8_t encoding_,
                     const uint8_t ledCount_,
                     const uint16_t frameDelay_)
{
  init(frameCount_, frameData_, encoding_, ledCount_, frameDelay_);
  reset();
}

void Animation::init(const uint16_t frameCount_,
                     const uint8_t* frameData_,
                     const uint8_t encoding_,
                     const uint8_t ledCount_,
                     const uint16_t frameDelay_)
{
  frameCount = frameCount_;
  frameData = (uint8_t*)frameData_;
  encoding = encoding_;
  ledCount = ledCount_;
  frameDelay = frameDelay_;

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

  if(frameIndex > frameCount) {frameIndex = 0;}
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
