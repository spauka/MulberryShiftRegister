/*
The MIT License (MIT)

Copyright (c) 2017 Sebastian Pauka

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef __SWITCH_H__
#define __SWITCH_H__
    
#include <stddef.h>
#include <stdint.h>

#define NUM_SWITCHES (32u)

/* Define a struct for each switch */
typedef struct __attribute__((packed)) {
    union {
        uint8_t byte;
    	struct __attribute__((packed)) {
            uint8_t e:1;
            uint8_t d:1;
    		uint8_t c:1;
            uint8_t b:1;
            uint8_t a:1;
            uint8_t pack:3;
    	};
    } switches[NUM_SWITCHES];
} switches_t ;

/**
 * Set all switches to a given state
 * args:
 *      state: a bitfield from 0 to 0x1F representing the state
 *      of the 5 switches (0 = all off, 1 = E, 2 = D, 3 = E+D ...)
 */
void switches_all(switches_t *switches, uint8_t state);

/**
 * Pack switches into a 20-byte string to be sent over SPI
 * args:
 *      state: the state of all switches
 *      out_buffer: pointer to at least 20 bytes of memory which will contain
 *          the raw data to be sent over SPI
 */
void switches_pack(switches_t *switches, uint8_t *out_buffer);

/** 
 * Return the bitmask corresponding to a given switch char:
 * i.e. A = 1, B = 2, C = 4 ...
 * For an invalid character, return 0.
 */
uint8_t switches_mask(uint8_t c); 

#endif