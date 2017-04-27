/**
 * Configuration for the NEXA 433 MHz decoder program
 *
 * For debugging purposes this program consists of multiple modules with various functionality.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/**
 * Module: RX decoder for Nexa protocol
 *
 * Full NEXA decoder program - detects packets from wall switches or hand-held remotes.
 */
#define ENABLE_FULL_DECODER 1

/**
 * Module: low level NEXA protocol decoder
 *
 * Records events into a memory buffer and prints them to the serial console for further debugging
 */
#define ENABLE_DEBUG_DECODER 0

/**
 * Module: Analog sample recording + printout
 * 
 * Record at nominal sample speed from the receiver and compact to one bit per bit from
 * the radio. Useful for plotting and debugging in Octave.
 */
#define ENABLE_RECORDER 0

// ------------------------- Sanity tests --------------------------
#if (ENABLE_FULL_DECODER + ENABLE_DEBUG_DECODER + ENABLE_RECORDER) > 1
#error "Select exactly one of the modules to compile!"
#endif

#if (ENABLE_FULL_DECODER + ENABLE_DEBUG_DECODER + ENABLE_RECORDER) == 0
#error "No module enabled!"
#endif

// ------------------------- Selective inclusion --------------------

// Sampler settings (used in all modules)
#include "sampler.h"
// Sample settings and protocol settings
#include "protocol.h"
// ADC control functions (for faster sampling)
#include "adc.h"

#if ENABLE_RECORDER
#include "recorder.h"
#endif

#if ENABLE_FULL_DECODER
#include "decoder_full.h"
#endif

#if ENABLE_DEBUG_DECODER
#include "decoder_debug.h"
#endif

#endif
