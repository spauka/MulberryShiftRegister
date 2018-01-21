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

#include "switch.h"
#include "usb_utils.h"

#include <string.h>
#include <stdio.h>

/**
 * Set the state of all switches to a given state
 */
void switches_all(switches_t *switches, uint8_t state) {
	for (size_t i = 0; i < NUM_SWITCHES; i += 1)
		switches->switches[i].byte = state;
}

/** 
 * Return the bitmask corresponding to a given switch char:
 * i.e. A = 1, B = 2, C = 4 ...
 * For an invalid character, return 0.
 */
uint8_t switches_mask(uint8_t c) {
    switch (c) {
    case 'a':
    case 'A':
        return 16;
    case 'b':
    case 'B':
        return 8;
    case 'c':
    case 'C':
        return 4;
    case 'd':
    case 'D':
        return 2;
    case 'e':
    case 'E':
        return 1;
    default:
        return 0;
    }
}

/**
 * Pack switches into a 20-byte string to be sent over SPI
 * args:
 *      state: the state of all switches
 *      out_buffer: pointer to at least 20 bytes of memory which will contain
 *          the raw data to be sent over SPI
 */
typedef union {
	uint64_t ld;
	uint8_t b[8];
} pack_t;
void switches_pack(switches_t *switches, uint8_t *out_buffer) {
	// Handle each packing in groups of 40 bits (8 switches)
	// First let's clear the buffer
	memset(out_buffer, 0x00, 20);

	// Then in groups of 8, iterate over the switch matrix
	for (size_t i = 0; i < NUM_SWITCHES/8; i += 1) {
		pack_t data;
		data.ld = 0;
		for (size_t j = 0; j < 8; j += 1) {
			data.ld |= ((uint64_t)(switches->switches[NUM_SWITCHES - (i*8 + j) - 1].byte & 0x1F)) << (j*5 + (1-i/2));
		}
		uint8_t *out_buffer_off = out_buffer + 5*i;
		for (size_t j = 0; j < 6; j += 1) {
			out_buffer_off[j] |= data.b[j];
		}
	}
	return;
}