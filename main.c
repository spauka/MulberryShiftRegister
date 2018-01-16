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

#include "project.h"

#include "SPIM.h"

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

#define USBFS_DEVICE    (0u)

/* The buffer size is equal to the maximum packet size of the IN and OUT bulk
* endpoints.
*/
#define USBUART_BUFFER_SIZE (64u)
#define LINE_STR_LENGTH     (20u)
char8* parity[] = {"None", "Odd", "Even", "Mark", "Space"};
char8* stop[]   = {"1", "1.5", "2"};

int main(void)
{
    uint8_t buffer[USBUART_BUFFER_SIZE];
    uint32_t count = 0;
    
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Start the SPI interface */
    uint8_t data_out[SPIM_TX_BUFFER_SIZE];
    memset(data_out, 0x01, SPIM_TX_BUFFER_SIZE);
    SPIM_Start();
    
    /* Start USBFS operation with 5-V operation. */
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);

    for(;;)
    {
        /* Host can send double SET_INTERFACE request. */
        if (USBUART_IsConfigurationChanged())
        {
            /* Initialize IN endpoints when device is configured. */
            if (USBUART_GetConfiguration())
            {
                /* Enumeration is done, enable OUT endpoint to receive data 
                 * from host. */
                USBUART_CDC_Init();
            }
        }

        /* Service USB CDC when device is configured. */
        if (USBUART_GetConfiguration())
        {
            /* Check for input data from host. */
            if (USBUART_DataIsReady())
            {
                /* Read received data and re-enable OUT endpoint. */
                count = USBUART_GetAll(buffer);

                if (count > 0)
                {
                    /* Wait until component is ready to send data to host. */
                    while (USBUART_CDCIsReady() == 0);

                    /* Send data back to host. */
                    USBUART_PutData(buffer, count);
                    
                    /* And write it out to SPI too */
                    for (uint32_t i = 0; i < count; i += 20) {
                        SPIM_PutArray(buffer+i, min(count-i, 20));
                    }

                    /* If the last sent packet is exactly the maximum packet 
                    *  size, it is followed by a zero-length packet to assure
                    *  that the end of the segment is properly identified by 
                    *  the terminal.
                    */
                    if (USBUART_BUFFER_SIZE == count)
                    {
                        /* Wait until component is ready to send data to PC. */
                        while (0u == USBUART_CDCIsReady())
                        {
                        }

                        /* Send zero-length packet to PC. */
                        USBUART_PutData(NULL, 0u);
                    }
                }
            }
        }
    }
}

/* [] END OF FILE */
