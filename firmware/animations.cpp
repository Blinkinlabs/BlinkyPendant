#include "animations.h"

// TODO: Move me to a separate class
#define ANIMATION_START ((uint8_t*)0xA000)

#define MAGIC_0     (0x31)
#define MAGIC_1     (0x23)

#define MAGIC_0_PTR       (ANIMATION_START+0x0000)   // Magic byte 0
#define MAGIC_1_PTR       (ANIMATION_START+0x0001)   // Magic byte 1
#define PATTERN_COUNT_PTR (ANIMATION_START+0x0002)   // Number of patterns in the pattern table (1 byte)
#define DISPLAY_MODE_PTR  (ANIMATION_START+0x0003)   // Playback mode (1 byte)
#define PATTERN_TABLE_PTR (ANIMATION_START+0x0004)   // Location of the pattern table in the flash memory

#define PATTERN_TABLE_ENTRY_LENGTH      9        // Length of each entry, in bytes
  
#define ENCODING_TYPE_OFFSET    0    // Encoding (1 byte)
#define FRAME_DATA_OFFSET       1    // Memory location (4 bytes)
#define FRAME_COUNT_OFFSET      5    // Frame count (2 bytes)
#define FRAME_DELAY_OFFSET      7    // Frame delay (2 bytes)


#define DISPLAY_MODE_POV       10   // Play back in POV mode
#define DISPLAY_MODE_TIMED     11   // Play back in timed mode

bool checkHeader() {
    if(*MAGIC_0_PTR != MAGIC_0) {
       return false;
    }

    if(*MAGIC_1_PTR != MAGIC_1) {
       return false;
    }

    return true;
}

/// Get the number of animations stored in the flash
/// @return animation count, or 0 if no animations present
unsigned int getAnimationCount() {
    if(!checkHeader()) {
        return 0;
    }

    return *PATTERN_COUNT_PTR;
}

uint8_t getDisplayMode() {
    if(!checkHeader()) {
        return 0;
    }

    return *DISPLAY_MODE_PTR;
}

bool loadAnimation(unsigned int index, Animation* animation) {
    if(index > getAnimationCount()) {
        return false;
    }

    uint8_t* patternEntryAddress =
            PATTERN_TABLE_PTR
            + index * PATTERN_TABLE_ENTRY_LENGTH;

    uint8_t encodingType = *(patternEntryAddress + ENCODING_TYPE_OFFSET);
    
    uint8_t *frameData  = PATTERN_TABLE_PTR
                        + getAnimationCount() * PATTERN_TABLE_ENTRY_LENGTH
                        + (*(patternEntryAddress + FRAME_DATA_OFFSET + 0) << 24)
                        + (*(patternEntryAddress + FRAME_DATA_OFFSET + 1) << 16)
                        + (*(patternEntryAddress + FRAME_DATA_OFFSET + 2) << 8)
                        + (*(patternEntryAddress + FRAME_DATA_OFFSET + 3) << 0);
 
    uint16_t frameCount = (*(patternEntryAddress + FRAME_COUNT_OFFSET    ) << 8)
                        + (*(patternEntryAddress + FRAME_COUNT_OFFSET + 1) << 0);

    uint16_t frameDelay = (*(patternEntryAddress + FRAME_DELAY_OFFSET    ) << 8)
                        + (*(patternEntryAddress + FRAME_DELAY_OFFSET + 1) << 0);
 
    // TODO: Validation, here or in Animation.init()

    animation->init(frameCount, frameData, encodingType, LED_COUNT, frameDelay);

    return true;
}
