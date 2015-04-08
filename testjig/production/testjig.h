// Testjig hardware definitions

#pragma once

// UI
static const unsigned buttonPin = 2;                // Ok
static const unsigned ledPin = 13;                  // Ok
static const unsigned ledPassPin = 11;              // Ok
static const unsigned ledFailPin = 12;              // Ok


// Debug port
static const unsigned swclkPin = 3;                 // Ok
static const unsigned swdioPin = 4;                 // Ok

// Electrical testing
static const unsigned fcTXPin = 0;                  // Ok
static const unsigned fcRXPin = 1;                  // Ok
static const unsigned powerPWMPin = 10;             // Ok
static const unsigned usbDMinusPin = 6;             // Ok
static const unsigned usbDPlusPin = 5;              // Ok
static const unsigned usbShieldGroundPin = 7;       // Ok
static const unsigned usbSignalGroundPin = 8;       // Ok
static const unsigned analogTarget33vPin = 8;       // Ok
static const unsigned analogTargetVUsbPin = 9;      // Ok

// LED functional testing (fast pin)
static const unsigned dataFeedbackPin = 11;         // TODO: Not present!

// Analog constants
//static const float powerSupplyFullScaleVoltage = 6.42;
static const float powerSupplyFullScaleVoltage = 5;
