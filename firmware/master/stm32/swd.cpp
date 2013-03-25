/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "swd.h"

#define SWD_WKP_CYCLES 150
#define SWD_REQ_CYCLES 8
#define SWD_ACK_CYCLES 3
#define SWD_DAT_CYCLES 32

#define SWD_ACK_OK      1
#define SWD_ACK_WAIT    2
#define SWD_ACK_ERR     4
#define SWD_ACK_DISC    7

void SWDMaster::init()
{
    swdclk.setControl(GPIOPin::OUT_2MHZ);
    swdclk.setLow();
    swdio.setControl(GPIOPin::OUT_2MHZ);
    swdio.setHigh();
}

void SWDMaster::startTimer()
{
    busyFlag = true;
    swdTimer.init(0x7fff,0x0);
    swdTimer.enableUpdateIsr();
}

void SWDMaster::stopTimer()
{
    swdTimer.deinit();
    busyFlag = false;
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = high
 *  SWDIO   = unchanged (but stays high throughout)
 *  timer   = stopped
 */
void SWDMaster::wkpControllerCB()
{
    static uint32_t cycles = SWD_WKP_CYCLES;

    if (isRisingEdge()) {
        return;
    }

    if(--cycles) {
        fallingEdge();
    } else {
        cycles = SWD_WKP_CYCLES;
        stopTimer();
    }
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = high (TODO: set to low when exiting debug mode)
 *  SWDIO   = done-care (TODO: set to high when exiting debug mode)
 *  timer   = stopped
 */
void SWDMaster::txnControllerCB()
{
    enum swdphase {
        _idle,
        _header,
        _turn1,
        _ack,
        _turn2,
        _data,
        _parity,
        _turn3,
        _done
    };
    static uint32_t buffer = 0;
    static swdphase state = _idle;
    static uint8_t cycles = SWD_REQ_CYCLES;

    if(isRisingEdge()) {
        if (state == _done) {
            state = _idle;
            stopTimer();
        }
        return;
    }

    switch(state) {
    case _idle:
        // fall through to header state;
        buffer = header<<24;
        cycles = SWD_REQ_CYCLES;
        state = _header;
    case _header:
        sendbit(buffer);
        if (--cycles == 0) {
            //buffer should be clear here
            state = _turn1;
        }
        break;
    case _turn1:
        swdio.setControl(GPIOPin::IN_FLOAT);
        cycles = SWD_ACK_CYCLES;
        state = _ack;
        break;
    case _ack:
        receivebit(buffer);
        if (--cycles == 0) {
            state = _turn2;
        }
        break;
    case _turn2:
        if (buffer == SWD_ACK_OK) {
            cycles = SWD_DAT_CYCLES;
            state = _data;
            if (!isDAPRead()) {
                // wait additional clock cycle for DAP write
                swdio.setControl(GPIOPin::OUT_2MHZ);
                buffer = payloadOut;
                break;
            }
        } else {
            // TODO handle wait/error case differently
            swdio.setControl(GPIOPin::OUT_2MHZ);
            swdio.setLow();
            state = _done;
        }
        //fall through to data for DAP read
        buffer = 0;
    case _data:
        if (isDAPRead()) {
            receivebit(buffer);
        } else {
            sendbit(buffer);
        }
        if (--cycles == 0) {
            state = _parity;
        }
        break;
    case _parity:
        // TODO implement parity check
        if (isDAPRead()) {
            //read and compare parity
            state = _turn3;
        } else {
            //compute and write parity
            state = _done;
        }
        break;
    case _turn3:
        swdio.setHigh();
        swdio.setControl(GPIOPin::OUT_2MHZ);
        state = _done;
        break;
    case _done:

    default:
        //shouldnt come here
        //TODO assert
        break;
    }

    fallingEdge();
}

bool SWDMaster::enableRequest()
{
    if(busyFlag)
        return false;

    controllerCB = &SWDMaster::wkpControllerCB;
    startTimer();
    return true;
}

bool SWDMaster::readRequest(uint8_t hdr)
{
    if(busyFlag)
        return false;

    header = hdr;
    controllerCB = &SWDMaster::txnControllerCB;
    startTimer();
    return true;
}

bool SWDMaster::writeRequest(uint8_t hdr, uint32_t payload)
{
    if(busyFlag)
        return false;

    header = hdr; payloadOut = payload;
    controllerCB = &SWDMaster::txnControllerCB;
    startTimer();
    return true;
}

void SWDMaster::isr()
{
    // Acknowledge IRQ by clearing timer status
    swdTimer.clearStatus();
    (this->*controllerCB)();
}
