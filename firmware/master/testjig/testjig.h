/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _TEST_JIG_H
#define _TEST_JIG_H

#include <stdint.h>
#include "protocol.h"
#include "macros.h"
#include "bits.h"

class TestJig
{
public:

    enum TestJigEventId {
        EventAckPacket      = 100,
        EventNeighbor       = 101,
        I2CTimeout          = 102
    };

    static void init();
    static void onTestDataReceived(uint8_t *buf, unsigned len);
    static void onI2cEvent();
    static void onI2cError();
    static void onNeighborRX(int8_t side, uint16_t msg);
    static void task();

    enum DAC_Out {
        DAC_0v0             = 0,
        DAC_1V2             = 1490,
        DAC_2V0             = 2482,
        DAC_2V2             = 2730,
        DAC_2V8             = 3475
    };

private:
    static const unsigned I2C_SLAVE_ADDRESS = 0x55;
    static const unsigned NUM_CURRENT_SAMPLES = 100;

    typedef void(*TestHandler)(uint8_t argc, uint8_t *args);
    static const TestHandler handlers[];

    static void setUsbEnabledHandler(uint8_t argc, uint8_t *args);
    static void setVBattEnabledHandler(uint8_t argc, uint8_t *args);
    static void setSimulatedBatteryVoltageHandler(uint8_t argc, uint8_t *args);
    static void getBatterySupplyCurrentHandler(uint8_t argc, uint8_t *args);
    static void getUsbCurrentHandler(uint8_t argc, uint8_t *args);
    static void beginNeighborRxHandler(uint8_t argc, uint8_t *args);
    static void stopNeighborRxHandler(uint8_t argc, uint8_t *args);
    static void writeToCubeI2CHandler(uint8_t argc, uint8_t *args);
    static void setCubeSensorsEnabledHandler(uint8_t argc, uint8_t *args);
    static void beginNeighborTxHandler(uint8_t argc, uint8_t *args);
    static void stopNeighborTxHandler(uint8_t argc, uint8_t *args);
    static void beginNoiseCheckHandler(uint8_t argc, uint8_t *args);
    static void stopNoiseCheckHandler(uint8_t argc, uint8_t *args);
    static void getFirmwareVersion(uint8_t argc, uint8_t *args);
    static void bootloadRequestHandler(uint8_t argc, uint8_t *args);

    struct AckPacket {
        RF_ACKType payload;
        bool enabled;
        uint8_t len;

        bool ALWAYS_INLINE full() const {
            return len == sizeof payload;
        }

        void ALWAYS_INLINE append(uint8_t b) {
            payload.bytes[len++] = b;
        }
    };

    // double buffered i2c payload to transmit via USB
    struct I2CUsbPayload {
        uint8_t bytes[sizeof(RF_ACKType) + 1];
    };

    enum I2CProtocolType {
        I2CVramMax          = 0x44,
        I2CSetNeighborID    = 0xfc,
        I2CFlashFifo        = 0xfd,
        I2CFlashReset       = 0xfe,
        I2CDone             = 0xff,
    };

    enum WorkItem {
        I2CWrite,
        NeighborRXWrite,
        NUM_WORK_ITEMS,         // Must be last
    };

    static BitVector<NUM_WORK_ITEMS> taskWork;

    struct NeighborRxData {
        uint16_t msg;
        uint8_t side;
    };

    // data that gets written to a cube
    struct I2CWriteTransaction {
        // volatile to ensure it gets re-loaded while we're waiting for it to
        // get updated from within the i2c irq
        volatile int remaining;
        uint8_t *ptr;
        uint8_t data[64];

        void ALWAYS_INLINE reset() {
            remaining = 0;
            ptr = data;
        }
    };

    static AckPacket ackPacket;
    static I2CUsbPayload i2cUsbPayload;
    static I2CWriteTransaction cubeWrite;
    static NeighborRxData neighborRxData;
};

#endif // _TEST_JIG_H
