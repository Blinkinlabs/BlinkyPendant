/*
 * Remote control for the Fadecandy firmware, using the ARM debug interface.
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
#include "fc_remote.h"
#include "testjig.h"
#include "firmware_data.h"

// TODO: DELETE ME!
#define fw_pFlags 0x00FFFFFF
#define fw_pFbPrev 0x00FFFFFF
#define fw_pFbNext 0x00FFFFFF
#define fw_pLUT 0x00FFFFFF
#define fw_usbPacketBufOffset 0x00FFFFFF


bool FcRemote::installFirmware()
{
    // Install firmware, blinking both target and local LEDs in unison.

    bool blink = false;
    static int i = 0;
    ARMKinetisDebug::FlashProgrammer programmer(target, fw_data, fw_sectorCount);

    if (!programmer.begin())
        return false;

    while (!programmer.isComplete()) {
        if (!programmer.next()) return false;
        
        if((i++ % 128) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }

    return true;
}

bool FcRemote::boot()
{ 
    // Run the new firmware, and let it boot
    if (!target.reset())
        return false;
    delay(50);
    return true;
}

bool FcRemote::setLED(bool on)
{
    const unsigned pin = target.PTD6;
    return
        target.pinMode(pin, OUTPUT) &&
        target.digitalWrite(pin, !on);
}

bool FcRemote::setFlags(uint8_t cflag)
{
    // Set control flags    
    return target.memStoreByte(fw_pFlags, cflag);
}

bool FcRemote::testUserButtons()
{
    // Button pins to test
    const unsigned button2Pin = target.PTD5;
    const unsigned button1Pin = target.PTD7;
    
    if(!target.pinMode(button1Pin,INPUT_PULLUP))
        return false;
    if(!target.pinMode(button2Pin,INPUT_PULLUP))
        return false;
        
    // Button should start high
    if(!target.digitalRead(button1Pin)) {
        target.log(target.LOG_ERROR, "BUTTON: Button 1 stuck low");
        return false;
    }
    if(!target.digitalRead(button2Pin)) {
        target.log(target.LOG_ERROR, "BUTTON: Button 2 stuck low");
        return false;
    }
        
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to press button 1");

    const int FLASH_SPEED = 200;

    while(target.digitalRead(button1Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }

    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to release button 1");

    while(!target.digitalRead(button1Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to press button 2");

    while(target.digitalRead(button2Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to release button 2");
    
    while(!target.digitalRead(button2Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Buttons ok.");
    return true;
}

bool FcRemote::testExternalFlash()
{
    target.log(target.LOG_NORMAL, "External flash test: Beginning external flash test");
    
    ////// Initialize the SPI hardware
    if (!target.initializeSpi0())
        return false;
    
    ////// Identify the flash
    
    if(!target.sendSpi0(0x9F, false)) {
        target.log(target.LOG_ERROR, "External flash test: Error communicating with device.");
        return false;
    }

    const int JEDEC_ID_LENGTH = 3;
    uint8_t receivedId[JEDEC_ID_LENGTH];

    for(int i = 0; i < JEDEC_ID_LENGTH; i++) {
        if(!target.receiveSpi0(receivedId[i], (JEDEC_ID_LENGTH - 1) == i)) {
            target.log(target.LOG_ERROR, "External flash test: Error communicating with device.");
            return false;
        }
    }
    
    if((receivedId[0] != 0x01) | (receivedId[1] != 0x40) | (receivedId[2] != 0x15)) {    // Spansion 16m
//    if((receivedId[0] != 0xEF) | (receivedId[1] != 0x40) | (receivedId[2] != 0x14)) {    // Winbond 8m
        target.log(target.LOG_ERROR, "External flash test: Invalid device type, got %02X%02X%02X", receivedId[0], receivedId[1], receivedId[2]);
        return false;
    }
    
    ////// Erase the flash
    
    if(!target.sendSpi0(0x06, true)) {
        target.log(target.LOG_ERROR, "External flash test: Error enabling flash write");
        return false;
    }

    if(!target.sendSpi0(0xC7, true)) {
        target.log(target.LOG_ERROR, "External flash test: Error sending erase command.");
        return false;
    }
    
    target.log(target.LOG_NORMAL, "External flash test: Erasing external flash");
    
    uint8_t sr;
    do {
        static bool blink = false;
        static int i = 0;
        const int FLASH_SPEED = 40;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
      
      if(!target.sendSpi0(0x05, false)) {
          target.log(target.LOG_ERROR, "External flash test: Error getting status report");
          return false;
      }
      if(!target.receiveSpi0(sr, true)) {
          target.log(target.LOG_ERROR, "External flash test: Error getting status report");
          return false;
      }
    }
    while(sr & 0x01);
    
    if(!target.sendSpi0(0x03, true)) {
        target.log(target.LOG_ERROR, "External flash test: Error disabling flash write");
        return false;
    }
    
    
    target.log(target.LOG_NORMAL, "External flash test: Successfully completed external flash test!");
    return true;
}

//bool FcRemote::initLUT()
//{
//    // Install a trivial identity-mapping LUT, writing directly to the firmware's LUT buffer.
//    for (unsigned channel = 0; channel < 3; channel++) {
//        for (unsigned index = 0; index < LUT_CH_SIZE; index++) {
//            if (!setLUT(channel, index, index << 8))
//                return false;
//        }
//    }
//    return true;
//}
//
//bool FcRemote::setLUT(unsigned channel, unsigned index, int value)
//{
//    return target.memStoreHalf(fw_pLUT + 2*(index + channel*LUT_CH_SIZE), constrain(value, 0, 0xFFFF));
//}
//
//bool FcRemote::setPixel(unsigned index, int red, int green, int blue)
//{
//    // Write one pixel directly into the fbNext framebuffer on the target.
//
//    uint32_t idPacket = index / PIXELS_PER_PACKET;
//    uint32_t idOffset = fw_usbPacketBufOffset + 1 + (index % PIXELS_PER_PACKET) * 3;
//
//    uint32_t fb;        // Address of the current fcFramebuffer bound to fbNext
//    uint32_t packet;    // Pointer to usb_packet in question
//
//    return
//        target.memLoad(fw_pFbNext, fb) &&
//        target.memLoad(fb + idPacket*4, packet) &&
//        target.memStoreByte(packet + idOffset + 0, constrain(red, 0, 255)) &&
//        target.memStoreByte(packet + idOffset + 1, constrain(green, 0, 255)) &&
//        target.memStoreByte(packet + idOffset + 2, constrain(blue, 0, 255));
//}
//
//float FcRemote::measureFrameRate(float minDuration)
//{
//    // Use the end-to-end LED data signal to measure the overall system frame rate.
//    // Gaps of >50us indicate frame boundaries
//
//    pinMode(dataFeedbackPin, INPUT);
//
//    uint32_t minMicros = minDuration * 1000000;
//    uint32_t startTime = micros();
//    uint32_t gapStart = 0;
//    bool inGap = false;
//    uint32_t frames = 0;
//    uint32_t duration;
//
//    while (1) {
//        long now = micros();
//        duration = now - startTime;
//        if (duration >= minMicros)
//            break;
//
//        if (digitalRead(dataFeedbackPin)) {
//            // Definitely not in a gap, found some data
//            inGap = false;
//            gapStart = now;
//
//        } else if (inGap) {
//            // Already in a gap, wait for some data.
//
//        } else if (uint32_t(now - gapStart) >= 50) {
//            // We just found an inter-frame gap
//
//            inGap = true;
//            frames++;
//        }
//    }
//
//    return frames / (duration * 1e-6);
//}
//
//bool FcRemote::testFrameRate()
//{
//    const float goalFPS = 375;
//    const float maxFPS = 450;
//
//    target.log(target.LOG_NORMAL, "FPS: Measuring frame rate...");
//    float fps = measureFrameRate();
//    target.log(target.LOG_NORMAL, "FPS: Measured %.2f frames/sec", fps);
//
//    if (fps > maxFPS) {
//        target.log(target.LOG_ERROR, "FPS: ERROR, frame rate of %.2f frames/sec is unrealistically high!", fps);
//        return false;
//    }
//
//    if (fps < goalFPS) {
//        target.log(target.LOG_ERROR, "FPS: ERROR, frame rate of %.2f frames/sec is below goal of %.2f!",
//            fps, goalFPS);
//        return false;
//    }
//
//    return true;
//}
