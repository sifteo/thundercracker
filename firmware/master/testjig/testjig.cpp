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
        if (command_buffer[char_counter]==44) {
            if (command_counter<MAX_NUMBER_OF_COMMANDS)
                command_counter++;
            else
                break;
        }
        char_counter++;
    }

    switch (commands[0]) {
    case Read_STM_register:
        i2c.setNextTest(commands[0]);  //load
        got_result=1;
        break;
    case Write_STM_register:
        break;
    case Read_STM_Int_flash:
        break;
    case Write_STM_int_flash:
        break;
    case Read_STM_ext_flash:
        break;
    case Write_STM_ext_flash:
        break;
    case Read_L01_register:
        break;
    case Write_L01_register:
        break;
    case Set_fixture_PS_voltage:
        break;
    case Read_fixture_PS_voltage:
        break;
    case Read_STM_VSYS_voltage:
        break;
    case Read_fixture_current:
        break;
    case Read_STM_batt_voltage:
        break;
    case Store_STM_batt_voltage:
        break;
    case Set_Speaker_on:
        break;
    case Set_Speaker_off:
        break;
    case Set_Fixture_USB_pwr_on:
        break;
    case Set_USB_Fixture_pwr_off:
        break;
    case read_STM_USB_voltage:
        break;
    case Set_HomeButton_LED_test:
        break;
    case Read_volume_slider_voltage:
        break;
    case Write_volume_slider_voltage:
        break;
    case Set_Master_state_standby:
        break;
    case Set_Master_start_active:
        break;
    case Enable_STM_neighbor_transmit:
        break;
    case Enable_Fixture_neighbor_receive:
        break;
    case Enable_Fixture_neighbor_transmit:
        break;
    case Set_STM_neighbor_ID:
        break;
    case Set_fixture_neighbor_ID:
        break;
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
    usbpwr.setControl(GPIOPin::OUT_10MHZ);
    usbpwr.setLow();
}

void TestJig::enableUsbPower()
{
    GPIOPin usbpwr = USB_PWR_GPIO;
    usbpwr.setControl(GPIOPin::OUT_10MHZ);
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
