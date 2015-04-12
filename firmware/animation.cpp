#include "Animation.h"

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
    case ENCODING_RGB565_RLE:
      // Nothing to preload.
      break;
    case ENCODING_INDEXED:
    case ENCODING_INDEXED_RLE:
      // Load the color table into memory
      // TODO: Free this memory somewhere?
      colorTableEntries = *(frameData);

      for(int i = 0; i < colorTableEntries; i++) {
        colorTable[i].R = *(frameData + 1 + i*3 + 0);
        colorTable[i].G = *(frameData + 1 + i*3 + 1);
        colorTable[i].B = *(frameData + 1 + i*3 + 2);
      }
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
    case ENCODING_RGB565_RLE:
      drawRgb16_RLE(pixels);
      break;
    case ENCODING_INDEXED:
      drawIndexed(pixels);
      break;
    case ENCODING_INDEXED_RLE:
      drawIndexed_RLE(pixels);
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
    pixels[i].R = *(currentFrameData+1);
    pixels[i].R = *(currentFrameData+2);
  }
}

void Animation::drawRgb16_RLE(Pixel* pixels) {
  if(frameIndex == 0) {
    currentFrameData = frameData;
  }

  // Read runs of RLE data until we get enough data.
  uint8_t count = 0;
  while(count < ledCount) {
    uint8_t run_length = 0x7F & *(currentFrameData);
    uint8_t upperByte = *(currentFrameData + 1);
    uint8_t lowerByte = *(currentFrameData + 2);
    
    uint8_t r = ((upperByte & 0xF8)     );
    uint8_t g = ((upperByte & 0x07) << 5)
              | ((lowerByte & 0xE0) >> 3);
    uint8_t b = ((lowerByte & 0x1F) << 3);
    
    for(uint8_t i = 0; i < run_length; i++) {
      pixels[count + i].R = r;
      pixels[count + i].G = g;
      pixels[count + i].B = b;
    }
    
    count += run_length;
    currentFrameData += 3;
  }
};

void Animation::drawIndexed(Pixel* pixels) {
  currentFrameData = frameData
    + 1 + 3*colorTableEntries   // Offset for color table
    + frameIndex*ledCount;      // Offset for current frame
  
  for(uint8_t i = 0; i < ledCount; i++) {
    pixels[i] = colorTable[*(currentFrameData + i)];
  }
}

void Animation::drawIndexed_RLE(Pixel* pixels) {
  if(frameIndex == 0) {
    currentFrameData = frameData
      + 1 + 3*colorTableEntries;   // Offset for color table
  }

  // Read runs of RLE data until we get enough data.
  int count = 0;
  while(count < ledCount) {
    uint8_t run_length = *(currentFrameData++);
    uint8_t colorIndex = *(currentFrameData++);
    
    for(uint8_t i = 0; i < run_length; i++) {
      pixels[count++] = colorTable[colorIndex];
    }
  }
};


