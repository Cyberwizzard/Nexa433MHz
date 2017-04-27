/**
 * NEXA protocol decoder - debug decoder which outputs an event stream to show what was received
 */

#ifndef _DECODER_DEBUG_H_
#define _DECODER_DEBUG_H_

/**
 * Debug decoder loop: read a sample, push it through the detection logic, wait - every now and then a printout is done to show the state of the recorded samples
 */
void debug_decoder_loop();

#endif
 
