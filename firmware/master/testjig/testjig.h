/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _TEST_JIG_H
#define _TEST_JIG_H

#include <stdint.h>
#include "protocol.h"

class TestJig
{
public:
    static void init();
    static void enable_neighbor_receive();
    static uint8_t got_i2c;
    static uint8_t got_result;
    static uint16_t get_received_data();

    static void onTestDataReceived(uint8_t *buf, unsigned len);
    static void neighborInIsr(uint8_t side);
    static void onNeighborMsgRx(uint8_t side, uint16_t msg);
    static void onI2cEvent();

private:
    static const unsigned I2C_SLAVE_ADDRESS = 0x55;

    typedef void(*TestHandler)(uint8_t argc, uint8_t *args);
    static const TestHandler handlers[];

    static void setUsbEnabledHandler(uint8_t argc, uint8_t *args);
    static void setSimulatedBatteryVoltageHandler(uint8_t argc, uint8_t *args);
    static void getBatterySupplyCurrentHandler(uint8_t argc, uint8_t *args);
    static void getUsbCurrentHandler(uint8_t argc, uint8_t *args);
    static void beginNeighborRxHandler(uint8_t argc, uint8_t *args);
    static void stopNeighborRxHandler(uint8_t argc, uint8_t *args);
    static void writeToCubeVramHandler(uint8_t argc, uint8_t *args);
    static void setCubeSensorsEnabledHandler(uint8_t argc, uint8_t *args);

    struct SensorsTransaction {
        bool enabled;
        RF_MemACKType cubeAck;
        uint8_t byteIdx;
    };

    enum VramWriteState {
        VramIdle,           // no vram write request has been issued
        VramAddressHigh,    // write the high addr byte on the next i2c event
        VramAddressLow,     // write the low addr byte on the next i2c event
        VramPayload         // write the payload on the next i2c event
    };

    struct VramTransaction {
        VramWriteState state;
        uint16_t address;
        uint8_t payload;
    };

    static SensorsTransaction sensorsTransaction;
    static VramTransaction vramTransaction;

};

#endif // _TEST_JIG_H
