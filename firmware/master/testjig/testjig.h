/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _TEST_JIG_H
#define _TEST_JIG_H

#include <stdint.h>

#define MAX_NUMBER_OF_COMMANDS 2
#define SIZE_OF_COMMAND_BUFFER 10

class TestJig
{
public:
    static void init();
    static void send(uint8_t addr, uint8_t data);
    static void send_test_result();
    static void new_command(uint8_t data);
    static void enable_neighbor_receive();
    static uint8_t got_i2c;
    static uint8_t got_result;
    static uint16_t get_received_data();

    static void enableUsbPower();
    static void disableUsbPower();

private:

    enum Command {
        StmExternalFlashComms,
        StmExternalFlashReadWrite,
        NrfComms,
        SetFixtureVoltage,
        ReadFixtureVoltage,
        ReadStmVsysVoltage,
        ReadFixtureCurrent,
        Read_STM_batt_voltage,
        Store_STM_batt_voltage,
        Set_Speaker_on,
        Set_Speaker_off,
        Set_Fixture_USB_pwr_on,
        Set_USB_Fixture_pwr_off,
        read_STM_USB_voltage,
        Set_HomeButton_LED_test,
        Read_volume_slider_voltage,
        Write_volume_slider_voltage,
        Set_Master_state_standby,
        Set_Master_start_active,
        Enable_STM_neighbor_transmit,
        Enable_Fixture_neighbor_receive,
        Enable_Fixture_neighbor_transmit,
        Set_STM_neighbor_ID,
        Set_fixture_neighbor_ID
    };

    static uint8_t current_test;
    static void parseCommand();
    static uint8_t command_buffer_pointer;
    static uint8_t command_buffer[SIZE_OF_COMMAND_BUFFER];

    void start_test(uint8_t test_num);

    typedef void(*TestHandler)(uint8_t *args);
    static const TestHandler handlers[];

    static void stmExternalFlashCommsHandler(uint8_t *args);
};

#endif // _TEST_JIG_H
