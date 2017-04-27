/**
 * Recorder module settings - used for recording a short bit stream from the antenna (digitized)
 */
 
#ifndef _RECORDER_H_
#define _RECORDER_H_

// Include the protocol for the sampling settings
#include "protocol.h"

// Length of the recording in seconds - note that due to the limited RAM size, only short traces can be made (also depending on the sample rate, which is 50 us by default)
// Default: 0.5f for 500ms recording (note that the cast later on prevents the need for float support)
#define RECORDER_RECORD_SECONDS 0.5f

// Compute how many samples are needed for the requested duration
#define RECORDER_SAMPLES ((int)((RECORDER_RECORD_SECONDS * 1000000) / RX_SAMPLE_INTERVAL_US))

// Compute how many bytes are needed for the trace
#define RECORDER_BYTES (RECORDER_SAMPLES / 8)

/**
 * Main control loop for the recorder logic
 */
void recorder_loop();

#endif
