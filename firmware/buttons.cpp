#include "WProgram.h"
#include "pins_arduino.h"
#include "buttons.h"
#include "blinkytile.h"

// Button definitions
button buttons[BUTTON_COUNT] = {
    {BUTTON_A_PIN,     0},  // Button A, PD4
};

void Buttons::setup() {
/*
    for(uint8_t b = 0; b < BUTTON_COUNT; b++) {
        pinMode(buttons[b].pin, INPUT_PULLUP);
    }
*/
    // Since our button is on a pin that isn't supported by core-pins, we'll have to access
    // it using the pin registers directly.
    PORTA_PCR3 = PORT_PCR_MUX(1) | PORT_PCR_PS | PORT_PCR_PE | PORT_PCR_SRE;
  
    pressedButton = BUTTON_COUNT;
    lastPressed = BUTTON_COUNT;
}


bool readButtonA() {
    const uint32_t button_1_bit = 1 << 3;  // Button 1 on A3

    GPIOA_PDDR = GPIOA_PDDR & (~button_1_bit);
    uint32_t status = GPIOA_PDIR & (button_1_bit);
    return status != 0;
}

// Scan for new button presses
void Buttons::buttonTask() {
    // If a button is currently pressed, don't bother looking for a new one
    if (lastPressed != BUTTON_COUNT) {
//        if(digitalRead(buttons[lastPressed].pin) == buttons[lastPressed].inverted) {
        if(readButtonA() == buttons[lastPressed].inverted) {
            if(debounceCount < DEBOUNCE_INTERVAL) {
                debounceCount++;
            }
            return;
        }
        lastPressed = BUTTON_COUNT;
    }
    
    for(uint8_t b = 0; b < BUTTON_COUNT; b++) {
        // TODO: Use port access here for speed?
        if (digitalRead(buttons[b].pin) == buttons[b].inverted) {
            lastPressed = b;
            pressedButton = b;
            debounceCount = 0;
            return;
        }
    }
}


bool Buttons::isPressed() {
    return (pressedButton != BUTTON_COUNT && debounceCount == DEBOUNCE_INTERVAL);
}

// If a button was pressed, return it!
int Buttons::getPressed() {
    int pressed;

// TODO: if this goes in an isr, we need to implement a lock here
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//    {
            pressed = pressedButton;
            pressedButton = BUTTON_COUNT;
//        }

    return pressed;
}
