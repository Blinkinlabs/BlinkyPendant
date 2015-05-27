/*
 * Fadecandy testjig firmware for production.
 * This loads an initial bootloader/firmware image on the device, and runs electrical tests.
 *
 * Communicates with the PC for debug purposes using USB-Serial.
 * Final OK / Error status shows up on WS2811 LEDs attached to the DUT.
 */

#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"
#include "fc_remote.h"
#include "electrical_test.h"
#include "testjig.h"

ARMKinetisDebug target(swclkPin, swdioPin, ARMDebug::LOG_NORMAL);
FcRemote remote(target);
ElectricalTest etest(target);

void setup()
{
  pinMode(ledPin, OUTPUT);
    pinMode(ledPassPin, OUTPUT);
    pinMode(ledFailPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    analogReference(INTERNAL);
    Serial.begin(115200);
}

void waitForButton()
{
    // Wait for button press, with debounce

    Serial.println("");
    Serial.println("--------------------------------------------");
    Serial.println(" BlinkyPendant Test Jig : Press button to start");
    Serial.println("--------------------------------------------");
    Serial.println("");

    while (digitalRead(buttonPin) == LOW);
    delay(20);
    while (digitalRead(buttonPin) == HIGH) {
        // While we're waiting, blink the LED to indicate we're alive
        digitalWrite(ledPin, (millis() % 1000) < 150);
    }

//    while(Serial.available() < 1) {
//    }
//    Serial.read();
    
    digitalWrite(ledPin, HIGH);
}

void success()
{
    Serial.println("");
    Serial.println("#### Tests Passed! ####");
    Serial.println("");
}

#define TEST_UNTESTED 0
#define TEST_FAIL 1
#define TEST_PASS 2
int testState = TEST_UNTESTED;


void loop()
{
    // Keep target power supply off when we're not using it
    etest.powerOff();
    
    // Set the status LEDs
    if(testState == TEST_FAIL) {
      digitalWrite(ledPassPin, LOW);
      digitalWrite(ledFailPin, HIGH);
    }
    else if(testState == TEST_PASS) {
      digitalWrite(ledPassPin, HIGH);
      digitalWrite(ledFailPin, LOW);
    }
    else {
      digitalWrite(ledPassPin, HIGH);
      digitalWrite(ledFailPin, HIGH);
    }      
      
    // Button press starts the test
    waitForButton();
    
    testState = TEST_FAIL;
    digitalWrite(ledPassPin, HIGH);
    digitalWrite(ledFailPin, HIGH);

    // Turn on the target power supply
    if (!etest.powerOn())
        return;
    
    // Force a reset during startup to be sure the test interface is available
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    
//    // Test the user button
//    if (!etest.testUserButton())
//        return;

    
    // Start debugging the target
    if (!target.begin())
        return;

    // Release the reset so that the target can be booted
    digitalWrite(resetPin, HIGH);

    if (!target.startup())
        return;

    // Run an electrical test, to verify that the target board is okay
    if (!etest.runAll())
        return;

    // Test that the accelerometer is present and can generate interrupts
    if (!remote.testAccelerometer())
          return;


    // Test that the LED outputs work
    if (!remote.testLEDOutputs())
          return;

//    // Program firmware, blinking both LEDs in unison for status.
//    if (!remote.installFirmware())
//        return;

    // Boot the target
    if (!remote.boot())
        return;

    testState = TEST_PASS;
    success();
}
