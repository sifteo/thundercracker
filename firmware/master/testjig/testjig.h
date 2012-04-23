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
    typedef void(*TestHandler)(uint8_t argc, uint8_t *args);
    static const TestHandler handlers[];

    static void setUsbPowerHandler(uint8_t argc, uint8_t *args);
    static void setFixtureVoltageHandler(uint8_t argc, uint8_t *args);
    static void getFixtureVoltageHandler(uint8_t argc, uint8_t *args);
    static void getFixtureCurrentHandler(uint8_t argc, uint8_t *args);
    static void getStmVsysVoltageHandler(uint8_t argc, uint8_t *args);
    static void getStmBattVoltageHandler(uint8_t argc, uint8_t *args);
    static void storeStmBattVoltageHandler(uint8_t argc, uint8_t *args);
    static void enableTestJigNeighborTx(uint8_t argc, uint8_t *args);
};

#endif // _TEST_JIG_H
