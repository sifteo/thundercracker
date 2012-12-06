/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF24L01 radio
 */

#include "nrf24l01.h"
#include "debug.h"
#include "board.h"
#include "sampleprofiler.h"

NRF24L01 NRF24L01::instance(RF_CE_GPIO,
                            RF_IRQ_GPIO,
                            SPIMaster(&RF_SPI,              // SPI:
                                      RF_SPI_CSN_GPIO,      //   CSN
                                      RF_SPI_SCK_GPIO,      //   SCK
                                      RF_SPI_MISO_GPIO,     //   MISO
                                      RF_SPI_MOSI_GPIO,     //   MOSI
                                      staticSpiCompletionHandler));

void NRF24L01::init()
{

    STATIC_ASSERT(PacketTransmission::MAX_HARDWARE_RETRIES == MAX_HW_RETRIES);

    /*
     * Common hardware initialization, regardless of radio usage mode.
     */

    const SPIMaster::Config cfg = {
        Dma::MediumPrio,
        SPIMaster::fPCLK_4
    };
    spi.init(cfg);

    ce.setLow();
    ce.setControl(GPIOPin::OUT_10MHZ);

    irq.setControl(GPIOPin::IN_FLOAT);
    irq.irqInit();
    irq.irqSetFallingEdge();

    transmitPower = PacketTransmission::dBm0;

    const uint8_t radio_setup[]  = {
        /* Enable nRF24L01 features */
        2, CMD_W_REGISTER | REG_FEATURE,        0x07,
        
        /* Enable receive pipe 0, to support auto-ack */
        2, CMD_W_REGISTER | REG_DYNPD,          0x01,
        2, CMD_W_REGISTER | REG_EN_RXADDR,      0x01,
        2, CMD_W_REGISTER | REG_EN_AA,          0x01,

        /* Max ACK payload size */
        2, CMD_W_REGISTER | REG_RX_PW_P0,       32,
        
        /* Auto retry delay, 500us, 15 retransmits */
        2, CMD_W_REGISTER | REG_SETUP_RETR,     AUTO_RETRY_DELAY | hardRetries,

        /* 5-byte address width */
        2, CMD_W_REGISTER | REG_SETUP_AW,       0x03,

        /* 2 Mbit, max transmit power */
        2, CMD_W_REGISTER | REG_RF_SETUP,       0x0e,

        /* Discard any packets queued in hardware */
        1, CMD_FLUSH_RX,
        1, CMD_FLUSH_TX,
        
        /* Clear write-once-to-clear bits */
        2, CMD_W_REGISTER | REG_STATUS,         0x70,

        0
    };
    spi.transferTable(radio_setup);

    /*
     * Enable the IRQ _after_ clearing the interrupt status and FIFOs
     * above, but before sending the first transmit packet. We don't
     * want to miss the first legit edge, but we also don't want to
     * trigger spuriously during init.
     */
    irq.irqEnable();

    /*
     * Setup for Primary Transmitter (PTX) mode
     */
    const uint8_t ptx_setup[]  = {
        /* Discard any packets queued in hardware */
        1, CMD_FLUSH_RX,
        1, CMD_FLUSH_TX,

        /* Clear write-once-to-clear bits */
        2, CMD_W_REGISTER | REG_STATUS,         0x70,

        /* 16-bit CRC, radio enabled, IRQs enabled */
        2, CMD_W_REGISTER | REG_CONFIG,         0x0e,

        0
    };
    spi.transferTable(ptx_setup);
}

/*
 * Test mode support for going to PRX-mode.
 * Mask Rx interrupt to skip any packet handling
 * Hold CE high
 */
void NRF24L01::setPRXMode(bool enabled)
{
    if (enabled) {
        /* Radio in PRX mode with IRQs masked */

        spi.begin();
        spi.transfer(CMD_W_REGISTER | REG_CONFIG);
        spi.transfer(0x7f);
        spi.end();

        ce.setHigh();

    } else {

        /* Radio back to PTX mode */
        const uint8_t ptx_setup[]  = {
            /* Discard any packets queued in hardware */
            1, CMD_FLUSH_RX,
            1, CMD_FLUSH_TX,

            /* Clear write-once-to-clear bits */
            2, CMD_W_REGISTER | REG_STATUS,         0x70,

            /* 16-bit CRC, radio enabled, IRQs enabled */
            2, CMD_W_REGISTER | REG_CONFIG,         0x0e,

            0
        };
        spi.transferTable(ptx_setup);

        ce.setLow();
    }
}

/*
 * Test mode support for emitting a constant carrier on the given channel.
 */
void NRF24L01::setConstantCarrier(bool enabled, unsigned channel)
{
    // From the L01 datasheet:
    //    1. Set PWR_UP  = 1 and  PRIM_RX = 0 in the  CONFIG  register.
    //    2. Wait 1.5ms  PWR_UP ->standby.
    //    3. In the RF register set:
    //    X CONT_WAVE = 1.
    //    X PLL_LOCK  = 1.
    //    X RF_PWR.
    //    4. Set the wanted RF channel.
    //    5. Set CE high.
    //    6. Keep CE high as long as the carrier is needed.

    if (enabled) {
        ce.setLow();

        spi.begin();
        spi.transfer(CMD_W_REGISTER | REG_RF_SETUP);
        spi.transfer(0x0e | (1 << 7) | (1 << 4));
        spi.end();

        spi.begin();
        spi.transfer(CMD_W_REGISTER | REG_RF_CH);
        spi.transfer(channel);
        spi.end();

        ce.setHigh();
    } else {

        spi.begin();
        spi.transfer(CMD_W_REGISTER | REG_RF_SETUP);
        spi.transfer(0x0e);
        spi.end();

        ce.setLow();
    }
}

void NRF24L01::setChannel(uint8_t ch)
{
    spi.begin();
    spi.transfer(CMD_W_REGISTER | REG_RF_CH);
    spi.transfer(ch);
    spi.end();
}

uint8_t NRF24L01::channel()
{
    spi.begin();
    spi.transfer(CMD_R_REGISTER | REG_RF_CH);
    uint8_t ch = spi.transfer(0);
    spi.end();

    return ch;
}

void NRF24L01::isr()
{
    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::RFISR);

    // Acknowledge to the IRQ controller
    irq.irqAcknowledge();

    // Let the radio watchdog know we're okay
    irqCount++;

    /*
     * Read the NRF STATUS register, then write to clear. This tells
     * us which IRQ(s) occurred, and acknowledges them to the nRF
     * chip.
     */
    spi.begin();
    uint8_t status = spi.transfer(CMD_W_REGISTER | REG_STATUS) & (MAX_RT | RX_DR | TX_DS);
    spi.transfer(status);
    spi.end();

    switch (status) {

    case 0:
        // Shouldn't happen, but.. electrical noise maybe?
        UART("Spurious nRF IRQ!\r\n");
        break;

    case MAX_RT:
        // Unsuccessful transmit
        handleTimeout();
        break;

    case TX_DS:
        // Successful transmit, no ACK data
        ackEmpty(retryCount());
        beginTransmitting();
        break;

    case TX_DS | RX_DR:
        // Successful transmit, with an ACK
        beginReceive();
        // The next transmission begins once this receive is completed
        break;

    default:
        // Other cases are not allowed. Do something non-fatal...
        UART("Unhandled nRF IRQ status!\r\n");
        beginTransmitting();
        break;
    }

    SampleProfiler::setSubsystem(s);
}

void NRF24L01::handleTimeout()
{
    /*
     * Hardware timeout occurred.
     *
     * We support software retries, up to a point. Past that, pass the
     * timeout on to RadioManager so it can take more permanent action.
     */

    if (softRetriesLeft) {
        /*
         * Retry.. again. The packet is still in our buffer.
         */
        softRetriesLeft--;
        pulseCE();

    } else {
        /*
         * Out of luck. Discard the packet, and pass on the error. Then transmit a new packet.
         */

        spi.begin();
        spi.transfer(CMD_FLUSH_TX);
        spi.end();

        txnState = Idle;
        timeout();
        beginTransmitting();
    }
}

void NRF24L01::beginReceive()
{
    /*
     * A packet has been received. Dequeue it from the hardware
     * buffer, and pass it on to RadioManager.
     *
     * Called from interrupt context.
     */

    txnState = RXStatus;
    spi.begin();
    // NOTE: reusing rxData for these status bytes
    rxData[0] = CMD_R_RX_PL_WID;
    spi.transferDma(rxData, rxData, 2);
}
 
void NRF24L01::beginTransmitting()
{
    /*
     * This is an opportunity to transmit. Ask RadioManager to produce
     * a packet, then send it to the radio.
     *
     * Called from interrupt OR non-interrupt context. It's possible
     * for this function to be re-entered, but only in limited
     * ways. We assume that re-entry only occurs after ce.setHigh().
     */

    if (!RadioManager::isRadioEnabled()) {
        // Do nothing. This will break the cycle of transmit/irq,
        // until the heartbeat task wakes us up again.
        txnState = Idle;
        return;
    }

    txBuffer.init();
    produce(txBuffer);

    softRetriesLeft = txBuffer.numSoftwareRetries;

#ifdef DEBUG_MASTER_TX
    Debug::logToBuffer(txBuffer.packet.bytes, txBuffer.packet.len);
#endif

    /*
     * Fire off our transmit process. Subsequent steps are performed in the
     * SPI completion event.
     */

    txnState = TXChannel;
    spi.begin();
    txAddressBuffer[0] = CMD_W_REGISTER | REG_RF_CH;
    txAddressBuffer[1] = txBuffer.dest->channel;
    spi.txDma(txAddressBuffer, 2);
}

void NRF24L01::pulseCE()
{
    /*
     * Pulse CE for at least 10us to start transmitting.
     *
     * This is a little bit sneaky, but use a dummy transaction on the SPI bus
     * (note, no chip select) to time the pulse. The SPI peripheral consumes
     * less power than it would take to fire up another timer peripheral,
     * so it's a bit cheaper this way.
     *
     * The pulse ends in the spi completion handler.
     *
     * We're clocking the nRF SPI @ 9MHz, making a single clock cycle ~11ns.
     * So we need to send at least (10us / 11ns) / 8 (bits/byte) = 12 bytes.
     * Bump up to 15 for a little margin.
     */

    txnState = TXPulseCE;
    ce.setHigh();
    spi.txDma(txData, 15);
}

void NRF24L01::staticSpiCompletionHandler()
{
    NRF24L01::instance.onSpiComplete();
}

void NRF24L01::onSpiComplete()
{
    /*
     * An SPI transmission has completed.
     * NRF transmit and receive operations consist of multiple SPI transmissions,
     * so step to the next state and execute accordingly.
     *
     * NOTE: the RX states are only executed in the event that data arrived on one
     * of the ACK packets that we've received. Otherwise, we start in directly on
     * the TX states.
     */

    SampleProfiler::SubSystem s = SampleProfiler::subsystem();
    SampleProfiler::setSubsystem(SampleProfiler::RFISR);

    spi.end();
    switch (txnState) {

    case RXStatus:
        /*
         * we've read the number of bytes that have arrived. Check for error,
         * and proceed to reading the payload data.
         */
        rxBuffer.len = rxData[1];
        if (rxBuffer.len > rxBuffer.MAX_LEN) {
            /*
             * Receive error. The data sheet requires that we flush the RX
             * FIFO.  We'll count this as a timeout.
             */

            spi.begin();
            spi.transfer(CMD_FLUSH_RX);
            spi.end();

            txnState = Idle;
            timeout();
            beginTransmitting();
            break;
        }

        txnState = RXPayload;
        rxData[0] = CMD_R_RX_PAYLOAD;
        spi.begin();
        spi.transferDma(rxData, rxData, rxBuffer.len + 1);
        break;

    case RXPayload:
        ackWithPacket(rxBuffer, retryCount());
        beginTransmitting();
        break;

    case TXChannel:
        txnState = TXAddressTx;
        spi.begin();
        txAddressBuffer[0] = CMD_W_REGISTER | REG_TX_ADDR;
        memcpy(txAddressBuffer + 1, txBuffer.dest->id, sizeof txBuffer.dest->id);
        spi.txDma(txAddressBuffer, sizeof(txBuffer.dest->id) + 1);
        break;

    case TXAddressTx:
        txnState = TXAddressRx;
        spi.begin();
        txAddressBuffer[0] = CMD_W_REGISTER | REG_RX_ADDR_P0;
        memcpy(txAddressBuffer + 1, txBuffer.dest->id, sizeof txBuffer.dest->id);
        spi.txDma(txAddressBuffer, sizeof(txBuffer.dest->id) + 1);
        break;

    case TXAddressRx:
        // Set retry count if necessary, otherwise FALL THROUGH to RF_SETUP.
        if (txBuffer.numHardwareRetries != hardRetries) {
            txnState = TXRfSetup;
            spi.begin();
            txAddressBuffer[0] = CMD_W_REGISTER | REG_SETUP_RETR;
            txAddressBuffer[1] = AUTO_RETRY_DELAY | txBuffer.numHardwareRetries;
            spi.txDma(txAddressBuffer, 2);
            break;
        }

    case TXRfSetup:
         // Set transmit power if necessary, otherwise FALL THROUGH to payload.
         if (txBuffer.txPower != transmitPower) {
             txnState = TXSetupRetr;
             spi.begin();
             txAddressBuffer[0] = CMD_W_REGISTER | REG_RF_SETUP;
             // enforce 2Mbit/sec transfer rate
             txAddressBuffer[1] = 0x08 | (txBuffer.txPower & 0x6);
             spi.txDma(txAddressBuffer, 2);
             break;
         }

    case TXSetupRetr:
        hardRetries = txBuffer.numHardwareRetries;
        transmitPower = txBuffer.txPower;
        txnState = TXPayload;
        spi.begin();
        txData[0] = txBuffer.noAck ? CMD_W_TX_PAYLOAD_NO_ACK : CMD_W_TX_PAYLOAD;
        spi.txDma(txData, txBuffer.packet.len + 1);
        break;

    case TXPayload:
        pulseCE();
        break;

    case TXPulseCE:
        if (softRetriesLeft == 0) {
            txnState = Idle;
        }
        ce.setLow();
        break;

    case Idle:
        break;
    }

    SampleProfiler::setSubsystem(s);
}
