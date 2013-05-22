/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF8001 Bluetooth Low Energy controller.
 *
 * This object handles all of the particulars of Bluetooth LE,
 * and exports a simple packet interface based on a pair of GATT
 * characteristics used as dumb input and output pipes. This plugs
 * into the hardware-agnostic BTProtocolHandler object.
 */

#ifndef _NRF8001_H
#define _NRF8001_H

#include "btprotocol.h"
#include "spi.h"
#include "gpio.h"
#include "macros.h"

class NRF8001 {
public:
    NRF8001(GPIOPin _reqn,
            GPIOPin _rdyn,
            SPIMaster _spi)
        : reqn(_reqn), rdyn(_rdyn), spi(_spi)
        {}

    static NRF8001 instance;

    void init();
    void isr();
    void task();    // Tasks::BluetoothDriver

    /*
     * The routines below are intended to be called only from FactoryTest.
     *
     * We perform testing in 3 phases, each with its own mini state machine.
     *
     * 1. Enter Test mode
     *    Run an Echo test
     *    Report Echo test results
     *
     * 2. Begin DTM RX *or* Begin DTM TX test
     *
     * 3. Complete test
     *    Report test results
     *    Exit Test mode
     */

    enum TestPhase {
        EnterTestMode,
        RXTest,
        TXTest,
        ExitTestMode
    };

    void test(unsigned phase, uint8_t pkt, uint8_t len, uint8_t freq);

private:
    struct ACICommandBuffer {
        uint8_t length;
        uint8_t command;
        union {
            uint8_t param[30];
            uint16_t param16[15];
        };
    };

    struct ACIEventBuffer {
        uint8_t debug;
        uint8_t length;
        uint8_t event;
        uint8_t param[29];
    };

    struct DynamicData {
        /*
         * nRF8001 "Dynamic Data" includes link keys that we want to store in SysLFS.
         * This is a tiny buffer that holds one packet's worth of state, shuttling it
         * between ISR and Task context.
         *
         * The 'record' struct defines the SysLFS value. Sequence number is stored in
         * the SysLFS key. Length is stored explicitly, since we need byte granularity.
         */

        uint8_t state;
        uint8_t sequence;

        struct {
            uint8_t length;
            uint8_t continued;
            uint8_t data[26];

            int calculatedLength() const {
                return length + sizeof *this - sizeof data;
            }

            uint8_t *bytes() {
                return reinterpret_cast<uint8_t*>(this);
            }
        } record;
    };

    enum DynamicDataState {
        DynStateIdle = 0,
        DynStateLoadRequest,
        DynStateLoadComplete,
        DynStateStoreRequest,
        DynStateStoreComplete,
    };

    GPIOPin reqn;
    GPIOPin rdyn;
    SPIMaster spi;

    // for access to rdyn in a shared IRQ vector - see exti.cpp
    friend void ISR_FN(NRF8001_EXTI_VEC)();

    // Owned by ISR context
    ACICommandBuffer txBuffer;
    ACIEventBuffer rxBuffer;
    bool requestsPending;        // Need at least one more request after the current one finishes
    bool isBonded;               // Have we bonded (paired) with another device?
    bool isSetupFinished;        // Did we get confirmation that SETUP completed?
    bool isUnbonding;            // Do we want to ignore bonding data temporarily?
    uint8_t sysCommandState;     // produceSystemCommand() state machine
    uint8_t sysCommandPending;   // Are we waiting on a response to a command?
    uint8_t testState;           // Requested SystemCommandState to get into Test mode
    uint8_t dataCredits;         // Number of data packets we're allowed to send
    uint8_t openPipes;           // First 8 bits of the nRF8001's open pipes bitmap

    // Shared between ISR and Task
    DynamicData dyn;

    static void staticSpiCompletionHandler();
    void onSpiComplete();

    // Low-level ACI interface
    void requestTransaction();   // Ask nicely for produceCommand() to be called once.
    void produceCommand();       // Fill the txBuffer if we can. ISR context only.
    void handleEvent();          // Consume the rxBuffer. ISR context only.

    // Mid-level ACI utilities
    void handleCommandStatus(unsigned command, unsigned status);
    void handleDtmResponse(unsigned status, uint16_t response);
    bool produceSystemCommand();

    friend class BTProtocolHardware;
};

#endif
