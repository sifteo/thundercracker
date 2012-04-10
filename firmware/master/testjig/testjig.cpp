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

static I2CSlave i2c(&I2C1);
static Neighbor neighbor(JIG_NBR_IN1_GPIO,
                         JIG_NBR_IN2_GPIO,
                         JIG_NBR_IN3_GPIO,
                         JIG_NBR_IN4_GPIO,
                         JIG_NBR_OUT1_GPIO,
                         JIG_NBR_OUT2_GPIO,
                         JIG_NBR_OUT3_GPIO,
                         JIG_NBR_OUT4_GPIO,
                         HwTimer(&TIM3),
                         HwTimer(&TIM5));

/*
 * Table of test handlers.
 * Order must match the Command enum.
 */
TestJig::TestHandler const TestJig::handlers[] = {
    stmExternalFlashCommsHandler,
    stmExternalFlashReadWriteHandler,
    nrfCommsHandler,
    setFixtureVoltageHandler,
    getFixtureVoltageHandler
};

void TestJig::init()
{
    i2c.init(JIG_SCL_GPIO, JIG_SDA_GPIO);
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
        handler(buf, len);
    }
}

void TestJig::enable_neighbor_receive()
{
    neighbor.beginReceiving();
}

uint16_t TestJig::get_received_data()
{
    return neighbor.getLastRxData();
}

void TestJig::disableUsbPower()
{
    GPIOPin usbpwr = USB_PWR_GPIO;
    usbpwr.setControl(GPIOPin::OUT_2MHZ);
    usbpwr.setLow();
}

void TestJig::enableUsbPower()
{
    GPIOPin usbpwr = USB_PWR_GPIO;
    usbpwr.setControl(GPIOPin::OUT_2MHZ);
    usbpwr.setHigh();
}

/*******************************************
 * T E S T  H A N D L E R S
 ******************************************/

void TestJig::stmExternalFlashCommsHandler(uint8_t *args, uint8_t len)
{

}

void TestJig::stmExternalFlashReadWriteHandler(uint8_t *args, uint8_t len)
{

}

void TestJig::nrfCommsHandler(uint8_t *args, uint8_t len)
{

}

void TestJig::setFixtureVoltageHandler(uint8_t *args, uint8_t len)
{

}

void TestJig::getFixtureVoltageHandler(uint8_t *args, uint8_t len)
{

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

IRQ_HANDLER ISR_EXTI0()
{
    neighbor.onRxPulse(0);
}

IRQ_HANDLER ISR_EXTI1()
{
    neighbor.onRxPulse(1);
}

IRQ_HANDLER ISR_EXTI2()
{
    neighbor.onRxPulse(2);
}

IRQ_HANDLER ISR_EXTI3()
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
