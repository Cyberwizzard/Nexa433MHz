#include "config.h"

// Configure the design
void setup() {
  // Enable serial debugging
  Serial.begin(9600);
  
  // Configure the pins used by this program
  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  
  // Speed up the ADC so it can keep up
  set_ADC_speed();
  
  Serial.print("Nexa RF - ");
  #if ENABLE_FULL_DECODER
  Serial.println("decoder module");
  #endif
  #if ENABLE_DEBUG_DECODER
  Serial.println("debug module");
  #endif
  #if ENABLE_RECORDER
  Serial.println("recorder module");
  #endif
}

void loop() {
  #if ENABLE_FULL_DECODER
  decoder_loop();
  #endif
  
  #if ENABLE_DEBUG_DECODER
  debug_decoder_loop();
  #endif
  
  #if ENABLE_RECORDER
  recorder_loop();
  #endif
  
  // Make sure we do not restart the program if something went very wrong
  Serial.println("FAIL");
  while(1) {}
}
