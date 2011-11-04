/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "touch.h"
#include "radio.h"
#include "sensors.h"
#include "draw.h"

/*
 * Register a touch, and queue it for the radio.
 * This should happen only once per physical touch event!
 * This is in assembly so we can ensure atomic operations are used.
 */
#pragma sdcc_hash +
#define TOUCH_DETECTED() {                                                      \
        __asm   xrl     (_ack_data + RF_ACK_NEIGHBOR + 0), #(NB0_FLAG_TOUCH)    __endasm ; \
        __asm   orl     _ack_len, #RF_ACK_LEN_NEIGHBOR                          __endasm ; \
    }

/*
 * If DEBUG_TOUCH is defined, we constantly display a variable on the LCD
 * for monitoring the raw touch sensor output.
 */
#ifdef DEBUG_TOUCH
volatile uint8_t debug_touch;
#endif


/*
 * A/D Converter ISR
 *
 * In sensors.c, we kick off touch sensing by starting an A/D reading of our
 * 2/3 Vdd reference voltage. Here we continue the touch sensing work.
 */
 
void adc_isr(void) __interrupt(VECTOR_MISC)
{
    static __bit sample_step;
    static __bit init_long_avg;
    static uint16_t long_avg;
    static int16_t fast_avg;

    if (sample_step) {
        /*
         * Second sample (Capacitive voltage division)
         */
        
        sample_step = 0;
        
        /*
         * Take a very long-term average by doing a low-gain feedback loop
         * which gradually moves touch_long_avg toward the current ADC reading.
         *
         * The ADC left-justifies our reading, which gives us a 16x boost in
         * resolution for this long-term average without having to do any extra
         * shifting. Neat.
         */

        if (init_long_avg) {
            if (long_avg < ADCDAT)
                long_avg++;
            else
                long_avg--;
        } else {
            // Don't lock-in initialization until some time has elapsed
            long_avg = ADCDAT;
            if (sensor_tick_counter & 0x10)
                init_long_avg = 1;            
        }
        
        /*
         * Now examine the difference between our instantaneous value and the
         * long-term average. This is where we'll see touches happening.
         * 
         * This data is still hella noisy, so we do another low-pass filtering
         * stage, but less agressive this time.
         *
         * XXX: Needs some work. Define DEBUG_TOUCH to play with it.
         */
        
        fast_avg += (int16_t)(long_avg - ADCDAT) - (fast_avg >> 8);
        if (fast_avg < 0)
            fast_avg = 0;
        
        #ifdef DEBUG_TOUCH
        debug_touch = fast_avg >> 8;
        #endif
        
        ADCCON1 = 0;        // Power down ADC
        IEN_MISC = 0;       // Disable ADC IRQ
        
    } else {
        /*
         * First Sample (Dummy sample, to wake up ADC)
         *
         * Charge Chold to reference again, wait for Tacq (0.75us = 12cycles).
         * Then switch to the sensor channel, and wait for another IRQ.
         * While we wait, discharge the sensor capacitance.        
         */
        
        sample_step = 1;
        
        __asm
            mov     _ADCCON1, #0xbc             ; 1 0 1111 00
            anl     _MISC_DIR, #~MISC_TOUCH     ; 4    Plate driver on
            anl     MISC_PORT, #~MISC_TOUCH     ; 3    Drive low
            orl     _MISC_DIR, #MISC_TOUCH      ; 4    Driver off
            nop			                        ; 1
            mov     _ADCCON1, #(0x80 | (TOUCH_ADC_CH << 2))
        __endasm ;
    }
}
