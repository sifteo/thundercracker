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

    enum TestJigEventId {
        EventAckPacket      = 100,
        EventNeighbor       = 101
    };

    static void init();
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
    static void writeToCubeI2CHandler(uint8_t argc, uint8_t *args);
    static void setCubeSensorsEnabledHandler(uint8_t argc, uint8_t *args);

    struct SensorsTransaction {
        bool enabled;
        RF_ACKType cubeAck;
        uint8_t byteIdx;
    };

    enum I2CProtocolType {
        I2CVramMax          = 0x44,
        I2CFlashFifo        = 0xfd,
        I2CFlashReset       = 0xfe,
        I2CDone             = 0xff,
    };

    // data that gets written to a cube
    struct I2CWriteTransaction {
        // volatile to ensure it gets re-loaded while we're waiting for it to
        // get updated from within the i2c irq
        volatile uint8_t remaining;
        uint8_t *data;
    };

    static SensorsTransaction sensorsTransaction;
    static I2CWriteTransaction cubeWrite;
};

#endif // _TEST_JIG_H
