/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
#include "macros.h"
#include "sampleprofiler.h"

namespace RfTest {
    enum State {
        L01TxLow,
        L01TxMid,
        L01TxHigh,
        L01Rx,
        L01TxSweep,
#ifdef HAVE_NRF8001
        BLETxLow,
        BLETxMid,
        BLETxHigh,
        BLERX,
        BLETxSweep,
#endif //HAVE_NRF8001
        NUM_STATES  // must be last
    };

    unsigned gState;
}

class NRF24L01Test {
public:
    static const uint8_t channels[3];

    static void idle() {
        NRF24L01::instance.setConstantCarrier(false);
        NRF24L01::instance.setPRXMode(false);
    }

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
            for (unsigned i = channels[0]; i <= channels[2]; i++) {
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

#ifdef HAVE_NRF8001
class NRFBLETest {
    static void waitForCompletion() {
        SysTime::Ticks t = SysTime::ticks() + SysTime::msTicks(500);
        while(SysTime::ticks() < t);
    }

    static void enterTestMode() {
        NRF8001::instance.test(NRF8001::EnterTestMode, 0, 0, 0);
        waitForCompletion();
    }

public:
    static const uint8_t channels[3];

    static void idle() {
        NRF8001::instance.test(NRF8001::ExitTestMode, 0, 0, 0);
        waitForCompletion();
    }

    static void carrier() {
        idle();
        enterTestMode();
        NRF8001::instance.test(NRF8001::TXTest, 3, 0, channels[RfTest::gState - RfTest::BLETxLow]);
        waitForCompletion();
    }

    static void rx() {
        idle();
        enterTestMode();
        NRF8001::instance.test(NRF8001::RXTest, 0, 0, 0);
        waitForCompletion();
    }

    static void txSweep() {
        for (;;) {
            for (unsigned i = channels[0]; i <= channels[2]; i++) {
                //UART("Sweep-ch: "); UART_HEX(i); UART("\r\n");
                idle();
                enterTestMode();
                NRF8001::instance.test(NRF8001::TXTest, 3, 0, i);
                SysTime::Ticks t = SysTime::ticks() + SysTime::msTicks(500);
                while (SysTime::ticks() < t) {
                    if (HomeButton::isPressed()) {
                        return;
                    }
                }
            }
        }
    }
};

const uint8_t NRFBLETest::channels[3] = { 0, 20, 39 };  //valid range: 0x00 - 0x27, N = (F-2402)/2
#endif //HAVE_NRF8001

/*
 * RF test application specific entry point.
 * Lower level init happens in setup.cpp.
 */
int main()
{
#ifdef BOOTLOADABLE
    NVIC.setVectorTable(NVIC.VectorTableFlash, Bootloader::SIZE);
#endif

#ifdef HAVE_NRF8001
    NVIC.irqEnable(IVT.NRF8001_EXTI_VEC);               // BTLE controller IRQ
    NVIC.irqPrioritize(IVT.NRF8001_EXTI_VEC, 0x78);     //  a little higher than radio, just below USB

    NVIC.irqEnable(IVT.NRF8001_DMA_CHAN_RX);            // BTLE SPI DMA channels
    NVIC.irqPrioritize(IVT.NRF8001_DMA_CHAN_RX, 0x74);  //  same prio as flash for now
    NVIC.irqEnable(IVT.NRF8001_DMA_CHAN_TX);
    NVIC.irqPrioritize(IVT.NRF8001_DMA_CHAN_TX, 0x74);
#endif // HAVE_NRF8001

    SysTime::init();
    PowerManager::init();
    HomeButton::init();
    SampleProfiler::init();

    Radio::init();

#ifdef HAVE_NRF8001
    NRF8001::instance.init();
#endif

    // wait for 2nd power on delay - this is normally done via the heartbeat task
    while (SysTime::ticks() < SysTime::msTicks(210));

    Usart::Dbg.init(UART_RX_GPIO, UART_TX_GPIO, 115200);
    UART(("RF Test\r\n"));

    bool lastButton = HomeButton::isPressed();

    GPIOPin green = LED_GREEN_GPIO;
    green.setHigh();
    green.setControl(GPIOPin::OUT_2MHZ);
    GPIOPin red = LED_RED_GPIO;
    red.setHigh();
    red.setControl(GPIOPin::OUT_2MHZ);

    RfTest::gState = RfTest::L01TxLow;

    for (;;) {

        bool button = HomeButton::isPressed();

        if (lastButton != button) {

            lastButton = button;
            green.toggle();

            // transition on release
            if (!button) {
                switch (RfTest::gState) {
                    case RfTest::L01TxLow:
                        red.setHigh();
                        NRFBLETest::idle();
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

#ifdef HAVE_NRF8001
                    case RfTest::BLETxLow:
                        red.setLow();
                        NRF24L01Test::idle();
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLETxMid:
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLETxHigh:
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLERX:
                        NRFBLETest::rx();
                        break;

                    case RfTest::BLETxSweep:
                        NRFBLETest::txSweep();
                        break;
#endif //HAVE_NRF8001
                }

                RfTest::gState = (RfTest::gState + 1) % RfTest::NUM_STATES;
            }
        }
    }
}
