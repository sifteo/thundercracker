/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _TEST_JIG_H
#define _TEST_JIG_H

#include <stdint.h>

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
        StmExternalFlashComms                   = 0,
        StmExternalFlashReadWrite               = 1,
        NrfComms                                = 2,
        SetFixtureVoltage                       = 3,
        ReadFixtureVoltage                      = 4,
        ReadFixtureCurrent                      = 5,
        ReadStmVsysVoltage                      = 6,
        ReadStmBattVoltage                      = 7,
        StoreStmBattVoltage                     = 8,
        EnableTestJigNeighborTransmit           = 9,
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
        Set_STM_neighbor_ID,
        Set_fixture_neighbor_ID
    };

    static void enableUsbPower();
    static void disableUsbPower();

    typedef void(*TestHandler)(uint8_t argc, uint8_t *args);
    static const TestHandler handlers[];

    static void stmExternalFlashCommsHandler(uint8_t argc, uint8_t *args);
    static void stmExternalFlashReadWriteHandler(uint8_t argc, uint8_t *args);
    static void nrfCommsHandler(uint8_t argc, uint8_t *args);
    static void setFixtureVoltageHandler(uint8_t argc, uint8_t *args);
    static void getFixtureVoltageHandler(uint8_t argc, uint8_t *args);
    static void getFixtureCurrentHandler(uint8_t argc, uint8_t *args);
    static void getStmVsysVoltageHandler(uint8_t argc, uint8_t *args);
    static void getStmBattVoltageHandler(uint8_t argc, uint8_t *args);
    static void storeStmBattVoltageHandler(uint8_t argc, uint8_t *args);
    static void enableTestJigNeighborTx(uint8_t argc, uint8_t *args);
};

#endif // _TEST_JIG_H
