/**
 * Configuration for the NEXA 433 MHz protocol
 *
 * Note that this config contains the official defaults (as found online) as well as
 * manually tweaked values for the specific setup.
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

// Sampler settings are needed to compute time to sample conversions
#include "sampler.h"

// --------- Utility defines ----------------
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

// --------- NEXA protocol settings --------- 
// Timing according to: https://github.com/mikaelpatel/Cosa-NEXA/blob/master/NEXA.hh
//
// Packet start: SHORT HIGH + START PULSE LOW
// Data: SHORT HIGH + SHORT LOW = 1
//       SHORT HIGH + LONG LOW  = 0
//       32 bits
// 

#define SHORT_PULSE 275
#define LONG_PULSE 1225
#define START_PULSE (2675 - SHORT_PULSE)

// Define how many samples we allow to be 'wrong' due to the rising or falling edges
// Note: this helps with a noisy receiver but also degrades the precision.
// Fuzzy matching on the 'short' pulses
#define FUZZY_SAMPLES_SHORT 1
// Fuzzy matching on the 'long' and 'sync' pulses
#define FUZZY_SAMPLES_LONG 2

// Number of bits in a packet (between the SYNC and PAUSE signals)
#define PAYLOAD_SIZE_BITS 64

// --------- Utility defines --------- 
// Because we will sample multiple times per pulse, compute how many samples make up a 'short' and a 'long' pulse
//#define SHORT_HIGH_PULSE_SAMPLES (SHORT_PULSE / RX_SAMPLE_INTERVAL_US)
//#define SHORT_LOW_PULSE_SAMPLES  (SHORT_PULSE / RX_SAMPLE_INTERVAL_US)
//#define LONG_PULSE_SAMPLES       (LONG_PULSE  / RX_SAMPLE_INTERVAL_US)
//#define START_PULSE_SAMPLES      (START_PULSE / RX_SAMPLE_INTERVAL_US)

#define SHORT_HIGH_PULSE_SAMPLES 4
#define SHORT_LOW_PULSE_SAMPLES  6
#define LONG_PULSE_SAMPLES       24
#define START_PULSE_SAMPLES      50

// A lot of low pulses after a frame denotes the end of the frame - a bit more than the SYNC pulse will do
#define END_PULSE_SAMPLES   (START_PULSE_SAMPLES + SHORT_LOW_PULSE_SAMPLES + FUZZY_SAMPLES_LONG)

/* http://tech.jolowe.se/home-automation-rf-protocols/
Packetformat
Every packet consists of a sync bit followed by 26 + 2 + 4 (total 32 logical data part bits) and is ended by a pause bit.

S HHHH HHHH HHHH HHHH HHHH HHHH HHGO CCEE P

S = Sync bit.
H = The first 26 bits are transmitter unique codes, and it is this code that the reciever "learns" to recognize.
G = Group code. Set to 0 for on, 1 for off.
O = On/Off bit. Set to 0 for on, 1 for off.
C = Channel bits. Proove/Anslut = 00, Nexa = 11.
E = Unit bits. Device to be turned on or off.
Proove/Anslut Unit #1 = 00, #2 = 01, #3 = 10.
Nexa Unit #1 = 11, #2 = 10, #3 = 01.
P = Pause bit.

For every button press, N identical packets are sent. For Proove/Anslut N is six, and for Nexa it is five.
*/

typedef struct {
  uint8_t  unit      : 2;
  uint8_t  channel   : 2;
  uint8_t  on_off    : 1;
  uint8_t  group     : 1;
  uint32_t device_id : 26;
} __attribute__((packed)) nexa_pckt_t;


#endif
