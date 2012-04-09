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

TestJig::TestHandler const TestJig::handlers[] = {
    stmExternalFlashCommsHandler
};

void TestJig::init()
{
    i2c.init(MASTER_SCL_GPIO, MASTER_SDA_GPIO);
}

void TestJig::enable_neighbor_receive()
{
    neighbor.beginReceiving();
}

void TestJig::parseCommand()
{
    uint16_t commands[MAX_NUMBER_OF_COMMANDS];

    memset(commands, 0, sizeof commands);

    uint8_t command_counter = 0;
    uint8_t char_counter = 0;
    while (char_counter < command_buffer_pointer) {

        uint8_t cmd = command_buffer[char_counter];
        if (cmd > 47 && cmd < 58) {
            commands[command_counter] = (commands[command_counter] * 10) + command_buffer[char_counter] - 48;
        }
        // is it a comma?
        if (command_buffer[char_counter] == 44) {
            if (command_counter<MAX_NUMBER_OF_COMMANDS)
                command_counter++;
            else
                break;
        }
        char_counter++;
    }

    uint8_t testId = commands[0];
    if (testId < arraysize(handlers)) {
        TestHandler hndlr = handlers[commands[0]];
        hndlr(command_buffer);
    }
}

void TestJig::new_command(uint8_t data)
{
    if (data==13) { //hit enter
        parseCommand();
        command_buffer_pointer=0;
    }
    else if (data == 8) { //backspace
        if (command_buffer_pointer > 0)
            command_buffer_pointer--;
    }
    else if (command_buffer_pointer < SIZE_OF_COMMAND_BUFFER) {
        command_buffer[command_buffer_pointer] = data;
        command_buffer_pointer++;
    }
}

void TestJig::send(uint8_t addr,uint8_t data)
{

}

uint16_t TestJig::get_received_data()
{
    return neighbor.getLastRxData();
}

void TestJig::start_test(uint8_t test_num)
{

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

void TestJig::send_test_result() {
    uint8_t str_len;
    static uint8_t buffer[30];
    uint8_t *b=buffer;
    uint8_t result=55;
    str_len = sprintf((char *)b, "Result=%03d\n\r", result);
    UsbDevice::write(b, str_len);
}

/*******************************************
 * T E S T  H A N D L E R S
 ******************************************/

void TestJig::stmExternalFlashCommsHandler(uint8_t *args)
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
