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

#include <stdio.h>

// Include generated header files
#include "project.h"

#include "usb_utils.h"
#include "switch.h"

/**
 * Returns the minimum of two values
 */
uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

/**
 * Write out a pulse on the LD line once the buffer has been cleared
 */
void SPIM_TX_ISR_ExitCallback(void) {
    if (SPIM_TX_STATUS_REG & SPIM_STS_SPI_DONE) {
        LD_PulseGen_Write(1u);
    }
}


int main(void)
{
    switches_t switch_states;

    // Buffer used to store USB commands
    usb_buf_t usb_input_buffer;

    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Start the SPI interface */
    uint8_t data_out[SPIM_TX_BUFFER_SIZE];
    memset(data_out, 0x01, SPIM_TX_BUFFER_SIZE);
    SPIM_Start();

    /* Empty the USB input buffer */
    init_usb_buffer(&usb_input_buffer);
    
    /* Start USBFS operation with 5-V operation. */
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);

    for(;;)
    {
        // Check for a USB UART config change
        // If it has changed, reset the buffer, this is an indication
        // that the connected has restarted
        if (check_usb_uart_config_change() == USB_CONFIG_CHANGED) {
        }

        // Read in USB data
        if (USBUART_GetConfiguration() != USB_NOT_CONFIGURED) {
            read_usb_data(&usb_input_buffer);

            // Check whether there is a command terminator in the buffer
            parse_usb_buffer(&usb_input_buffer, &switch_states);
        }
    }
}

/* [] END OF FILE */
