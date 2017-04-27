
#include "recorder.h"
#include "Arduino.h"

// Global pointer to the memory allocated to hold the recording
uint8_t *recording = (uint8_t *)-1;

inline void pushSample(uint8_t val) {
  static unsigned long scnt = 0;
  static uint8_t curbyte = 0;
  static uint8_t bitcnt = 0;
   
  if(scnt < RECORDER_BYTES) {
    // Store value
    // Shift in the next bit
    curbyte <<= 1;
    curbyte |= (val && 0x1);
    bitcnt++;
 
    // Every 8 bits, store the shift buffer into the main buffer
    if(bitcnt == 8) {
      recording[scnt] = curbyte; // Save value
      bitcnt = 0;     // Reset bit counter
      scnt++;         // Increase sample counter
    }
  } else {
    // Done, print and lock up
    Serial.println("Recording complete");

    for(scnt = 0; scnt < RECORDER_BYTES; scnt++) {
      for(int i=0; i<8; i++) {
        if(recording[scnt] & (1 << (7 - i))) {
          Serial.print("1 ");
        } else {
          Serial.print("0 ");
        }
      }
      if(scnt % 4 == 3) Serial.println();
    }
    
    while(1) {} 
  }
}

/**
 * Main control loop for the recorder logic
 */
void recorder_loop() {
  unsigned long time, dur, wait;

  // Allocate memory for the recording
  // Note that this will fail if too many samples are requested
  recording = (uint8_t *)malloc(RECORDER_BYTES);
  if((int)recording == 0) {
    Serial.println("Could not allocate memory of ");
    Serial.print(RECORDER_BYTES);
    Serial.println(" bytes");
    while(1) {}
  }
  
  Serial.print("Sample interval: ");
  Serial.print(RX_SAMPLE_INTERVAL_US);
  Serial.print("us\nSamples in recording: ");
  Serial.println(RECORDER_SAMPLES);
  Serial.print("Bytes in recording: ");
  Serial.println(RECORDER_BYTES);

  while(1) {
    // Grab current time
    time = micros();
    
    // Sample from the antenna
    uint8_t val = readRxPin();
    
    // Store into the correct sample buffer
    pushSample(val);
    
    // Correct time offset due to computations
    dur = micros() - time;
    if(dur > RX_SAMPLE_INTERVAL_US) break;
    wait = RX_SAMPLE_INTERVAL_US - dur;
    // Overflow protection
    //if(wait > RX_SAMPLE_INTERVAL_US) break;
    
    // Wait for the next sampling point
    delayMicroseconds(wait);
  }
  
  Serial.print("Error: could not compensate for computation time.\nDuration in us: ");
  Serial.print(dur);
  Serial.print("\nTarget in us: ");
  Serial.println(RX_SAMPLE_INTERVAL_US);
  while(1) {}
}
