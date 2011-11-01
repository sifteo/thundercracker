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
 * Macros for use in the filtering ISR code.
 */
 
// Power down ADC, and disable our interrupt until next time.
#define ADC_ISR_DONE() {                \
        ADCCON1 = TOUCH_ADC_CH << 2;    \
        IEN_MISC = 0;                   \
    }

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
volatile uint8_t debug_touch = 0;
#endif
 
 /*
 * Touch sensor filtering.
 *
 * A lot of the touch setup is, by necessity, done in sensors.c. But here we
 * implement the A/D converter ISR which filters the raw capacitance data and
 * gives us a touch signal that can be sent back over the radio.
 *
 * This ISR signals the end of a touch sensor sample. We only enable the ADC interrupt
 * when we're doing the meaningful portion of a touch capacitance measurement.
 *
 * This is less performance critical, and doesn't have to directly interoperate with
 * any timing- or register-critical code, so it can be in C. 
 */
 
void adc_isr(void) __interrupt(VECTOR_MISC)
{
    static __bit state_pressed, state_timeout;
    static uint8_t touch_timestamp;
    
    // Ignore high byte; the signals we're interested in are small.
    uint8_t value = ADCDATL;
    
#ifdef DEBUG_TOUCH
    debug_touch = value;
#endif
    
    /*
     * Filter goes here :)
     * This is a stub that is only intended to work with the simulator.
     */
    
    if (state_pressed) {
        // Touch has been detected; wait for idle, and check the duration
        
        if (sensor_tick_counter - touch_timestamp > 20) {
            // Too long; not a valid touch
            state_timeout = 1;
        }        
        
        if (value > 95) {
            state_pressed = 0;
            if (!state_timeout)
                TOUCH_DETECTED();
        }

    } else {
        if (value < 85) {
            // Finger on cube
            state_pressed = 1;
            state_timeout = 0;
            touch_timestamp = sensor_tick_counter;
        }
    }

    ADC_ISR_DONE();
}
