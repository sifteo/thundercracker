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
#include "neighbor_tx.h"
#include "neighbor_rx.h"
#include "gpio.h"
#include "macros.h"
#include "dac.h"
#include "adc.h"

static I2CSlave i2c(&I2C1);

// control for the pass-through USB of the master under test
static GPIOPin testUsbEnable = USB_PWR_GPIO;

static Adc adc(&PWR_MEASURE_ADC);
static GPIOPin usbCurrentSign = USB_CURRENT_DIR_GPIO;
static GPIOPin v3CurrentSign = V3_CURRENT_DIR_GPIO;

TestJig::I2CWriteTransaction TestJig::cubeWrite;
TestJig::AckPacket TestJig::ackPacket;
TestJig::I2CUsbPayload TestJig::i2cUsbPayload;

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
    writeToCubeI2CHandler,                  // 6
    setCubeSensorsEnabledHandler,           // 7
    beginNeighborTxHandler,                 // 8
    stopNeighborTxHandler,                  // 9
};

void TestJig::init()
{
    NVIC.irqEnable(IVT.NBR_RX_TIM);
    NVIC.irqPrioritize(IVT.NBR_RX_TIM, 0x50);

    NVIC.irqEnable(IVT.NBR_TX_TIM);
    NVIC.irqPrioritize(IVT.NBR_TX_TIM, 0x60);

    // neighbor 0 and 1 are on ISR_EXTI9_5 - see exti.cpp

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

    Dac::init();
    Dac::configureChannel(BATTERY_SIM_DAC_CH);
    Dac::enableChannel(BATTERY_SIM_DAC_CH);
    Dac::write(BATTERY_SIM_DAC_CH, 0); // default to off

    GPIOPin v3CurrentPin = V3_CURRENT_GPIO;
    v3CurrentPin.setControl(GPIOPin::IN_ANALOG);

    GPIOPin usbCurrentPin = USB_CURRENT_GPIO;
    usbCurrentPin.setControl(GPIOPin::IN_ANALOG);

    adc.init();
    adc.setSampleRate(USB_CURRENT_ADC_CH, Adc::SampleRate_55_5);
    adc.setSampleRate(V3_CURRENT_ADC_CH, Adc::SampleRate_55_5);

    testUsbEnable.setControl(GPIOPin::OUT_2MHZ);
    testUsbEnable.setHigh();    // default to enabled

    ackPacket.enabled = false;
    ackPacket.len = 0;
    i2cUsbPayload.usbWritePending = false;
    cubeWrite.remaining = 0;

    i2c.init(JIG_SCL_GPIO, JIG_SDA_GPIO, I2C_SLAVE_ADDRESS);

    NeighborRX::init();
    NeighborTX::init();
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

/*
 * Called from ISR context within Neighbors once we've received a message.
 * Forward it on over USB.
 */
void NeighborRX::callback(unsigned side, unsigned msg)
{
    // NB! neighbor transmitted in its native big endian format
    const uint8_t response[] = { TestJig::EventNeighbor, side, (msg >> 8) & 0xff, msg & 0xff };
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
     *
     * NOTE: because we need to potentially block when writing this data over USB,
     *       we trigger our task to do it for us, but this means that any
     *       additional i2c data that arrives in the meantime will be dropped.
     *       This is generally fine, since we're not tracking any specific packet.
     */

    if (status & I2CSlave::AddressMatch) {
        // begin new rx sequence
        ackPacket.len = 0;
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
        cubeWrite.remaining = 0;
    }

    // send next byte
    if (status & I2CSlave::TxEmpty) {
        if (cubeWrite.remaining > 0) {
            byte = *cubeWrite.ptr++;
            cubeWrite.remaining--;
        } else {
            byte = 0xff;
        }
    }

    i2c.isrEV(status, &byte);

    /*
     * We received a byte. Capture it if we won't overwrite any
     * pending USB writes, and it won't overflow our buffer.
     */
    if (status & I2CSlave::RxNotEmpty) {

        if (!ackPacket.full())
            ackPacket.append(byte);

        if (ackPacket.full() && ackPacket.enabled) {
            if (!i2cUsbPayload.usbWritePending) {

                /*
                 * If our double buffer is available, stash it there until
                 * our task can write it out over USB, and we're free to start
                 * collecting our next packet.
                 */

                memcpy(&i2cUsbPayload.bytes[1], &ackPacket.payload, sizeof ackPacket.payload);
                ackPacket.len = 0;
                i2cUsbPayload.usbWritePending = true;
                Tasks::trigger(Tasks::TestJig);
            }
        }
    }
}

/*
 * An opportunity to perform any potentially longer running operations.
 * The only requirement for this at the moment is to write i2c data over USB
 * since that's a blocking operation.
 */
void TestJig::task()
{
    if (!i2cUsbPayload.usbWritePending)
        return;

    i2cUsbPayload.bytes[0] = EventAckPacket;
    UsbDevice::write(i2cUsbPayload.bytes, sizeof i2cUsbPayload.bytes);
    i2cUsbPayload.usbWritePending = false;
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
    
    //Have to divide the targeted voltage by two. 
    //Gets multiplied by the gain stage of the external opamp (gain == 2)
    val /= 2;
    Dac::write(BATTERY_SIM_DAC_CH, val);

    // no response data - just indicate that we're done
    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 *  no args
 */
void TestJig::getBatterySupplyCurrentHandler(uint8_t argc, uint8_t *args)
{
    uint32_t sampleSum = 0;

    for (unsigned i = 0; i < NUM_CURRENT_SAMPLES; i++) {
        sampleSum += adc.sample(V3_CURRENT_ADC_CH);
    }
    
    uint16_t sampleAvg = sampleSum / NUM_CURRENT_SAMPLES;

    const uint8_t response[] = { args[0], sampleAvg & 0xff, sampleAvg >> 8 };
    UsbDevice::write(response, sizeof response);
}

/*
 *  no args
 */
void TestJig::getUsbCurrentHandler(uint8_t argc, uint8_t *args)
{
    uint32_t sampleSum = 0;

    for (unsigned i = 0; i < NUM_CURRENT_SAMPLES; i++) {
        sampleSum += adc.sample(USB_CURRENT_ADC_CH);
    }

    uint16_t sampleAvg = sampleSum / NUM_CURRENT_SAMPLES;

    const uint8_t response[] = { args[0], sampleAvg & 0xff, sampleAvg >> 8 };
    UsbDevice::write(response, sizeof response);
}

void TestJig::beginNeighborRxHandler(uint8_t argc, uint8_t *args)
{
    if (argc < 2)
        return;

    uint8_t mask = args[1];
    NeighborRX::start(mask);

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

void TestJig::stopNeighborRxHandler(uint8_t argc, uint8_t *args)
{
    NeighborRX::stop();

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * starting with args[1], back to back payloads as specified in
 * docs/cube-factorytest-protocol.rst.
 */
void TestJig::writeToCubeI2CHandler(uint8_t argc, uint8_t *args)
{
    uint8_t transactionsWritten = 0;

    while (cubeWrite.remaining > 0)
        ;

    // step past command
    uint8_t len = argc - 1;
    memcpy(cubeWrite.data, &args[1], len);
    cubeWrite.ptr = cubeWrite.data;
    cubeWrite.remaining = len;

    const uint8_t response[] = { args[0], transactionsWritten };
    UsbDevice::write(response, sizeof response);
}

/*
 * args[1] - 0 for disabled, non-zero for enabled
 */
void TestJig::setCubeSensorsEnabledHandler(uint8_t argc, uint8_t *args)
{
    ackPacket.enabled = args[1];

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * args[1-2] - uint16_t txdata, args[3] sideMask
 */
void TestJig::beginNeighborTxHandler(uint8_t argc, uint8_t *args)
{
    uint16_t txData = *reinterpret_cast<uint16_t*>(&args[1]);
    uint8_t sideMask = args[3];

    NeighborTX::start(txData, sideMask);

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * No args.
 */
void TestJig::stopNeighborTxHandler(uint8_t argc, uint8_t *args)
{
    NeighborTX::stop();

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*******************************************
 * I N T E R R U P T  H A N D L E R S
 ******************************************/

/*
 * Neighbor Ins 2 and 3 are on EXTI lines 0 and 1 respectively.
 */
IRQ_HANDLER ISR_EXTI0()
{
    NeighborRX::pulseISR(2);
}

IRQ_HANDLER ISR_EXTI1()
{
    NeighborRX::pulseISR(3);
}

IRQ_HANDLER ISR_I2C1_EV()
{
    TestJig::onI2cEvent();
}

IRQ_HANDLER ISR_I2C1_ER()
{
    i2c.isrER();
}
