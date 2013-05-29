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
    static const unsigned SWEEP_CHAN_MAX = 83;

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

#ifdef HAVE_NRF8001
class NRFBLETest {
    static void waitForCompletion() {
        SysTime::Ticks t = SysTime::ticks() + SysTime::msTicks(500);
        while(NRF8001::instance.testStatus == NRF8001::instance.Pending) {
            if (SysTime::ticks() > t) {
                UART("NRFBLETest: timeout on DTM command\r\n");
                break;
            }
        }
        UART("RFTest status: "); UART_HEX(NRF8001::instance.testStatus); UART("\r\n");
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
        idle();
        enterTestMode();
        //TODO: need to implement
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
                        UART("8001: txlow\r\n");
                        red.setLow();
                        NRF24L01Test::idle();
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLETxMid:
                        UART("8001: txmid\r\n");
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLETxHigh:
                        UART("8001: txhigh\r\n");
                        NRFBLETest::carrier();
                        break;

                    case RfTest::BLERX:
                        UART("8001: rx\r\n");
                        NRFBLETest::rx();
                        break;

                    case RfTest::BLETxSweep:
                        UART("8001: txsweep\r\n");
                        NRFBLETest::txSweep();
                        break;
#endif //HAVE_NRF8001
                }

                RfTest::gState = (RfTest::gState + 1) % RfTest::NUM_STATES;
            }
        }
    }
}
