
#include "decoder.h"
// Load the project config
#include "config.h"

// Only implement the functions when this module is enabled
#if ENABLE_DEBUG_DECODER

#define MAX_BITS 192

// Buffers to hold the decoded bits
uint8_t bits[MAX_BITS];
uint16_t bitPtr = 0;

/**
 * Print the bit buffer of the packet as far as it has been received
 */
void printBuffer() {
  Serial.print("Events in buffer: ");
  Serial.println(bitPtr);
  
  for(int i=0; i<bitPtr; i++) {
    char c = '?';
    switch(bits[i]) {
      case EVENT_NONE:       c = '.'; break;
      case EVENT_INVALID:    c = 'X'; break;
      case EVENT_HIGH_SHORT: c = 'h'; break;
      case EVENT_LOW_SHORT:  c = 'l'; break;
      case EVENT_LOW_LONG:   c = 'L'; break;
      case EVENT_SYNC:       c = 'S'; break;
      case EVENT_PAUSE:      c = '_'; break;
    }
    
    Serial.print(c);
  }
  Serial.println("-");
}

/**
 * Decode a packet if the stream is valid.
 */
void decodePacket() {
  nexa_pckt_t p;
  uint32_t raw;
  uint8_t bitcnt = 0;
  uint8_t valid = 1;
  uint8_t complete = 0;
  int i = 0;
  
  // Make sure we start with a SYNC
  if(bits[0] != EVENT_SYNC) return;
  
  Serial.print("PCKT: ");
  
  // Every bit is made up of 4 pulses:
  // 1: high short, low long, high short, low short
  // 0: high short, low short, high short, low long
  // Note that this means the energy is averaged per bit: each contains 2 high pulses, 1 short low pulse and 1 long low pulse
  for(i=1; i<bitPtr; i+=4) {
    // Overflow protection: we can receive up to 32 bits; any more is invalid
    if(bitcnt > 32) {
      Serial.println("Too many bits");
      valid = 0;
      break;
    }
    
    // Do a pattern match if at least 4 symbols are available
    if(bitPtr - i > 4) {
      if( bits[i  ] == EVENT_HIGH_SHORT &&
          bits[i+1] == EVENT_LOW_LONG   &&
          bits[i+2] == EVENT_HIGH_SHORT &&
          bits[i+3] == EVENT_LOW_SHORT) {
        // Pattern: hLhl = 1, shift a 1 into the data buffer
        raw <<= 1;
        raw |= 1;
        bitcnt++;
      } else if(
          bits[i  ] == EVENT_HIGH_SHORT &&
          bits[i+1] == EVENT_LOW_SHORT  &&
          bits[i+2] == EVENT_HIGH_SHORT &&
          bits[i+3] == EVENT_LOW_LONG) {
        // Pattern hlhL = 0, shift a 0 into the data buffer
        raw <<= 1;
        bitcnt++;
      } else {
        Serial.print("Invalid pulse pattern at event ");
        Serial.println(i);
        Serial.print("Symbols left until end of buffer: ");
        Serial.println((int)(bitPtr - i));
        valid = 0;
        break;
      }
    } else {
      // Less than 4 symbols available - check for frame ending
      if(bitPtr - i == 2 && bits[i  ] == EVENT_HIGH_SHORT) {
        complete = 1;
      } else {
        Serial.print("Incorrect frame end at event ");
        Serial.println(i);
        valid = 0;
        break;
      }
    }
  }

  // Jump to last point
  //i+=4;

  // Check for the frame end
  //if((i+1) < bitPtr) {
  //  Serial.println(i);
    
  //  if( bits[i] == EVENT_HIGH_SHORT && bits[i+1] == EVENT_PAUSE) {
  //    // Pattern h_: high pulses followed by a very long low pulse = end of packet
  //    complete = 1;
  //  }
 // }
  
  if(valid && complete) {
    Serial.print("Bits: ");
    Serial.println(bitcnt);
    // Apply value to the packet
    *(uint32_t *)(&p) = raw;
    
    Serial.print("Device ID: ");
    Serial.println(p.device_id, HEX);
    Serial.print("Group: ");
    Serial.println(p.group);
    Serial.print("On: ");
    Serial.println(p.on_off);
    Serial.print("Channel: ");
    Serial.println(p.channel);
    Serial.print("Unit: ");
    Serial.println(p.unit);
  } else {
    if(!complete) {
      Serial.println("Incomplete");
    }
    if(!valid) {
      Serial.println("Invalid"); 
    }
  }
}

/**
 * Standard decoder logic: process captured sample and process the event stream that results from it.
 * Once a valid packet has been detected, the packet decoder is called to respond to the NEXA command.
 */
static inline void pushSample(uint8_t val) {
  static uint8_t last_event = EVENT_NONE;
  uint8_t doPrint = 0;
  
  // Begin with event detection
  uint8_t event = detectPulse(val);
  
  // Event recording: store each event type
  switch(event) {
    case EVENT_NONE:
      // multiple occurances of INVALID or NONE are ignored to save space
      if(last_event != event) {
        //bits[bitPtr++] = event;
      }
      break;
    case EVENT_INVALID:
      // multiple occurances of INVALID or NONE are ignored to save space
      if(last_event != event) {
        bits[bitPtr++] = event;
      }
      break;
    case EVENT_HIGH_SHORT:
      bits[bitPtr++] = event;
      break;
    case EVENT_LOW_SHORT:
      bits[bitPtr++] = event;
      break;
    case EVENT_LOW_LONG:
      bits[bitPtr++] = event;
      break;
    case EVENT_SYNC:
      bitPtr = 0;
      bits[bitPtr++] = event;
      break;
    case EVENT_PAUSE:
      bits[bitPtr++] = event;
      // If we have enough symbols print the buffer
      if(bitPtr >= 127) doPrint = 1;
      break;
    default:
      // BUG: unknown event type - will be printed as such
      bits[bitPtr++] = event;
      break;
  }
  
  // When requested, print the buffer and reset the pointer
  if(doPrint) {
    printBuffer();
    decodePacket();
    bitPtr = 0;
  }
  
  // Keep rewriting the last event until a SYNC resets to the start
  if(bitPtr == MAX_BITS) {
    bitPtr = MAX_BITS - 1;
  }
  
  last_event = event;
}

/**
 * When the code takes to long, the recorded data stream is useless - print a warning and continue.
 * Note: this is expected to happen during debug printouts
 */
void print_timeout(unsigned long dur) {
  Serial.print("Error: could not compensate for computation time.\nDuration in us: ");
  Serial.print(dur);
  Serial.print("\nTarget in us: ");
  Serial.println(RX_SAMPLE_INTERVAL_US);
}

/**
 * Standard decoder loop: read a sample, push it through the detection logic, wait
 */
void debug_decoder_loop() {
  unsigned long time, dur, wait;

  while(1) {
    // Grab current time
    time = micros();
    
    // Sample from the antenna
    uint8_t val = readRxPin();
    
    // Push the sample into the detection logic
    pushSample(val);
    
    // Correct time offset due to computations
    dur = micros() - time;
    if(dur > RX_SAMPLE_INTERVAL_US) print_timeout(dur);
    wait = RX_SAMPLE_INTERVAL_US - dur;
    // Overflow protection
    //if(wait > RX_SAMPLE_INTERVAL_US) break;
    
    // Wait for the next sampling point
    delayMicroseconds(wait);
  }
  

}

#endif
