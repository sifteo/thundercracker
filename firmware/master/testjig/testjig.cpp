/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include "hardware.h"
#include "testjig.h"
#include "board.h"
#include "usb/usbdevice.h"
#include "tasks.h"
#include "i2c.h"
#include "neighbor.h"
#include "gpio.h"
#include "macros.h"
#include "dac.h"
#include "adc.h"

static I2CSlave i2c(&I2C1);
static Neighbor neighbor(&TIM3, &TIM5);

// control for the pass-through USB of the master under test
static GPIOPin testUsbEnable = USB_PWR_GPIO;

static Adc adc(&PWR_MEASURE_ADC);
static GPIOPin usbCurrentSign = USB_CURRENT_DIR_GPIO;
static GPIOPin v3CurrentSign = V3_CURRENT_DIR_GPIO;

/*
 * Table of test handlers.
 * Their index in this table specifies their ID.
 */
TestJig::TestHandler const TestJig::handlers[] = {
    setUsbEnabledHandler,                   // 0
    setSimulatedBatteryVoltageHandler,      // 1
    getBatterySupplyCurrentHandler,         // 2
    getUsbCurrentHandler,                   // 3
    beginNeighborRxHandler,                 // 4
    stopNeighborRxHandler,                  // 5
};

void TestJig::init()
{
    NVIC.irqEnable(IVT.TIM3);                   // neighbor tx
    NVIC.irqPrioritize(IVT.TIM3, 0x60);

    NVIC.irqEnable(IVT.TIM5);                   // neighbor rx
    NVIC.irqPrioritize(IVT.TIM5, 0x50);

    NVIC.irqEnable(IVT.EXTI0);                   // neighbor in2
    NVIC.irqPrioritize(IVT.EXTI0, 0x64);

    NVIC.irqEnable(IVT.EXTI1);                   // neighbor in3
    NVIC.irqPrioritize(IVT.EXTI1, 0x64);

    GPIOPin dacOut = BATTERY_SIM_GPIO;
    dacOut.setControl(GPIOPin::IN_ANALOG);

    Dac::instance.init();
    Dac::instance.configureChannel(BATTERY_SIM_DAC_CH);
    Dac::instance.enableChannel(BATTERY_SIM_DAC_CH);
    Dac::instance.write(BATTERY_SIM_DAC_CH, 0); // default to off

    GPIOPin v3CurrentPin = V3_CURRENT_GPIO;
    v3CurrentPin.setControl(GPIOPin::IN_ANALOG);

    GPIOPin usbCurrentPin = USB_CURRENT_GPIO;
    usbCurrentPin.setControl(GPIOPin::IN_ANALOG);

    adc.init();
    adc.setSampleRate(USB_CURRENT_ADC_CH, Adc::SampleRate_55_5);
    adc.setSampleRate(V3_CURRENT_ADC_CH, Adc::SampleRate_55_5);

    testUsbEnable.setControl(GPIOPin::OUT_2MHZ);
    testUsbEnable.setHigh();    // default to enabled

    i2c.init(JIG_SCL_GPIO, JIG_SDA_GPIO);
    neighbor.init();
}

/*
 * Test data has arrived from the host via USB.
 * Dispatch the command to the appropriate handler.
 */
void TestJig::onTestDataReceived(uint8_t *buf, unsigned len)
{
    uint8_t testId = buf[0];
    if (testId < arraysize(handlers)) {
        TestHandler handler = handlers[testId];
        handler(len, buf);
    }
}

// this additional glue is required to service IRQs from the shared EXTI vector.
void TestJig::neighborInIsr(uint8_t side)
{
    neighbor.onRxPulse(side);
}

/*
 * Called from ISR context within Neighbors once we've received a message.
 * Forward it on over USB.
 */
void TestJig::onNeighborMsgRx(uint8_t side, uint16_t msg)
{
    const uint8_t response[] = { 6, side, msg & 0xff, (msg >> 8) & 0xff};
    UsbDevice::write(response, sizeof response);
}

/*******************************************
 * T E S T  H A N D L E R S
 ******************************************/

/*
 * args[1] == non-zero for enable, 0 for disable
 */
void TestJig::setUsbEnabledHandler(uint8_t argc, uint8_t *args)
{
    bool enable = args[1];
    if (enable) {
        testUsbEnable.setHigh();
    } else {
        testUsbEnable.setLow();
    }

    // no response data - just indicate that we're done
    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * args[1] == value, LSB
 * args[2] == value, MSB
 */
void TestJig::setSimulatedBatteryVoltageHandler(uint8_t argc, uint8_t *args)
{
    uint16_t val = (args[1] | args[2] << 8);
    Dac::instance.write(BATTERY_SIM_DAC_CH, val);

    // no response data - just indicate that we're done
    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 *  no args
 */
void TestJig::getBatterySupplyCurrentHandler(uint8_t argc, uint8_t *args)
{
    uint16_t sample = adc.sample(V3_CURRENT_ADC_CH);

    const uint8_t response[] = { args[0], sample & 0xff, sample >> 8 };
    UsbDevice::write(response, sizeof response);
}

/*
 *  no args
 */
void TestJig::getUsbCurrentHandler(uint8_t argc, uint8_t *args)
{
    uint16_t sample = adc.sample(USB_CURRENT_ADC_CH);

    const uint8_t response[] = { args[0], sample & 0xff, sample >> 8 };
    UsbDevice::write(response, sizeof response);
}

void TestJig::beginNeighborRxHandler(uint8_t argc, uint8_t *args)
{
    neighbor.beginReceiving();

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

void TestJig::stopNeighborRxHandler(uint8_t argc, uint8_t *args)
{
    neighbor.stopReceiving();

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*******************************************
 * I N T E R R U P T  H A N D L E R S
 ******************************************/

IRQ_HANDLER ISR_TIM3()
{
    if (TIM3.SR & 1)    // update event
        neighbor.transmitNextBit();
    TIM3.SR = 0; // must clear status to acknowledge the ISR

}

IRQ_HANDLER ISR_TIM5()
{
    if (TIM5.SR & 1)
        neighbor.rxPeriodIsr();
    TIM5.SR = 0; // must clear status to acknowledge the ISR
}

/*
 * Neighbor Ins 2 and 3 are on EXTI lines 0 and 1 respectively.
 */
IRQ_HANDLER ISR_EXTI0()
{
    neighbor.onRxPulse(2);
}

IRQ_HANDLER ISR_EXTI1()
{
    neighbor.onRxPulse(3);
}

IRQ_HANDLER ISR_I2C1_EV()
{
    i2c.isr_EV();
}

IRQ_HANDLER ISR_I2C1_ER()
{
    i2c.isr_ER();
}
