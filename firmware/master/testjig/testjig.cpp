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

TestJig::VramTransaction TestJig::vramTransaction;
TestJig::SensorsTransaction TestJig::sensorsTransaction;

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
    writeToCubeVramHandler,                 // 6
    setCubeSensorsEnabledHandler,           // 7
};

void TestJig::init()
{
    NVIC.irqEnable(IVT.TIM3);                   // neighbor tx
    NVIC.irqPrioritize(IVT.TIM3, 0x60);

    NVIC.irqEnable(IVT.TIM5);                   // neighbor rx
    NVIC.irqPrioritize(IVT.TIM5, 0x50);

    NVIC.irqEnable(IVT.EXTI0);                  // neighbor in2
    NVIC.irqPrioritize(IVT.EXTI0, 0x64);

    NVIC.irqEnable(IVT.EXTI1);                  // neighbor in3
    NVIC.irqPrioritize(IVT.EXTI1, 0x64);

    /*
     * Errata indicates that if i2c interrupts aren't handled quickly enough,
     * data corruption can occur - use highest possible ISR priorities.
     */
    NVIC.irqEnable(IVT.I2C1_EV);                // highest
    NVIC.irqPrioritize(IVT.I2C1_EV, 0x0);

    NVIC.irqEnable(IVT.I2C1_ER);
    NVIC.irqPrioritize(IVT.I2C1_ER, 0x1);       // highest - 1

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

    sensorsTransaction.enabled = false;
    sensorsTransaction.byteIdx = 0;
    vramTransaction.state = VramIdle;

    i2c.init(JIG_SCL_GPIO, JIG_SDA_GPIO, I2C_SLAVE_ADDRESS);
//    neighbor.init();
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
    // NB! neighbor transmitted in its native big endian format
    const uint8_t response[] = { 6, side, (msg >> 8) & 0xff, msg & 0xff };
    UsbDevice::write(response, sizeof response);
}

/*
 * Called from ISR context when an i2c event has occurred.
 * If the host has asked us to report on i2c related
 */
void TestJig::onI2cEvent()
{
    uint8_t byte;
    uint16_t status = i2c.irqEvStatus();

    /*
     * Dataflow is as follows:
     *  - cube TXes ACK packet to us
     *  - repeated start condition follows, and we can transmit to the cube.
     *
     * On the repeated start condition, send the ACK packet that we
     * received during the former transmission.
     */
    if (status & I2CSlave::AddressMatch) {
        if (sensorsTransaction.enabled && sensorsTransaction.byteIdx > 0) {
            uint8_t resp[1 + sizeof sensorsTransaction.cubeAck] = { 6 };
            memcpy(resp + 1, &sensorsTransaction.cubeAck, sizeof sensorsTransaction.cubeAck);
            UsbDevice::write(resp, sizeof resp);
        }
        sensorsTransaction.byteIdx = 0;
    }

    /*
     * The nRFLE1 and STM32 have somewhat complementary i2c hardware oddities.
     * When the LE1 in master mode is done receiving, it will NACK the last byte
     * before setting the stop bit. The STM32 does not emit a STOP event in the
     * case that a NACK was received.
     *
     * It's a bit gross, but treat a NACK as equivalent to a STOP to work around this.
     */
    if (status & (I2CSlave::Nack | I2CSlave::StopBit)) {
        vramTransaction.state = VramIdle;
    }

    // send next byte
    if (status & I2CSlave::TxEmpty) {
        switch (vramTransaction.state) {

        case VramIdle:
            byte = 0xff;
            break;

        case VramAddressHigh:
            byte = vramTransaction.address >> 8;
            vramTransaction.state = VramAddressLow;
            break;

        case VramAddressLow:
            byte = vramTransaction.address & 0xff;
            vramTransaction.state = VramPayload;
            break;

        case VramPayload:
            byte = vramTransaction.payload;
            vramTransaction.state = VramIdle;
            break;

        }
    }

    i2c.isrEV(status, &byte);

    // we received a byte
    if (status & I2CSlave::RxNotEmpty) {
        if (sensorsTransaction.byteIdx < sizeof sensorsTransaction.cubeAck) {
            uint8_t *pAck = reinterpret_cast<uint8_t*>(&sensorsTransaction.cubeAck);
            pAck[sensorsTransaction.byteIdx++] = byte;
        }
    }
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

/*
 * args[1] - low byte of address
 * args[2] - high byte of address
 * args[3] - data payload
 */
void TestJig::writeToCubeVramHandler(uint8_t argc, uint8_t *args)
{
    // ensure any vram transactions in progress are complete
    while (vramTransaction.state != VramIdle)
        ;

    vramTransaction.address = args[2] << 8 | args[1];
    vramTransaction.payload = args[3];
    vramTransaction.state = VramAddressHigh;

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * args[1] - 0 for disabled, non-zero for enabled
 */
void TestJig::setCubeSensorsEnabledHandler(uint8_t argc, uint8_t *args)
{
    sensorsTransaction.enabled = args[1];
    sensorsTransaction.byteIdx = 0;

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
    TestJig::onI2cEvent();
}

IRQ_HANDLER ISR_I2C1_ER()
{
    i2c.isrER();
}
