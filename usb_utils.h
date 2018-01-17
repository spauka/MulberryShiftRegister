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

#ifndef __USB_UTILS_H__
#define __USB_UTILS_H__

#include <stddef.h>
#include <stdint.h>
    
typedef enum {
	USB_NOT_CONFIGURED = 0,
	USB_NOT_READY = 0,
	USB_BUF_OVERFLOW = 1,
	USB_INVALID_BUF = 2,
	USB_OTHER_FAIL = 0x7F,
	USB_SUCCESS = 0xFF
} usb_status_t;

// Define buffer parameters
static const uint32_t USBFS_DEVICE = 0u;
#define USBUART_BUFFER_SIZE (64u)

// Define CDC properties
extern const char* parity[];
extern const char* stop[];

// Term character
static const char term[] = "\r";

typedef struct {
    char buf[USBUART_BUFFER_SIZE];
    size_t buf_size;
} usb_buf_t;

// Define an instance of the circular buffer
extern usb_buf_t usb_input_buffer;

/**
 * Write USB Data, blocking until the device is ready.
 */
usb_status_t write_usb(uint8_t *buffer, size_t len);

/**
 * Check for a USB configuration change from the host.
 */
usb_status_t check_usb_uart_config_change(void);

/**
 * Read data in from the USB device.
 */
usb_status_t read_usb_data(void);

/**
 * Parse the USB buffer for completed commands
 */
usb_status_t parse_usb_buffer(void);

#endif