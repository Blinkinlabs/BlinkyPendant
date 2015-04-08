#include "WProgram.h"
#include "pins_arduino.h"
#include "blinkytile.h"

void initBoard() {
    pinMode(BUTTON_A_PIN, INPUT);
    pinMode(BUTTON_B_PIN, INPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(POWER_ENABLE_PIN, OUTPUT);
    pinMode(ADDRESS_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);

    setStatusLed(255);
    disableOutputPower();
}

void setStatusLed(uint8_t value) {
    analogWrite(STATUS_LED_PIN, (255-value));
}

void enableOutputPower() {
    digitalWrite(POWER_ENABLE_PIN, LOW);
}

void disableOutputPower() {
    digitalWrite(POWER_ENABLE_PIN, HIGH);
}
