/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "hardware.h"
#include "board.h"
#include "powermanager.h"
#include "systime.h"
#include "radio.h"
#include "nrf24l01.h"
#include "homebutton.h"

/*
 * Bootloader application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
    PowerManager::init();

    SysTime::init();
    SysTime::Ticks start = SysTime::ticks();

    HomeButton::init();

    while (SysTime::ticks() - start < SysTime::msTicks(110))
        ;
    Radio::init();

    /*
     * Cycle through constant carrier on the given channels when
     * we detect a button press.
     */
    uint8_t channelIdx = 0;
    const uint8_t channels[3] = { 2, 40, 80 };

    NRF24L01::instance.setConstantCarrier(true, channels[channelIdx]);
    bool lastButton = HomeButton::isPressed();

    GPIOPin green = LED_GREEN_GPIO;
    green.setControl(GPIOPin::OUT_2MHZ);
    green.setHigh();

    for (;;) {

        bool button = HomeButton::isPressed();

        if (lastButton != button) {

            // transition on press
            if (button == true) {
                NRF24L01::instance.setConstantCarrier(false);

                green.setLow();
                channelIdx = (channelIdx + 1) % sizeof(channels);
                NRF24L01::instance.setConstantCarrier(true, channels[channelIdx]);
            } else {
                green.setHigh();
            }

            lastButton = button;
        }
    }
}
