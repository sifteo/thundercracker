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
#include "nrf8001/nrf8001.h"
#include "homebutton.h"
#include "bootloader.h"

namespace RfTest {
    enum State {
        L01TxLow,
        L01TxMid,
        L01TxHigh,
        L01Rx,
        L01TxSweep,
        NUM_STATES  // must be last
    };

    unsigned gState;
}

class NRF24L01Test {
public:
    static const uint8_t channels[3];
    static const unsigned SWEEP_CHAN_MAX = 83;

    static void carrier() {
        NRF24L01::instance.setConstantCarrier(false);
        NRF24L01::instance.setConstantCarrier(true, channels[RfTest::gState - RfTest::L01TxLow]);
    }

    static void rx() {
        NRF24L01::instance.setConstantCarrier(false);
        NRF24L01::instance.setPRXMode(true);
    }

    static void txSweep() {
        NRF24L01::instance.setPRXMode(false);

        for (;;) {
            for (unsigned i = 0; i < SWEEP_CHAN_MAX; ++i) {
                NRF24L01::instance.setConstantCarrier(true, i);
                SysTime::Ticks t = SysTime::ticks() + SysTime::msTicks(100);
                while (SysTime::ticks() < t) {
                    if (HomeButton::isPressed()) {
                        NRF24L01::instance.setConstantCarrier(false);
                        return;
                    }
                }
            }
        }
    }
};

const uint8_t NRF24L01Test::channels[3] = { 4, 41, 79 };


/*
 * RF test application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{

    #ifdef BOOTLOADABLE
        NVIC.setVectorTable(NVIC.VectorTableFlash, Bootloader::SIZE);
    #endif

    SysTime::init();
    PowerManager::init();
    HomeButton::init();
    Radio::init();

    // wait for 2nd power on delay - this is normally done via the heartbeat task
    while (SysTime::ticks() < SysTime::msTicks(210));

    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);
    UART(("RF Test\r\n"));

    bool lastButton = HomeButton::isPressed();

    GPIOPin green = LED_GREEN_GPIO;
    green.setControl(GPIOPin::OUT_2MHZ);
    green.setHigh();

    for (;;) {

        bool button = HomeButton::isPressed();

        if (lastButton != button) {

            lastButton = button;
            green.toggle();

            // transition on release
            if (!button) {
                switch (RfTest::gState) {

                    case RfTest::L01TxLow:
                    case RfTest::L01TxMid:
                    case RfTest::L01TxHigh:
                        NRF24L01Test::carrier();
                        break;

                    case RfTest::L01Rx:
                        NRF24L01Test::rx();
                        break;

                    case RfTest::L01TxSweep:
                        NRF24L01Test::txSweep();
                        break;
                }

                RfTest::gState = (RfTest::gState + 1) % RfTest::NUM_STATES;
            }
        }
    }
}
