/**
 * NEXA protocol decoder - shared logic
 */

#ifndef _DECODER_H_
#define _DECODER_H_

#include <stdint.h>

// Events from the detector
#define EVENT_NONE 0
#define EVENT_INVALID 1
#define EVENT_HIGH_SHORT 10
#define EVENT_LOW_SHORT 11
#define EVENT_LOW_LONG 12
#define EVENT_SYNC 13
#define EVENT_PAUSE 14

/**
 * Utility function to detect various pulse types; works on a sample stream so we do not need to store a lot of samples while decoding the stream.
 *
 * @return the event code for the current sample given
 */
uint8_t detectPulse(uint8_t val);

#endif
 
