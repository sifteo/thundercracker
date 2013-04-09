/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "swd.h"

#define SWD_RST_CYCLES  200
#define SWD_REQ_CYCLES  8
#define SWD_ACK_CYCLES  3
#define SWD_DAT_CYCLES  32

#define SWD_ACK_OK      1
#define SWD_ACK_WAIT    2
#define SWD_ACK_ERR     4
#define SWD_ACK_DISC    7

#define SWD_ID_REQ      0xa5

void SWDMaster::init()
{
    swdclk.setLow();
    swdclk.setControl(GPIOPin::OUT_2MHZ);
    swdio.setHigh();
    swdio.setControl(GPIOPin::OUT_2MHZ);
    busyFlag = false;
}

void SWDMaster::startTimer()
{
    busyFlag = true;
    //Tclk = 2*count/72 us
    //swdTimer.init(0x8ca0,0x0);  //1ms
    //swdTimer.init(0xE10,0x0);   //100us
    //swdTimer.init(0x2d0,0x0);   //20us
    swdTimer.init(0xb4,0x0);   //5us
    swdTimer.enableUpdateIsr();
}

void SWDMaster::stopTimer()
{
    swdTimer.deinit();
    busyFlag = false;
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = low
 *  SWDIO   = high
 *  timer   = stopped
 */
void SWDMaster::wkpControllerCB()
{
    enum initphase {
        _idle,
        _reset,
        _done
    };

    static initphase state = _idle;
    static uint32_t cycles = 0;

    if (isRisingEdge()) {
        return;
    }

    fallingEdge();

    switch(state) {
    case _idle:
        cycles = SWD_RST_CYCLES;
        state = _reset;
        //fall through
    case _reset:
        if (--cycles == 0) {
            state=_done;
            swdio.setLow(); // Must send 0 before start bit
            swdio.setControl(GPIOPin::OUT_2MHZ);        }
        break;
    case _done:
        state = _idle;
        //send an ID req immediately
        swdio.setHigh();
        header = SWD_ID_REQ;
        controllerCB = &SWDMaster::txnControllerCB;
        break;
    default:
        break;
    }
}

/*
 *  Leaves finally in the following state
 *  SWDCLK  = low
 *  SWDIO   = high
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
        _reset,
        _done
    };
    static uint32_t buffer = 0;
    static swdphase state = _idle;
    static uint8_t cycles = SWD_REQ_CYCLES;
    static bool parity = false;

    if(isRisingEdge()) {
        return;
    }

    fallingEdge();

    switch(state) {

    case _idle:
        // fall through to header state;
        // start bit has been Xmitted before we enter FSM
        controllerStatus = OK;
        buffer = header<<(24+1);
        cycles = SWD_REQ_CYCLES-1;
        state = _header;

    case _header:
        sendbit(buffer);
        if (--cycles == 0) {
            //buffer should be clear here
            state = _turn1;
        }
        break;

    case _turn1:
        cycles = SWD_ACK_CYCLES;
        state = _ack;
        swdio.setControl(GPIOPin::IN_FLOAT);
        break;

    case _ack:
        receivebit(buffer);
        if (--cycles == 0) {
            state = _turn2;
        }
        break;

    case _turn2:
        buffer >>= (32-SWD_ACK_CYCLES);  //Right-aligned
        switch (buffer) {
        case SWD_ACK_OK:
            cycles = SWD_DAT_CYCLES;
            state = _data;
            if (isDAPRead()) {
                buffer = 0;
            } else {
                swdio.setControl(GPIOPin::OUT_2MHZ);
                buffer = payloadOut;
            }
            break;
        case SWD_ACK_WAIT:
            //end sequence and send next start bit
            swdio.setHigh();
            swdio.setControl(GPIOPin::OUT_2MHZ);
            controllerStatus = ACKWAIT;
            state = _done;
            break;
        default:
            swdio.setHigh();
            swdio.setControl(GPIOPin::OUT_2MHZ);
            controllerStatus = ACKERROR;
            cycles = SWD_RST_CYCLES-1;
            state = _reset;
            break;
        }
        //fall through if no error
        if (controllerStatus == OK) {
            parity = false; //reset parity
        } else {
            break;
        }

    case _data:
        if (isDAPRead()) {
            receivebit(buffer);
        } else {
            sendbit(buffer);
        }
        parity ^= swdio.isHigh();
        if (--cycles == 0) {
            state = _parity;
        }
        break;

    case _parity:
        // TODO implement parity check
        if (isDAPRead()) {
            //check parity
            if (swdio.isHigh() ^ parity) {
                //parity fail
                payloadIn = 0;
                controllerStatus = PARITY;
                cycles = SWD_RST_CYCLES;
                state = _reset;
                break;
            } else {
                payloadIn = buffer;
            }
        } else {
            //write parity
            if (parity) {
                swdio.setHigh();
            } else {
                swdio.setLow();
            }
        }
        state = _turn3;
        break;

    case _turn3:
        //The next start bit
        swdio.setHigh();
        swdio.setControl(GPIOPin::OUT_2MHZ);
        state = _done;
        break;

    case _done:
        stopTimer();
        state = _idle;
        break;

    case _reset:
        if (--cycles == 0) {
            //Must send a 0 after reset sequence
            swdio.setLow();
            swdio.setControl(GPIOPin::OUT_2MHZ);
            state = _turn3;
        }
        break;

    default:
        //shouldnt come here
        //TODO assert
        break;
    }

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
