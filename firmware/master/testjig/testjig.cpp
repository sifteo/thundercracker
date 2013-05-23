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
#include "pulse_rx.h"
#include "gpio.h"
#include "macros.h"
#include "dac.h"
#include "adc.h"
#include "bootloader.h"

extern unsigned     __data_start;

static I2CSlave i2c(&I2C1);

// control for the pass-through USB of the master under test
static GPIOPin testUsbEnable = USB_PWR_GPIO;

static GPIOPin usbCurrentSign = USB_CURRENT_DIR_GPIO;
static GPIOPin v3CurrentSign = V3_CURRENT_DIR_GPIO;
static GPIOPin dip1 = DIP_SWITCH1_GPIO;
static GPIOPin dip2 = DIP_SWITCH2_GPIO;
static GPIOPin dip3 = DIP_SWITCH3_GPIO;
static GPIOPin dip4 = DIP_SWITCH4_GPIO;

static PulseRX PulseRX2v0Rail(NBR_IN4_GPIO);
static PulseRX PulseRX3v3Rail(NBR_IN3_GPIO);

BitVector<TestJig::NUM_WORK_ITEMS> TestJig::taskWork;

TestJig::I2CWriteTransaction TestJig::cubeWrite;
TestJig::AckPacket TestJig::ackPacket;
TestJig::I2CUsbPayload TestJig::i2cUsbPayload;
TestJig::NeighborRxData TestJig::neighborRxData;

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
    beginNoiseCheckHandler,                 // 10
    stopNoiseCheckHandler,                  // 11
    getFirmwareVersion,                     // 12
    bootloadRequestHandler,                 // 13
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

    dip1.setControl(GPIOPin::IN_PULL);          // dip1 is used to make the default 2.8V for master stations
    dip2.setControl(GPIOPin::IN_PULL);          // dip2 is used to make the default voltage 2.0V for pairing station
    dip3.setControl(GPIOPin::IN_PULL);          // dip3 is used to disable the USB peripheral and to be used as power only
    dip4.setControl(GPIOPin::IN_PULL);          // dip4 is used for the bootloader. we shouldn't use this for anything else

    dip1.pullup();
    dip2.pullup();
    dip3.pullup();
    dip4.pullup();

    // Pullups need some time to charge the line up
    SysTime::Ticks pullupsCharged = SysTime::ticks() + SysTime::usTicks(10);
    while (SysTime::ticks() < pullupsCharged) {
        ;
    }

    if(dip1.isLow()) {
        Dac::write(BATTERY_SIM_DAC_CH, DAC_2V8);
    } else if(dip2.isLow()){
        Dac::write(BATTERY_SIM_DAC_CH, DAC_2V0); // used as a happy medium for both cube and master for pairing station
    } else {
        Dac::write(BATTERY_SIM_DAC_CH, DAC_1V2); // default to 1v2
    }

    GPIOPin v3CurrentPin = V3_CURRENT_GPIO;
    v3CurrentPin.setControl(GPIOPin::IN_ANALOG);

    GPIOPin usbCurrentPin = USB_CURRENT_GPIO;
    usbCurrentPin.setControl(GPIOPin::IN_ANALOG);

    PWR_MEASURE_ADC.init();
    PWR_MEASURE_ADC.setSampleRate(USB_CURRENT_ADC_CH, Adc::SampleRate_55_5);
    PWR_MEASURE_ADC.setSampleRate(V3_CURRENT_ADC_CH, Adc::SampleRate_55_5);

    testUsbEnable.setControl(GPIOPin::OUT_2MHZ);
    testUsbEnable.setHigh();    // default to enabled

    ackPacket.enabled = false;
    ackPacket.len = 0;
    taskWork.clear();
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
void TestJig::onNeighborRX(int8_t side, uint16_t msg)
{
    // NB! neighbor transmitted in its native big endian format

    if (taskWork.test(NeighborRXWrite))
            return;

    neighborRxData.side = side;
    neighborRxData.msg = msg;

    taskWork.atomicMark(NeighborRXWrite);
    Tasks::trigger(Tasks::TestJig);
}

ALWAYS_INLINE static bool flagsMatch(uint32_t value, uint32_t flags) {
    return (value & flags) == flags;
}

/*
 * Called from ISR context when an i2c event has occurred.
 * If the host has asked us to report on i2c related
 */
void TestJig::onI2cEvent()
{
    uint32_t status = i2c.status();

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

    if (flagsMatch(status, I2CSlave::RxAddressMatched)) {
        // begin new rx sequence
        ackPacket.len = 0;

    }

    if (flagsMatch(status, I2CSlave::TxAddressMatched)) {

        /*
         * begin new TX sequence if we have data ready to go, otherwise
         * send 0xff to just keep the cube alive.
         */

        if (cubeWrite.remaining > 0) {
            i2c.write(*cubeWrite.ptr);
        } else {
            i2c.write(0xff);
        }
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

        if (status & I2CSlave::StopBit) {
            i2c.onStopBit();
        }
    }

    if (flagsMatch(status, I2CSlave::ByteTransmitted)) {

        /*
         * our previous byte was written - update our count and continue
         * sending if we're not done.
         *
         * Otherwise, write 0xff to keep the cube alive.
         */

        if (cubeWrite.remaining > 0) {

            cubeWrite.ptr++;
            cubeWrite.remaining--;
        }

        // more to write?
        if (cubeWrite.remaining > 0) {
            i2c.write(*cubeWrite.ptr);
        } else {
            i2c.write(0xff);
        }
    }

    if (flagsMatch(status, I2CSlave::ByteReceived)) {

        /*
         * We received a byte. Capture it if we won't overwrite any
         * pending USB writes, and it won't overflow our buffer.
         */

        uint8_t byte = i2c.read();

        if (!ackPacket.full()) {
            ackPacket.append(byte);
        }

        if (ackPacket.full() && ackPacket.enabled) {
            if (!taskWork.test(I2CWrite)) {

                /*
                 * If our double buffer is available, stash it there until
                 * our task can write it out over USB, and we're free to start
                 * collecting our next packet.
                 */

                memcpy(&i2cUsbPayload.bytes[1], &ackPacket.payload, sizeof ackPacket.payload);
                ackPacket.len = 0;
                taskWork.atomicMark(I2CWrite);
                Tasks::trigger(Tasks::TestJig);
            }
        }
    }
}

void TestJig::onI2cError()
{
    i2c.isrER();
}

/*
 * An opportunity to perform any potentially longer running operations.
 * The only requirement for this at the moment is to write i2c data over USB
 * since that's a blocking operation.
 */
void TestJig::task()
{
    BitVector<NUM_WORK_ITEMS> pending = taskWork;
    unsigned index;

    while (pending.clearFirst(index)) {
        taskWork.atomicClear(index);

        switch (index) {
            case I2CWrite:
                i2cUsbPayload.bytes[0] = EventAckPacket;
                UsbDevice::write(i2cUsbPayload.bytes, sizeof i2cUsbPayload.bytes);
                break;

            case NeighborRXWrite: {

                const uint8_t response[] = {
                    EventNeighbor,
                    neighborRxData.side,
                    (neighborRxData.msg >> 8) & 0xff,
                    neighborRxData.msg & 0xff
                };

                UsbDevice::write(response, sizeof response);
                break;
            }
        }
    }


}

/*******************************************
 * T E S T  H A N D L E R S
 ******************************************/

/*
 *  no args
 */
 void TestJig::bootloadRequestHandler(uint8_t argc, uint8_t *args)
 {
 #ifdef BOOTLOADABLE
     __data_start = Bootloader::UPDATE_REQUEST_KEY;
     NVIC.deinit();
     NVIC.systemReset();
 #endif
 }

/*
 *  no args
 */
void TestJig::getFirmwareVersion(uint8_t argc, uint8_t *args)
{
    const uint8_t MAX_SIZE = 32;
    const uint8_t sz = MIN( MAX_SIZE, strlen(TOSTRING(SDK_VERSION)));
    uint8_t response[MAX_SIZE] = { args[0] };
    memcpy(&response[1], TOSTRING(SDK_VERSION), sz);

    UsbDevice::write(response, sz+1);
}

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
        sampleSum += PWR_MEASURE_ADC.sampleSync(V3_CURRENT_ADC_CH);
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
        sampleSum += PWR_MEASURE_ADC.sampleSync(USB_CURRENT_ADC_CH);
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

    SysTime::Ticks timeout = SysTime::ticks() + SysTime::msTicks(250);
    while (cubeWrite.remaining > 0) {
        if (SysTime::ticks() > timeout) {

            // Lets send an error to FAT if there is still data
            // in the output buffer for I2C. Most likely b/c a cube
            // is not connected!

            cubeWrite.reset();
            const uint8_t response[] = { I2CTimeout };
            UsbDevice::write(response, sizeof response);
            return;
        }
    }

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

/*
 * No args.
 */
void TestJig::beginNoiseCheckHandler(uint8_t argc, uint8_t *args)
{
    /*
     * NeighborRX and PulseRX share input pins.
     * Since the neighbor protocol disables irq
     * of the masked side, we stop NeighborRX
     * before starting pulseRX, just in case.
     */
    NeighborRX::stop();

    // setup and start noise test on both rails simultaneously
    PulseRX2v0Rail.init();
    PulseRX3v3Rail.init();
    PulseRX2v0Rail.start();
    PulseRX3v3Rail.start();

    const uint8_t response[] = { args[0] };
    UsbDevice::write(response, sizeof response);
}

/*
 * No args.
 */
void TestJig::stopNoiseCheckHandler(uint8_t argc, uint8_t *args)
{
    uint8_t response[] = { args[0], \
            (PulseRX2v0Rail.count() >> 8) & 0xff, PulseRX2v0Rail.count() & 0xff, \
            (PulseRX3v3Rail.count() >> 8) & 0xff, PulseRX3v3Rail.count() & 0xff \
    };
    PulseRX2v0Rail.stop();
    PulseRX3v3Rail.stop();

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
    PulseRX3v3Rail.pulseISR();
}

IRQ_HANDLER ISR_EXTI1()
{
    NeighborRX::pulseISR(3);
    PulseRX2v0Rail.pulseISR();
}

IRQ_HANDLER ISR_I2C1_EV()
{
    TestJig::onI2cEvent();
}

IRQ_HANDLER ISR_I2C1_ER()
{
    TestJig::onI2cError();
}
