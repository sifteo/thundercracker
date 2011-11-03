/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "hardware.h"
#include "params.h"


void params_init()
{
    /*
     * Generate a hardware ID, if necessary.
     *
     * More accurately, any HWID bytes that are not yet programmed
     * get programmed with a true random value. So, if there's a power fail
     * during this operation, we'll continue where we left off.
     *
     * MUST be called before interrupts are enabled. We don't want
     * to enter ISRs while FSR_WEN is set!
     */
     
    uint8_t i;
     
    // Power on RNG, with bias corrector
    RNGCTL = 0xC0;
    
    for (i = 0; i < HWID_LEN; i++)
        if (params.hwid[i] == 0xFF) {
            uint8_t b;

            /*  
             * IDs can't have 0xFF bytes, since that is indistinguishable
             * from unprogrammed flash. Cycle the hardware RNG until we get
             * a byte that isn't 0xFF.
             */
            do {
                while (!(RNGCTL & 0x20));
                b = RNGDAT;
            } while (b == 0xFF);
                
            FSR_WEN = 1;
            params.hwid[i] = b;
            FSR_WEN = 0;
        }
            
    // RNG to sleep now!
    RNGCTL = 0;
}