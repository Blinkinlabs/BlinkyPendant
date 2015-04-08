#include "defaultanimation.h"
#include "animation.h"
#include "blinkytile.h"

// Make a default animation, and write it to flash
void makeDefaultAnimation(NoFatStorage& storage) {
    #define SAMPLE_ANIMATION_FRAME_COUNT  6
    #define SAMPLE_ANIMATION_SPEED        2000

    // Force this to a page size
//    #define SAMPLE_ANIMATION_SIZE (ANIMATION_HEADER_LENGTH + SAMPLE_ANIMATION_FRAME_COUNT*LED_COUNT*BYTES_PER_PIXEL)
    #define SAMPLE_ANIMATION_SIZE PAGE_SIZE

    uint8_t sampleAnimation[SAMPLE_ANIMATION_SIZE];
    for(int i = 0; i < SAMPLE_ANIMATION_SIZE; i++)
        sampleAnimation[i] = 0xFF;

    ((uint32_t*)sampleAnimation)[0] = SAMPLE_ANIMATION_FRAME_COUNT;
    ((uint32_t*)sampleAnimation)[1] = SAMPLE_ANIMATION_SPEED;

    for(int frame = 0; frame < SAMPLE_ANIMATION_FRAME_COUNT; frame++) {
        for(int pixel = 0; pixel < LED_COUNT; pixel++) {
            uint8_t value = 0;

            switch(frame) {
                case 0:
                    value = 255;
                    break;
                case 1:
                    value = 128;
                    break;
                case 2:
                    value = 64;
                    break;
                case 3:
                    value = 32;
                    break;
                case 4:
                    value = 16;
                    break;
                case 5:
                    value = 8;
                    break;
            }
            sampleAnimation[ANIMATION_HEADER_LENGTH + frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 0] = value;     // b
            sampleAnimation[ANIMATION_HEADER_LENGTH + frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 1] = 0;     // g
            sampleAnimation[ANIMATION_HEADER_LENGTH + frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 2] = 0; // r
        }
    }

    // TODO: Handle multi-page writes
    int animationFile = storage.createNewFile(FILETYPE_ANIMATION, SAMPLE_ANIMATION_SIZE);
    if(animationFile < 0)
        return;
    storage.writePageToFile(animationFile, 0, sampleAnimation);
}
