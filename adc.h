/**
 * ADC control functions
 */

#ifndef _ADC_H_
#define _ADC_H_

#include "Arduino.h"

// Define various ADC prescaler
static const unsigned char PS_16 = (1 << ADPS2);
static const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
static const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
static const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

/**
 * Standard ADC clock requires 220us per sample, which is way too slow. We speed up the clock 8 times to the almost maximum clock speed for the ADC.
 * The datasheet warns that this reduces the resolution but we do not need 10 bits anyway.
 */
inline static void set_ADC_speed() {
  // set up the ADC
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library

  // you can choose a prescaler from above.
  // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_16;    // set our own prescaler to 64 
}

#endif
