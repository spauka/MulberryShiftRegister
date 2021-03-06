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

#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "project.h"
#include "globals.h"
#include "usb_utils.h"
#include "switch.h"

const char* parity[] = {"None", "Odd", "Even", "Mark", "Space"};
const char* stop[]   = {"1", "1.5", "2"};

const cmd_map_t commands[] = {
	{"NOOP", CMD_NOOP},
	{"CLEAR", CMD_CLEAR},
	{"WRITE", CMD_WRITE},
	{"SELECT", CMD_SELECT},
	{"LOAD", CMD_LOAD},
	{"CLOCK", CMD_CLOCK},
	{"STOP", CMD_STOP}
};

/**
 * Initialize USB buffer
 */
usb_status_t init_usb_buffer(usb_buf_t *buf) {
    memset(buf->buf, 0x00, USBUART_BUFFER_SIZE);
    buf->buf_size = 0;
    buf->overflow = 0;
    return USB_SUCCESS;
}

/**
 * Check for a USB configuration change from the host.
 */
usb_status_t check_usb_uart_config_change(void) {
    // Configure the USB UART interface if the host sends a change configuration request
    if (USBUART_IsConfigurationChanged())
    {
        /* Initialize IN endpoints when device is configured. */
        if (USBUART_GetConfiguration())
        {
            /* Enumeration is done, enable OUT endpoint to receive data 
             * from host. */
            USBUART_CDC_Init();

            return USB_CONFIG_CHANGED;
        }
    }
    return USB_SUCCESS;
}

/**
 * Send data over USB CDC. Wait until the USB device is ready before putting data.
 */
usb_status_t write_usb(uint8_t *buffer, size_t len) {
	// Wait until the usb device is ready
	while (USBUART_CDCIsReady() == USB_NOT_READY);

	// Check if we want to write a zero-length packet
	if (len == 0) {
		USBUART_PutData(NULL, 0);
		return USB_SUCCESS;
	}

	// Check that the buffer is not NULL, and we won't overflow
	if (buffer == NULL)
		return USB_INVALID_BUF;
	if (len > 64)
		return USB_BUF_OVERFLOW;

	// Otherwise write data
	USBUART_PutData(buffer, len);
	/* If the packet is exactly the length of the buffer, we put a zero
	 * length packet to ensure that the end of segment is properly 
	 * identified by the host */
	if (len == USBUART_BUFFER_SIZE)
		return write_usb(NULL, 0);

	// Succcessful write :)
	return USB_SUCCESS;
}

/**
 * Read data in from the USB device and place data into usb_input_buffer
 */
usb_status_t read_usb_data(usb_buf_t *usb_input_buffer) {
    uint8_t buffer[USBUART_BUFFER_SIZE];
    /* Check for input data from host. */
    if (USBUART_DataIsReady())
    {
    	size_t count = 0;
        /* Read received data and re-enable OUT endpoint. */
        count = USBUART_GetAll(buffer);
        if (count > 0)
        {
            // Store the received data in the UART buffer, unless there is an overflow
            // In the case of an overflow, we'll just reset the buffer
            // The overflow flag is set such that we can ignore the next (partial)
            // message.
            if ((count + usb_input_buffer->buf_size) > USBUART_BUFFER_SIZE) {
                usb_input_buffer->buf_size = 0;
                usb_input_buffer->overflow = 1;
                return USB_BUF_OVERFLOW;
            }
            memcpy(usb_input_buffer->buf + usb_input_buffer->buf_size, buffer, count);
            usb_input_buffer->buf_size += count;
        }
    }
    return USB_SUCCESS;
}

/**
 * Parse input buffer looking for completed lines.
 */
usb_status_t parse_usb_buffer(usb_buf_t *usb_input_buffer, switches_t *state) {
    const size_t term_len = sizeof(term)/sizeof(char) - 1;

    // Look for terminators
    if (usb_input_buffer->buf_size < term_len)
        return USB_SUCCESS;
    for (size_t i = 0; i < usb_input_buffer->buf_size - (term_len-1); i += 1) {
        if (memcmp(usb_input_buffer->buf + i, term, term_len) == 0) {
            // Ignore if the last command overflowed, we may be looking at a partial message
            if (usb_input_buffer->overflow == 0) {
                // We've found a terminator, handle the command
                usb_input_buffer->buf[i] = '\0';
                write_usb((uint8_t *)"Command: \"", 11);
                write_usb((uint8_t *)usb_input_buffer->buf, i - (term_len-1));
                write_usb((uint8_t *)"\"\r\n", 3);

                // Execute the command
            	do_command(usb_input_buffer->buf, state);
            } else {
            	// If we had an overflow on the last command, ignore the potentially
            	// partial message
                usb_input_buffer->overflow = 0;
            }

            // Shuffle the buffer down
            const char *eoc = usb_input_buffer->buf + i + (term_len-1);
            const size_t len_remain = usb_input_buffer->buf_size - i - (term_len);
            memmove(usb_input_buffer->buf, eoc, len_remain);
            usb_input_buffer->buf_size = len_remain;

            // Reset the loop to the beginning of the list
            i = -1;
        }
    }
    return USB_SUCCESS;
}

/**
 * Extract parameters from a command
 */
usb_status_t extract_params(char *buffer, command_t *cmd, size_t *argc, char **argv) {
    // First, figure out the length of the command
    size_t len = strlen(buffer);
    
    // Next, loop through the string looking for whitespace
    size_t argc_actual = 0;

    // The first argument starts at zero
    argv[0] = buffer;
    argc_actual = 1;
    for (size_t i = 0; i < len; i += 1) {
    	// If we hit a whitespace character, we have a new argument
    	if (isspace(buffer[i])) {
    		// This becomes a whitespace
    		buffer[i] = '\0';
    		// Loop until the next non-whitespace character
    		while(isspace(buffer[i]) || buffer[i] == '\0') {
    			i += 1;
    			if (i >= len) break; // Hit end of buffer, stop here
    		}
    		if (i >= len) break; // Hit end of buffer, stop here
    		// Otherwise add to list of arguments
    		// Check that we aren't going to overflow the list of arguments
    		if (argc_actual >= *argc)
    			return USB_INVALID_NUM_ARGS;
    		argv[argc_actual] = buffer+i;
    		argc_actual += 1;
    		i -= 1;
    	}
    }
    // Fill in argc with correct number of commands
    *argc = argc_actual;

    // We should now have a list of arguments.
    // Let's figure out what the command is
    char *cmd_str = argv[0];
    size_t n_cmds = sizeof(commands)/sizeof(cmd_map_t);
    *cmd = CMD_INVALID;
    for (size_t i = 0; i < n_cmds; i += 1) {
    	if (strcasecmp(cmd_str, commands[i].cmd_str) == 0) {
    		*cmd = commands[i].cmd;
    		break;
    	}
    }
    if (*cmd == CMD_INVALID)
    	return USB_INVALID_CMD;

    return USB_SUCCESS;
}

/**
 * Parse command
 */
uint8_t hex_start[] = {'0', 'x'};
usb_status_t do_command(char* buffer, switches_t *state) {
    command_t cmd = CMD_NOOP;
    size_t argc = USB_CMD_MAX_ARGS;
    char *argv[USB_CMD_MAX_ARGS] = {0};

    // Create a buffer to send over SPI
    uint8_t out_buffer[SPIM_TX_BUFFER_SIZE];

    // Extract the command from the command line
    extract_params(buffer, &cmd, &argc, argv);

    // Switch on the extracted command
    switch(cmd) {
    case CMD_NOOP:
    	break;
    case CMD_CLEAR:
        switches_all(state, 0x00);
        switches_pack(state, out_buffer);
        if (CLK_OUT == 0) {
        	PULSE_LD = 1;
        	SPIM_PutArray(out_buffer, SPIM_TX_BUFFER_SIZE);
        }
        else
        	return USB_CLOCK_ON;
        break;
    case CMD_WRITE:
    	if (argc != 2) // Must be a single command + argument
    		return USB_INVALID_NUM_ARGS;
    	if (memcmp(argv[1], hex_start, 2) == 0) // If we start with "0x" move pointer past this
    		argv[1] += 2;
    	return USB_NOT_IMPLEMENTED;
    case CMD_SELECT:
    	if (argc != 2) // Must be a single command + argument
    		return USB_INVALID_NUM_ARGS;
    	if (strlen(argv[1]) != 1) // Must be a single character
    		return USB_INVALID_ARG;
    	switches_all(state, switches_mask(argv[1][0]));
    	switches_pack(state, out_buffer);
    	if (CLK_OUT == 0) {
    		PULSE_LD = 1;
    		SPIM_PutArray(out_buffer, SPIM_TX_BUFFER_SIZE);
    	}
    	else
    		return USB_CLOCK_ON;
        break;
    case CMD_LOAD:
    	if (CLK_OUT == 0)
    		LD_PulseGen_Write(1u);
    	else
    		return USB_CLOCK_ON;
    	break;
    case CMD_CLOCK:
    	CLK_OUT = 1;
   		break;
   	case CMD_STOP:
   		CLK_OUT = 0;
   		SPIM_ClearTxBuffer();
   		break;
    default:
        return USB_INVALID_CMD;
    }
    return USB_SUCCESS;
}