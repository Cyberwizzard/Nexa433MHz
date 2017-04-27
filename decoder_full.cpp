
// Function prototypes for this file
#include "decoder_full.h"
// Shared decoder functions
#include "decoder.h"
// Load the project config
#include "config.h"

// Only implement the functions when this module is enabled
#if ENABLE_FULL_DECODER

#define PAYLOAD_SIZE_BITS 32                          // Payload size - standard Nexa packet contains 32 bits
#define PAYLOAD_BYTES ((PAYLOAD_SIZE_BITS-1) / 8 + 1) // Complicated way of dividing by 8 and rounding up

union {
  uint32_t    raw;
  nexa_pckt_t pkt;
} buf;

uint32_t prev_pkt_raw;       // Previous raw packet received - used to detect when repeats occur within a number of sampling periods
uint32_t prev_pkt_cnt;       // Counter since last packet was decoded, when it reaches 0 after starting from REPEAT_IGNORE_SAMPLES the value of prev_pkt_raw is erased to support receiving it again

// Repeat interval in which identical packets are ignored after first reception: each bit consists of 3 short and 1 long pulse, 32 bits, plus start and sync, repeated 5 to 6 times.
#define SAMPLES_PER_BIT        ( SHORT_HIGH_PULSE_SAMPLES * 2 + SHORT_LOW_PULSE_SAMPLES + LONG_PULSE_SAMPLES )
#define REAL_PAUSE_SAMPLES     (END_PULSE_SAMPLES * 5)  // The real pause is longer but to save time its defined 5 times too small
#define NUM_REPEATS            6
#define REPEAT_IGNORE_SAMPLES  (((SAMPLES_PER_BIT * PAYLOAD_SIZE_BITS) + START_PULSE_SAMPLES + REAL_PAUSE_SAMPLES) * NUM_REPEATS)

// Clever macro to generate code which causes a compiler error when the condition does not hold
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

uint8_t eventbuf[4];         // Event buffer - each bit consists of 4 pulse events, or 2 events for the frame ending
uint8_t ep = 0;              // Event pointer, set to the location of the next event insertion point
uint8_t seqval = 0;          // Event sequence valid, set to 0 when unexpected sequences are detected
uint8_t dbit = 0;            // Data bit pointer, valid as long as its smaller than 32

// Check the size of some things using a clever preprocessor trick that generates compiler errors if some condition does not hold
// Note: do not call this function as will not result in any instructions when compiled (so it only adds size)
inline void sanityCheck() {
  BUILD_BUG_ON(sizeof(buf) != PAYLOAD_BYTES);
}

/**
 * Invalidate the current packet (if any)
 */
inline void invalidate() {
  ep = 0;     // when invalid pulses or sequences of pulses are detected, reset the event pointer
  seqval = 0; // mark the whole sequence invalid
  dbit = 0;   // Set the data buffer to begin over
}

/**
 * Push a bit in the data buffer. Returns 0 when no space is left in the buffer.
 */
inline uint8_t pushBit(uint8_t bitVal) {
  if(dbit < PAYLOAD_SIZE_BITS) {
    buf.raw = (buf.raw << 1) | (bitVal & 0x1); // Shift a single bit into the buffer
    dbit++;
    return 1; // True: bit added
  }
  return 0; // False: overflow
}

/**
 * Standard decoder logic: process captured sample and process the event stream that results from it.
 * Once a valid packet has been detected, the packet decoder is called to respond to the NEXA command.
 * @return 0 for no result, 1 for packet received in *data
 */
static inline int8_t pushSample(uint8_t val) {
  // Begin with event detection
  uint8_t event = detectPulse(val);

  // Boot mode: find the start of a packet
  if(seqval == 0) {
    if(event == EVENT_SYNC) {
      // Sync pulse found - start of a new packet
      seqval = 1; // mark the sequence valid to start with
    }
    
    return 0; // Without a valid sequence - stop processing here
  } else {
    // Decoding mode: decoding a packet
    switch(event) {
      case EVENT_NONE:
        // Inter-event samples, ignore
        return 0; // Stop processing for this event
        break;
      case EVENT_HIGH_SHORT:
      case EVENT_LOW_SHORT:
      case EVENT_LOW_LONG:
        // Normal events during a packet - add to event buffer
        eventbuf[ep++] = event;
        break;
      case EVENT_SYNC:
        // New sync detected, invalidate previous results and start over
        invalidate();
        seqval = 1;   // Mark the sequence as valid again as this might be the start of a correct packet
        return 0;     // Stop processing for this event
        break;
      case EVENT_PAUSE:
        // Marks the completion of the packet - this event is used down below so do not act here
        break;
      case EVENT_INVALID:
        invalidate();
        return 0; // Stop processing for this event
        break;
    }
  }

  // When the event buffer is full, see if a valid bit pattern was received
  if(ep == 4) {
    // Reset event pointer (before jumping out of this function)
    ep = 0;
    
    if(eventbuf[0] == EVENT_HIGH_SHORT && 
       eventbuf[1] == EVENT_LOW_SHORT  && 
       eventbuf[2] == EVENT_HIGH_SHORT && 
       eventbuf[3] == EVENT_LOW_LONG) {
       // Pattern: hlhL = 1, shift a 0 into the data buffer
       if(!pushBit(0)) {
         // On a buffer overflow, mark the whole packet as invalid
         invalidate();
         return 0;
       }
    } else if(
       eventbuf[0] == EVENT_HIGH_SHORT && 
       eventbuf[1] == EVENT_LOW_LONG   && 
       eventbuf[2] == EVENT_HIGH_SHORT && 
       eventbuf[3] == EVENT_LOW_SHORT) {
       // Pattern: hLhl = 1, shift a 1 into the data buffer
       if(!pushBit(1)) {
         // On a buffer overflow, mark the whole packet as invalid
         invalidate();
         return 0;
       } 
    } else {
      // Anything else is an error and invalidates the whole reception
      invalidate();
      return 0; // Stop processing for this event
    }
  }
  
  // See if we detected a PAUSE which should complete the packet
  if(event == EVENT_PAUSE) {
    if(dbit == PAYLOAD_SIZE_BITS) {
      // Invalidate the decoder state so a new packet can be received
      invalidate();
      
      // Payload complete
      return 1;
    }
  }

  // Nothing to report, return zero
  return 0;  
}

static inline void printTooSlow(unsigned long dur) {
  Serial.print("Error: could not compensate for computation time.\nDuration in us: ");
  Serial.print(dur);
  Serial.print("\nTarget in us: ");
  Serial.println(RX_SAMPLE_INTERVAL_US);
  
//  while(1) {}
}

static inline uint8_t decodeSample(uint8_t sample) {
  uint8_t res = pushSample(sample);
  
  // If a debouncer is running - count it down
  if(prev_pkt_cnt > 0) {
    prev_pkt_cnt--;
    // Wipe the previously received value if the debouncer reached 0
    if(prev_pkt_cnt == 0) prev_pkt_raw = 0;
  }
  
  if(res) {
    // Received a packet - check if it was received before
    if(prev_pkt_raw == buf.raw) {
      // It was seen before - drop it
      return 0;
    }
    
    // Received new packet, copy it into the debouncer value
    prev_pkt_raw = buf.raw;
    // Set the debounce counter so we will not 'receive' this packet in the next time
    prev_pkt_cnt = REPEAT_IGNORE_SAMPLES;
  }

  return res;
}

/**
 * Standard decoder loop: read a sample, push it through the detection logic, wait
 */
void decoder_loop() {
  unsigned long time, dur, wait;
  uint8_t res;
  
  // Init the debouncer
  prev_pkt_raw = 0;
  prev_pkt_cnt = 0;

  while(1) {
    // Grab current time
    time = micros();
    
    // Sample from the antenna
    uint8_t val = readRxPin();

    // Decode the sample - when a whole packet is received, it will return true
    if(res = decodeSample(val)) {
      // Packet received
      //Serial.println(buf.raw, HEX);
      Serial.print(buf.pkt.device_id, HEX);
      Serial.print(":");
      Serial.print(buf.pkt.unit, HEX);
      Serial.print(" group:");
      Serial.print(buf.pkt.group, HEX);
      Serial.print(" channel: ");
      Serial.print(buf.pkt.channel, HEX);
      Serial.print(" on:");
      Serial.println(buf.pkt.on_off, HEX);
    }
    
    // Correct time offset due to computations
    dur = micros() - time;
    // If there was no packet received (so no serial print) - and the routine is too slow, print something
    if(!res && dur > RX_SAMPLE_INTERVAL_US) printTooSlow(dur);
    wait = RX_SAMPLE_INTERVAL_US - dur;
    // Overflow protection
    //if(wait > RX_SAMPLE_INTERVAL_US) break;
    
    // Wait for the next sampling point
    delayMicroseconds(wait);
  }
}

#endif
