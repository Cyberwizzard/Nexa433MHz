
#include "decoder.h"
// Load the project config
#include "config.h"

// Only implement the functions when this module is enabled
#if ENABLE_FULL_DECODER || ENABLE_DEBUG_DECODER

/**
 * Utility function to detect various pulse types; works on a sample stream so we do not need to store a lot of samples while decoding the stream
 */
uint8_t detectPulse(uint8_t val) {
  static uint16_t zeroes = 0;     // Number of consequtive zeroes
  static uint16_t ones = 0;       // Number of consequtive ones
  static uint8_t last_event = 0; // Track the last relevant event
  
  // High pulse detection
  if(val == 1) {
    uint16_t last_zeroes = zeroes;
    // Reset the zero count and count the ones
    zeroes = 0;
    ones++;
    
    // Low pulse detection: SYNC, SHORT and LONG low pulse types are embedded between SHORT HIGH pulses
    if(last_zeroes != 0) {
      if(last_zeroes >= SHORT_LOW_PULSE_SAMPLES - FUZZY_SAMPLES_SHORT && last_zeroes <= SHORT_LOW_PULSE_SAMPLES + FUZZY_SAMPLES_SHORT) {
        // Short low pulse detected
        last_event = EVENT_LOW_SHORT;
        return EVENT_LOW_SHORT;
      }
      
      if(last_zeroes >= LONG_PULSE_SAMPLES - FUZZY_SAMPLES_LONG && last_zeroes <= LONG_PULSE_SAMPLES + FUZZY_SAMPLES_LONG) {
        // Long low pulse detected
        last_event = EVENT_LOW_LONG;
        return EVENT_LOW_LONG;
      }     

      if(last_zeroes >= START_PULSE_SAMPLES - FUZZY_SAMPLES_LONG && last_zeroes <= START_PULSE_SAMPLES + FUZZY_SAMPLES_LONG) {
        // Start low pulse detected
        last_event = EVENT_SYNC;
        return EVENT_SYNC;
      }      
    }

    // Detect short high pulse
    if(ones >= SHORT_HIGH_PULSE_SAMPLES - FUZZY_SAMPLES_SHORT && ones <= SHORT_HIGH_PULSE_SAMPLES + FUZZY_SAMPLES_SHORT) {
      // Matches short high pulse, did we detect this before (due to the fuzzy match?)
      if(last_event != EVENT_HIGH_SHORT) {
        // New detection
        last_event = EVENT_HIGH_SHORT;
        return EVENT_HIGH_SHORT;
      } else
        // Pulse already reported - ignore re-detection
        return EVENT_NONE;
    } else if(ones > SHORT_HIGH_PULSE_SAMPLES + FUZZY_SAMPLES_SHORT) {
      // Too many ones, this is garbage
      last_event = EVENT_INVALID;
      return EVENT_INVALID;
    }
    
    last_event = EVENT_NONE;
    return EVENT_NONE;
  } else {
    // Low pulse detection, when more low pulses than the SYNC + SHORT pulse is seen - it is usually an end of a frame
    ones = 0;
    
    // Sanity: make sure to only count when it makes sense and skip computations once we go beyond a certain number of zeroes
    if(zeroes < END_PULSE_SAMPLES+10) {
      zeroes++;
    
      if(zeroes == END_PULSE_SAMPLES) {
        // Very long pause - this has to be the end of a frame
        last_event = EVENT_PAUSE;
        return EVENT_PAUSE;
      }
    } else {
      // Too many zeroes - this is between frames or noise
      last_event = EVENT_INVALID;
      return EVENT_INVALID;
    }
    
    // No event
    last_event = EVENT_NONE;
    return EVENT_NONE;
  }
}


#endif
