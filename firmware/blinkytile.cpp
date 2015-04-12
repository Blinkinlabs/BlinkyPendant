#include "WProgram.h"
#include "pins_arduino.h"
#include "blinkytile.h"

const uint32_t button_1_bit = 1 << 3;  // Button 1 on A3

void initBoard() {
//    pinMode(BUTTON_A_PIN, INPUT);

    // Read the status pin.
    PORTA_PCR3 = PORT_PCR_MUX(1) | PORT_PCR_PS | PORT_PCR_PE | PORT_PCR_SRE;

    GPIOA_PDDR = GPIOA_PDDR & (~button_1_bit);
}

bool readButton() {
    uint32_t status = GPIOA_PDIR & (button_1_bit);

    return status == 0;
}
