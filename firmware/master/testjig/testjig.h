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
    static void enable_neighbor_receive();
    static uint8_t got_i2c;
    static uint8_t got_result;
    static uint16_t get_received_data();

    static void onTestDataReceived(uint8_t *buf, unsigned len);

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
        SetFixtureUsbPower,
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

    static void enableUsbPower();
    static void disableUsbPower();

    typedef void(*TestHandler)(uint8_t *args, uint8_t len);
    static const TestHandler handlers[];

    static void stmExternalFlashCommsHandler(uint8_t *args, uint8_t len);
    static void stmExternalFlashReadWriteHandler(uint8_t *args, uint8_t len);
    static void nrfCommsHandler(uint8_t *args, uint8_t len);
    static void setFixtureVoltageHandler(uint8_t *args, uint8_t len);
    static void getFixtureVoltageHandler(uint8_t *args, uint8_t len);
};

#endif // _TEST_JIG_H
