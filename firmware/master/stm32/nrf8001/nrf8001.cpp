/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF8001 Bluetooth Low Energy controller
 */

#include "nrf8001/nrf8001.h"
#include "nrf8001/services.h"
#include "nrf8001/constants.h"
#include "board.h"
#include "sampleprofiler.h"
#include "systime.h"

/*
 * Hardware instance
 */

#ifdef HAVE_NRF8001

NRF8001 NRF8001::instance(NRF8001_REQN_GPIO,
                          NRF8001_RDYN_GPIO,
                          SPIMaster(&NRF8001_SPI,
                                    NRF8001_SCK_GPIO,
                                    NRF8001_MISO_GPIO,
                                    NRF8001_MOSI_GPIO,
                                    staticSpiCompletionHandler));

IRQ_HANDLER ISR_FN(NRF8001_EXTI_VEC)()
{
    NRF8001::instance.isr();
}

#endif // HAVE_NRF8001

/*
 * States for our produceSystemCommand() state machine.
 */
namespace SysCS {
    enum SystemCommandState {
        SetupFirst = 0,
        SetupLast = SetupFirst + NB_SETUP_MESSAGES - 1,
        Idle,           // Must follow SetupLast
        BeginConnect,
        RadioReset,
        InitSysVersion,
    };
}


void NRF8001::init()
{
    // 3 MHz maximum SPI clock according to data sheet. Mode 0, LSB first.
    const SPIMaster::Config cfg = {
        Dma::MediumPrio,
        SPIMaster::fPCLK_16 | SPIMaster::fLSBFIRST
    };

    spi.init(cfg);

    // Reset state
    txBuffer.length = 0;
    requestsPending = 0;
    dataCredits = 0;
    sysCommandState = SysCS::RadioReset;
    sysCommandPending = false;

    // Output pin, requesting a transaction
    reqn.setHigh();
    reqn.setControl(GPIOPin::OUT_10MHZ);

    // Input IRQ pin, beginning a (requested or spontaneous) transaction
    rdyn.setControl(GPIOPin::IN_FLOAT);
    rdyn.irqInit();

    /*
     * The RDYN level isn't valid until at least 62ms after reset, according
     * to the data sheet. This is a conservative delay, but still shorter
     * than Radio::init()'s delay.
     */
    while (SysTime::ticks() < SysTime::msTicks(80));

    // Now we can enable the IRQ
    rdyn.irqSetFallingEdge();
    rdyn.irqEnable();

    // Ask for the first transaction, so we can start the SETUP process.
    requestTransaction();
}

void NRF8001::isr()
{
    /*
     * This ISR triggers when there's a falling edge on RDYN.
     * This is our signal to do one SPI transaction, which consists
     * of an optional command (out) an an optional event (in).
     *
     * We currently always perform a maximum-length transaction
     * (32 bytes) in order to avoid having to split the transaction
     * into two pieces to handle the length byte and the payload
     * separately.
     */

    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::BluetoothISR);

    // Acknowledge to the IRQ controller
    rdyn.irqAcknowledge();

    /*
     * Set REQN low to indicate we're ready to start the transaction.
     * Effectively, the nRF8001's virtual "chip select" is (REQN && RDYN).
     * If this ISR was due to a command rather than an event, REQN will
     * already be low and this has no effect.
     *
     * Note that this must happen prior to produceCommand(). In case
     * that function calls requestTransaction(), we must know that we're
     * already in a transaction.
     */
    reqn.setLow();

    // Populate the transmit buffer now, or set it empty if we have nothing to say.
    produceCommand();

    // Fire off the asynchronous SPI transfer. We finish up in onSpiComplete().
    STATIC_ASSERT(sizeof txBuffer == sizeof rxBuffer);
    spi.transferDma((uint8_t*) &txBuffer, (uint8_t*) &rxBuffer, sizeof txBuffer);

    SampleProfiler::setSubsystem(s);
}

void NRF8001::staticSpiCompletionHandler()
{
    NRF8001::instance.onSpiComplete();
}

void NRF8001::onSpiComplete()
{
    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::BluetoothISR);

    // Done with the transaction! End our SPI request.
    reqn.setHigh();

    // Handle the event we received, if any.
    // This also may call requestTransaction() to keep the cycle going.
    handleEvent();

    // Start the next pending transaction, if any. (Serialized by requestTransaction)
    if (requestsPending) {
        requestsPending = false;
        reqn.setLow();
    }

    SampleProfiler::setSubsystem(s);
}

void NRF8001::requestTransaction()
{
    /*
     * Ask for produceCommand() to be called once. This can be called from
     * Task context at any time, or from ISR context during produceCommand()
     * or handleEvent(). This is idempotent; multiple calls to requestTransaction()
     * are only guaranteed to lead to a single produceCommand() call.
     *
     * If a transaction is already in progress, this will set 'requestsPending'
     * which will cause another transaction to start in onSpiComplete(). If not,
     * we start the transaction immediately by asserting REQN.
     */

    // Critical section
    NVIC.irqDisable(IVT.NRF8001_EXTI_VEC);
    NVIC.irqDisable(IVT.NRF8001_DMA_CHAN_RX);
    NVIC.irqDisable(IVT.NRF8001_DMA_CHAN_TX);

    if (reqn.isOutputLow()) {
        // Already in a transaction. Pend another one for later.
        requestsPending = true;
    } else {
        reqn.setLow();
    }

    // End critical section
    NVIC.irqEnable(IVT.NRF8001_EXTI_VEC);
    NVIC.irqEnable(IVT.NRF8001_DMA_CHAN_RX);
    NVIC.irqEnable(IVT.NRF8001_DMA_CHAN_TX);
}

void BTProtocolHandler::requestProduceData()
{
    /*
     * The BTProtocolHandler wants us to call onProduceData() at least once.
     *
     * If we request a transaction, this will happen. If we're currently blocked due
     * to flow control, we'll end up requesting a transaction anyway when we get more tokens.
     */

    NRF8001::instance.requestTransaction();
}

void NRF8001::produceCommand()
{
    // System commands are highest priority, but at most one can be pending at a time.
    if (!sysCommandPending && produceSystemCommand()) {
        sysCommandPending = true;
        return;
    }

    // If we can transmit, see if the BTPRotocolHandler wants to.
    if (dataCredits && (openPipes & (1 << PIPE_SIFTEO_BASE_DATA_IN_TX))) {
        unsigned len = BTProtocolHandler::onProduceData(&txBuffer.param[1]);
        if (len) {
            txBuffer.length = len + 2;
            txBuffer.command = Op::SendData;
            txBuffer.param[0] = PIPE_SIFTEO_BASE_DATA_IN_TX;
            return;
        }
    }

    // Nothing to do.
    txBuffer.length = 0;
}

bool NRF8001::produceSystemCommand()
{
    switch (sysCommandState) {

        default:
        case SysCS::Idle:
            return false;

        case SysCS::RadioReset: {
            /*
             * Send a RadioReset command. This may well fail if we aren't setup yet,
             * but we ignore that error. If we experienced a soft reset of any kind, this
             * will ensure the nRF8001 isn't in the middle of anything.
             *
             * After this finishes, we'll start SETUP.
             */

            txBuffer.length = 1;
            txBuffer.command = Op::RadioReset;
            sysCommandState = SysCS::SetupFirst;
            dataCredits = 0;
            return true;
        }

        case SysCS::SetupFirst ... SysCS::SetupLast: {
            /*
             * Send the next SETUP packet.
             * Thanks a lot, Nordic, this format is terrible.
             *
             * After SETUP completes, we'll head to the Idle state.
             * When the device finishes initializing, we'll get a DeviceStartedEvent.
             */

            static const struct {
                uint8_t unused;
                uint8_t data[ACI_PACKET_MAX_LEN];
            } packets[] = SETUP_MESSAGES_CONTENT;

            STATIC_ASSERT(sizeof txBuffer == sizeof packets[0].data);
            STATIC_ASSERT(SysCS::SetupLast - SysCS::SetupFirst == arraysize(packets) - 1);
            STATIC_ASSERT(SysCS::SetupLast + 1 == SysCS::Idle);

            memcpy(&txBuffer, packets[sysCommandState - SysCS::SetupFirst].data, sizeof txBuffer);
            sysCommandState++;

            return true;
        }

        case SysCS::InitSysVersion: {
            /*
             * Send our system version identifier to the nRF8001, to be stored in its RAM.
             * It will handle firmware version reads without bothering us. This is the
             * same version we report to userspace with _SYS_version().
             *
             * This happens after SETUP is finished and we've entered Standby mode, but
             * before initiating a Connect.
             */

            txBuffer.length = 6;
            txBuffer.command = Op::SetLocalData;
            txBuffer.param[0] = PIPE_SIFTEO_BASE_SYSTEM_VERSION_SET;

            uint32_t version = _SYS_version();
            memcpy(&txBuffer.param[1], &version, sizeof version);

            // No more local data to set after this.
            sysCommandState = SysCS::BeginConnect;

            return true;
        };

        case SysCS::BeginConnect: {
            /*
             * After all setup is complete, send a 'Connect' command. This begins the potentially
             * long-running process of looking for a peer. This is what enables advertisement
             * broadcasts.
             *
             * After this command, we'll be idle until a connection event arrives.
             *
             * We use Apple's recommended advertising interval of 20ms here. If we need
             * to save power, we could increase it. See the Apple Bluetooth Design Guidelines:
             *
             * https://developer.apple.com/hardwaredrivers/BluetoothDesignGuidelines.pdf
             */

            txBuffer.length = 5;
            txBuffer.command = Op::Connect;
            txBuffer.param16[0] = 0x0000;       // Infinite duration
            txBuffer.param16[1] = 32;           // 20ms, in 0.625ms units
            sysCommandState = SysCS::Idle;
            return true;
        }

    }
}

void NRF8001::handleEvent()
{
    if (rxBuffer.length == 0) {
        // No pending event.
        return;
    }

    switch (rxBuffer.event) {

        case Op::CommandResponseEvent: {
            /*
             * The last command finished. This is where we would take note of the status
             * if we need to. Only one system command may be pending at a time, so this
             * lets us move to the next command if we want.
             */

            sysCommandPending = false;
            handleCommandStatus(rxBuffer.param[0], rxBuffer.param[1]);
            if (sysCommandState != SysCS::Idle) {
                // More work to do, ask for another transaction.
                requestTransaction();
            }
            return;
        }

        case Op::DeviceStartedEvent: {
            /*
             * The device has changed operating modes. This happens after SETUP
             * finishes, when the device enters Standby mode. When this happens,
             * we want to set up any local data that needs to be sent to the
             * nRF8001's RAM, then initiate a Connect to start broadcasting
             * advertisement packets.
             *
             * This is also where our pool of data credits gets initialized.
             */
            uint8_t mode = rxBuffer.param[0];
            dataCredits = rxBuffer.param[2];

            if (mode == OperatingMode::Standby && sysCommandState == SysCS::Idle) {
                // Start sending local data
                sysCommandState = SysCS::InitSysVersion;
                requestTransaction();
            }
            return;
        }

        case Op::ConnectedEvent: {
            /*
             * Established a connection! We don't actually care, but the BTProtocolHandler might.
             */

            BTProtocolHandler::onConnect();
            return;
        }

        case Op::DisconnectedEvent: {
            /*
             * One connection ended; start trying to establish another.
             */

            sysCommandState = SysCS::BeginConnect;
            openPipes = 0;
            requestTransaction();
            BTProtocolHandler::onDisconnect();
            return;
        }

        case Op::PipeStatusEvent: {
            /*
             * This event contains two 64-bit bitmaps, indicating which
             * pipes are open and which ones are closed and require
             * opening prior to use.
             *
             * This is a form of flow control. Data credits are flow
             * control at the ACI level, pipe status is flow control
             * at the per-pipe level. This is how we know that the peer
             * will be listening when we transmit.
             *
             * We use very few pipes, since we're using the nRF8001 mostly
             * lke a dumb serial pipe rather than a normal GATT device.
             * So, we won't bother storing the whole bitmap.
             * 
             * This may mean we can now send data wheras before we couldn't,
             * so we'll request a transaction in case we need to transmit.
             */

             openPipes = rxBuffer.param[0];     // Just the LSB of the 'opened' bitmap.
             requestTransaction();
             return;
        }

        case Op::DataReceivedEvent: {
            /*
             * Data received from an nRF8001 pipe.
             */

            int length = int(rxBuffer.length) - 1;
            uint8_t pipe = rxBuffer.param[0];

            if (length > 0 && pipe == PIPE_SIFTEO_BASE_DATA_OUT_RX) {
                BTProtocolHandler::onReceiveData(&rxBuffer.param[1], length);
            }
            return;
        }

        case Op::DataCreditEvent: {
            /*
             * Received flow control credits that allow us to transmit more packets.
             * 
             * This may mean we can now send data wheras before we couldn't,
             * so we'll request a transaction in case we need to transmit.
             */

            dataCredits += rxBuffer.param[0];
            requestTransaction();
            return;
        }
    }
}

void NRF8001::handleCommandStatus(unsigned command, unsigned status)
{
    if (command == Op::RadioReset) {
        /*
         * RadioReset will complain if the device hasn't been setup yet.
         * We care not, since we send the reset just-in-case. Ignore errors here.
         */
        return;
    }

    if (status > ACI_STATUS_TRANSACTION_COMPLETE) {
        /*
         * An error occurred! For now, just try resetting as best we can...
         */
        sysCommandState = SysCS::RadioReset;
    }
}
