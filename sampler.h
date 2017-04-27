#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include "Arduino.h"

// --------- Hardware configuration --------- 

static const int rxPin = 7;          // Receive pin for digital sampling (much faster than analog but requires level shifters)
static const int rxPinAna = A0;      // Receive pin for analog reception
static const int txPin = 3;          // Transmit pin, digital 5V output

// --------- Sampling settings --------- 

#define RX_ANALOG 1                  // When set to 1, use analog sampling on rxPinAna, otherwise use digital sampling on rxPin
//#define RX_ANALOG_LEVEL_HIGH 250     // Analog level to reach before a '1' is detected
//#define RX_ANALOG_LEVEL_LOW 230      // Analog level to drop below before detecting a '0'
#define RX_ANALOG_LEVEL_HIGH 90     // Analog level to reach before a '1' is detected
#define RX_ANALOG_LEVEL_LOW 70      // Analog level to drop below before detecting a '0'
#define RX_INVERT 0                  // When level shifting causes an inversion - the sampler can simply be inverted

// Sample interval in us; this should be sufficiently high to get an accurate bit stream
// Note: the standard ADC settings require 220us per sample - which is useless; the ADC core clock is sped up to reduce this to 32us per sample at the cost of reduced resolution...
#define RX_SAMPLE_INTERVAL_US 50

// Utility function to handle reading from the analog or digital pins
// Note that analog reading is needed when the receiver is running at 3.3V
static inline uint8_t readRxPin() {
#if RX_ANALOG
  static uint8_t last = 0;
  uint8_t val = 0;

  // Do a read from the ADC
  int anaval = analogRead(rxPinAna);

  // Convert into a binary choice - to de-noise, use a gray area before flipping bits
  if(last == 0) {
    // Last sample was a 0; use the upper limit to switch to 1
    val = anaval > RX_ANALOG_LEVEL_HIGH;
  } else {
    // Last sample was a 1; use the lower limit to switch to 1
    val = anaval > RX_ANALOG_LEVEL_LOW;
  }
  
  #if RX_INVERT
  return !val;
  #else
  return val;
  #endif
#else
  // Digital read
  #if RX_INVERT
  return !digitalRead(rxPin);
  #else
  return digitalRead(rxPin);
  #endif
#endif
}

#endif
