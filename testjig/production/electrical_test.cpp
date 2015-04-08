/*
 * Electrical test for Fadecandy boards
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include "electrical_test.h"
#include "testjig.h"
#include "arm_kinetis_reg.h"

// General IO pin that should be tested
typedef struct testPin {
    int analogInput;        // analog input the pin is connected to
    int pinNumber;          // pin number on the DUT
    float expectedVoltage;  // Expected voltage (3.3V from the processor, 5V through a buffer)
    bool inverted;          // True if the analog signal is inverted from the input.
};


float ElectricalTest::analogVolts(int pin)
{
    // Analog input and voltage divider constants
    const float reference = 1.2;
    const float dividerA = 1000;    // Input to ground
    const float dividerB = 6800;    // Input to signal
    const int adcMax = 1023;

    const float scale = (reference / adcMax) * ((dividerA + dividerB) / dividerA);
    return analogRead(pin) * scale;
}

bool ElectricalTest::analogThreshold(int pin, float nominal, float tolerance)
{
    // Measure an analog input, and verify it's close enough to expected values.
    return analogThresholdFromSample(analogVolts(pin), pin, nominal, tolerance);
}

bool ElectricalTest::analogThresholdFromSample(float volts, int pin, float nominal, float tolerance)
{
    float lower = nominal - tolerance;
    float upper = nominal + tolerance;

    if (volts < lower || volts > upper) {
        target.log(LOG_ERROR,
                "ETEST: Analog value %d outside reference range! "
                "value = %.2fv, ref = %.2fv +/- %.2fv",
                pin, volts, nominal, tolerance);
        return false;
    }

    return true;
}

bool ElectricalTest::testOutputPattern(uint8_t bits)
{   
    // These are the output pins that we want to cycle through
    #define OUTPUT_PIN_COUNT 4
    testPin outputPins[OUTPUT_PIN_COUNT] {
//        {0, target.PTA4, 3.3, false},    // GPIO PA4
        {1, target.PTB0, 3.3, false},    // GPIO PB0
//        {2, target.PTC1, 3.3, false},    // GPIO PC1
        {3, target.PTD4, 5, true},       // LED 5V out
        {4, target.PTC3, 5, false},      // LED ADDR
//        {5, target.PTB1, 3.3, false},    // GPIO PB1
        {7, target.PTC4, 5, false},      // LED DATA
    };

    // Set the target's output pins to the given bitmask, and check all analog values

    // Write the port all at once
    for(int pin = 0; pin < OUTPUT_PIN_COUNT; pin++) {
        if(!target.pinMode(outputPins[pin].pinNumber, OUTPUT))
            return false;
        if(!target.digitalWrite(outputPins[pin].pinNumber, (bits >> pin) & 0x01))
            return false;
    }

    // Check power supply each time
    if (!analogThreshold(analogTarget33vPin, 3.3)) return false;
    if (!analogThreshold(analogTargetVUsbPin, 5.0)) return false;

    // Check all data signal levels
    for(int pin = 0; pin < OUTPUT_PIN_COUNT; pin++)  {
        bool bit = (bits >> pin) & 1;
        if(!outputPins[pin].inverted) {
            if (!analogThreshold(outputPins[pin].analogInput, bit ? outputPins[pin].expectedVoltage : 0.0))
                return false;
        }
        else {
            if (!analogThreshold(outputPins[pin].analogInput, bit ? 0.0 : outputPins[pin].expectedVoltage))
                return false;
        }
    }

    return true;
}

bool ElectricalTest::testAllOutputPatterns()
{
    target.log(logLevel, "ETEST: Testing data output patterns");

    // All on, all off
    if (!testOutputPattern(0x00)) return false;
    if (!testOutputPattern(0xFF)) return false;

    // One bit set
    for (unsigned n = 0; n < 8; n++) {
        if (!testOutputPattern(1 << n))
            return false;
    }

    // One bit missing
    for (unsigned n = 0; n < 8; n++) {
        if (!testOutputPattern(0xFF ^ (1 << n)))
            return false;
    }

    // Leave all outputs on
    return testOutputPattern(0xFF);
}

bool ElectricalTest::initTarget()
{
    // Target setup that's needed only once per test run

    // Output pin directions
    for (unsigned n = 0; n < 8; n++) {
        if (!target.pinMode(outPin(n), OUTPUT))
            return false;
    }

    // Disable target USB USB pull-ups
    if (!target.usbSetPullup(false))
        return false;

    return true;
}

void ElectricalTest::setPowerSupplyVoltage(float volts)
{
    // Set the variable power supply voltage. Usable range is from 0V to system VUSB.

    int pwm = constrain(volts * (255 / powerSupplyFullScaleVoltage), 0, 255);
    pinMode(powerPWMPin, OUTPUT);
    analogWriteFrequency(powerPWMPin, 1000000);
    analogWrite(powerPWMPin, pwm);

    /*
     * Time for the PSU to settle. Our testjig's power supply settles very
     * fast (<1ms), but the capacitors on the target need more time to charge.
     */
    delay(150);
}

void ElectricalTest::powerOff()
{
    setPowerSupplyVoltage(0);
}

bool ElectricalTest::powerOn()
{
    target.log(logLevel, "ETEST: Enabling power supply");
    const float volts = 5.0;
    setPowerSupplyVoltage(volts);
    return analogThreshold(analogTargetVUsbPin, volts);
}

bool ElectricalTest::testHighZ(int pin)
{
    // Test a pin to make sure it's high-impedance, by using its parasitic capacitance
    for (unsigned i = 0; i < 10; i++) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, i & 1);
        pinMode(pin, INPUT);
        if (digitalRead(pin) != (i & 1))
            return false;
    }
    return true;
}

bool ElectricalTest::testPull(int pin, bool state)
{
    // Test a pin for a pull-up/down resistor
    for (unsigned i = 0; i < 10; i++) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, i & 1);
        pinMode(pin, INPUT);
        if (digitalRead(pin) != state)
            return false;
    }
    return true;
}    

bool ElectricalTest::testUSBConnections()
{
    target.log(logLevel, "ETEST: Testing USB connections");

    // Run this test a few times
    for (unsigned iter = 0; iter < 4; iter++) {

        // Start with pull-up disabled
        if (!target.usbSetPullup(false))
            return false;

        // Test both USB ground connections
        pinMode(usbShieldGroundPin, INPUT_PULLUP);
        pinMode(usbSignalGroundPin, INPUT_PULLUP);
        if (digitalRead(usbShieldGroundPin) != LOW) {
            target.log(LOG_ERROR, "ETEST: Faulty USB shield ground");
            return false;
        }
        if (digitalRead(usbSignalGroundPin) != LOW) {
            target.log(LOG_ERROR, "ETEST: Faulty USB signal ground");
            return false;
        }

        // Test for a high-impedance USB D+ and D- by charging and discharging parasitic capacitance
        if (!testHighZ(usbDMinusPin)) {
            target.log(LOG_ERROR, "ETEST: Fault on USB D-, expected High-Z");
            return false;
        }
        if (!testHighZ(usbDPlusPin)) {
            target.log(LOG_ERROR, "ETEST: Fault on USB D+, expected High-Z");
            return false;
        }

        // Turn on USB pull-up on D+
        if (!target.usbSetPullup(true))
            return false;

        // Now D+ should be pulled up, and D- needs to still be high-Z
        if (!testPull(usbDPlusPin, HIGH)) {
            target.log(LOG_ERROR, "ETEST: Fault on USB D+, no pull-up found");
            return false;
        }
        if (!testHighZ(usbDMinusPin)) {
            target.log(LOG_ERROR, "ETEST: Fault on USB D-, expected High-Z. Possible short to D+");
            return false;
        }

    }

    return true;
}

bool ElectricalTest::testPinsForShort(int count, unsigned* pins) {
    // First set all the pins to input, pullup.
    for(int pin = 0; pin < count; pin++) {
        target.pinMode(pins[pin], INPUT_PULLUP);
    }
    
    // Check that all pins are indeed high
    for(int pin = 0; pin < count; pin++) {
        if(!target.digitalRead(pins[pin])) {
            target.log(LOG_ERROR, "ETEST: pin stuck low, pin %i", pins[pin]);
            return false;
        }
    }
    
    // For each pin, set it low, then check if any adjacent pins went low
    for(int pinUnderTest = 0; pinUnderTest < count; pinUnderTest++) {
        target.pinMode(pins[pinUnderTest], OUTPUT);
        target.digitalWrite(pins[pinUnderTest], LOW);
        
        for(int pin = 0; pin < count; pin++) {
            if((pinUnderTest != pin) && !(target.digitalRead(pins[pin]))) {
                target.log(LOG_ERROR, "ETEST: pin %i shorted to pin %i", pins[pinUnderTest], pins[pin]);
                return false;
            }
        }
        
        target.pinMode(pins[pinUnderTest], INPUT_PULLUP);
    }
    
    return true;
}

bool ElectricalTest::testTopPinShorts()
{
  target.pinMode(target.PTC2, OUTPUT);
  target.digitalWrite(target.PTC2, HIGH);
  
    // These pins are in the top row of the chip and can be tested as a block
    #define TOP_PIN_COUNT 9
    unsigned topPins[TOP_PIN_COUNT] = {
        target.PTC3, // ADDRESS_PROGRAM (it's around the corner anyway)
        target.PTC4, // DATA_OUT (has 1k resistor)
        target.PTC5, // FLASH_SCK
        target.PTC6, // FLASH_MOSI (must pull CS pin high)
        target.PTC7, // FLASH_MISO
        target.PTD4, // POWER_ENABLE
        target.PTD5, // BUTTON2
        target.PTD6, // STATUS
        target.PTD7, // BUTTON1
    };
    
    return testPinsForShort(TOP_PIN_COUNT, topPins);
}

bool ElectricalTest::testSidePinShorts()
{
    // These pins are in the top row of the chip and can be tested as a block
    #define SIDE_PIN_COUNT 2
    unsigned sidePins[TOP_PIN_COUNT] = {
//        target.PTB0,    // Can't test these since 
//        target.PTB1,
//        target.PTC1,
        target.PTC2, // FLASH_CS
        target.PTC3, // ADDRESS_PROGRAM
//        target.PTA4, // (it's adjacent to PB0 in the breakout header)
    };
    
    return testPinsForShort(SIDE_PIN_COUNT, sidePins);
}

bool ElectricalTest::runAll()
{
    target.log(logLevel, "ETEST: Beginning electrical test");

    if (!initTarget())
        return false;

    // USB tests
    if (!testUSBConnections())
        return false;
        
    // Top row of pins short test
    if (!testTopPinShorts())
        return false;
        
    // Top row of pins short test
    if (!testSidePinShorts())
        return false;

    // Output patterns
    if (!testAllOutputPatterns())
        return false;

    target.log(logLevel, "ETEST: Successfully completed electrical test");
    return true;
}
