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
 * RF test application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
    SysTime::init();
    PowerManager::init();
    HomeButton::init();
    Radio::init();

    // wait for 2nd power on delay - this is normally done via the heartbeat task
    while (SysTime::ticks() < SysTime::msTicks(210));

    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);
    UART(("RF Test\r\n"));

    /*
     * Cycle through constant carrier and PRX on the given channels when
     * we detect a button press.
     */
    uint8_t channelIdx = 0;
    const uint8_t channels[4] = { 2, 40, 80, 128 };     // channel>127 => PRX

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
                NRF24L01::instance.setPRXMode(false);
                NRF24L01::instance.setConstantCarrier(false);

                green.setLow();
                channelIdx = (channelIdx + 1) % sizeof(channels);
                if (channels[channelIdx] < 128) {
                    NRF24L01::instance.setConstantCarrier(true, channels[channelIdx]);
                } else {
                    NRF24L01::instance.setPRXMode(true);
                }
            } else {
                green.setHigh();
            }

            lastButton = button;
        }
    }
}
