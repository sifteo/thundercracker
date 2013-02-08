/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF8001 Bluetooth Low Energy controller
 */

#include "nrf8001/nrf8001.h"
#include "nrf8001/services.h"
#include "board.h"
#include "sampleprofiler.h"
#include "systime.h"

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
    numSetupPacketsSent = 0;

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

void NRF8001::produceCommand()
{
    // Do we need to send more SETUP data before the controller is initialized?
    if (numSetupPacketsSent < NB_SETUP_MESSAGES)
        return produceSetupCommand();

    // Nothing to do.
    txBuffer.length = 0;
}

void NRF8001::handleEvent()
{
}

void NRF8001::produceSetupCommand()
{
    // Thanks a lot, Nordic, this format is terrible.
    static const struct {
        uint8_t unused;
        uint8_t data[ACI_PACKET_MAX_LEN];
    } packets[] = SETUP_MESSAGES_CONTENT;

    STATIC_ASSERT(sizeof txBuffer == sizeof packets[0].data);
    memcpy(&txBuffer, packets[numSetupPacketsSent++].data, sizeof txBuffer);
    requestTransaction();
}
