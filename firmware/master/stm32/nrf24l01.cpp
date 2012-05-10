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

NRF24L01 NRF24L01::instance(RF_CE_GPIO,
                            RF_IRQ_GPIO,
                            SPIMaster(&RF_SPI,              // SPI:
                                      RF_SPI_CSN_GPIO,      //   CSN
                                      RF_SPI_SCK_GPIO,      //   SCK
                                      RF_SPI_MISO_GPIO,     //   MISO
                                      RF_SPI_MOSI_GPIO));   //   MOSI

void NRF24L01::init() {
    /*
     * Common hardware initialization, regardless of radio usage mode.
     */

    spi.init();

    ce.setLow();
    ce.setControl(GPIOPin::OUT_10MHZ);

    irq.setControl(GPIOPin::IN_FLOAT);
    irq.irqInit();
    irq.irqSetFallingEdge();

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
        2, CMD_W_REGISTER | REG_SETUP_RETR,     0x1f,

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
}

void NRF24L01::ptxMode()
{
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

    /*
     * XXX: Super-Gross hack! Give the radio 100ms right here, to initialize.
     *
     * It needs to be fully ready to transmit a packet by the time we
     * run transmitPacket(), otherwise that first transmit will fail and
     * we'll never get the IRQ that we would be using to start future
     * transmissions.
     *
     * We can do this asynchronously instead, firing off a timer after a while
     * which starts the first transmit. But instead, we'll probably just want
     * to integrate this with some new code that tracks whether we're actively
     * transmitting or not, since there's a lot of overlap between this bugfix
     * and some much needed radio throttling / power management.
     */

    for (unsigned i = 0; i < 100000; i++)
        spi.transfer(0);  // About 1us

    /*
     * Transmit the first packet. Subsequent packets will be sent from
     * within the isr().
     */

    transmitPacket();
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

void NRF24L01::setTxPower(Radio::TxPower pwr)
{
    spi.begin();
    spi.transfer(CMD_W_REGISTER | REG_RF_SETUP);
    spi.transfer(0x08 | pwr);   // enforce 2Mbit/sec transfer rate
    spi.end();
}

Radio::TxPower NRF24L01::txPower()
{
    spi.begin();
    spi.transfer(CMD_R_REGISTER | REG_RF_SETUP);
    uint8_t setup = spi.transfer(0);
    spi.end();

    return static_cast<Radio::TxPower>(setup);
}

void NRF24L01::isr()
{
    // Acknowledge to the IRQ controller
    irq.irqAcknowledge();

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
        Debug::log("Spurious nRF IRQ!");
        break;

    case MAX_RT:
        // Unsuccessful transmit
        handleTimeout();
        break;

    case TX_DS:
        // Successful transmit, no ACK data
        RadioManager::ackEmpty();
        transmitPacket();
        break;

    case TX_DS | RX_DR:
        // Successful transmit, with an ACK
        receivePacket();
        transmitPacket();
        break;

    default:
        // Other cases are not allowed. Do something non-fatal...
        Debug::log("Unhandled nRF IRQ status");
        transmitPacket();
        break;
    }
}

void NRF24L01::handleTimeout()
{
    /*
     * Hardware timeout occurred.
     *
     * We support software retries, up to a point. Past that, pass the
     * timeout on to RadioManager so it can take more permanent action.
     */

    if (--softRetriesLeft) {
        /*
         * Retry.. again. The packet is still in our buffer.
         */

        pulseCE();

    } else {
        /*
         * Out of luck. Discard the packet, and pass on the error. Then transmit a new packet.
         */

        spi.begin();
        spi.transfer(CMD_FLUSH_TX);
        spi.end();
        
        RadioManager::timeout();
        transmitPacket();
    }
}

void NRF24L01::receivePacket()
{
    /*
     * A packet has been received. Dequeue it from the hardware
     * buffer, and pass it on to RadioManager.
     *
     * Called from interrupt context.
     */

    spi.begin();
    spi.transfer(CMD_R_RX_PL_WID);
    rxBuffer.len = spi.transfer(0);
    spi.end();

    if (rxBuffer.len > rxBuffer.MAX_LEN) {
        /*
         * Receive error. The data sheet requires that we flush the RX
         * FIFO.  We'll count this as a timeout.
         */

        spi.begin();
        spi.transfer(CMD_FLUSH_RX);
        spi.end();

        RadioManager::timeout();
        return;
    }

    spi.begin();
    spi.transfer(CMD_R_RX_PAYLOAD);
    spi.transferDma(rxBuffer.bytes, rxBuffer.bytes, rxBuffer.len);
    while (spi.dmaInProgress())
        ;
    spi.end();

    RadioManager::ackWithPacket(rxBuffer);
}
 
void NRF24L01::transmitPacket()
{
    /*
     * This is an opportunity to transmit. Ask RadioManager to produce
     * a packet, then send it to the radio.
     *
     * Called from interrupt OR non-interrupt context. It's possible
     * for this function to be re-entered, but only in limited
     * ways. We assume that re-entry only occurs after ce.setHigh().
     */

    txBuffer.noAck = false;
    RadioManager::produce(txBuffer);
    softRetriesLeft = SOFT_RETRY_MAX;

#ifdef DEBUG_MASTER_TX
    Debug::logToBuffer(txBuffer.packet.bytes, txBuffer.packet.len);
#endif

    /*
     * Set the tx/rx address and channel
     */

    spi.begin();
    spi.transfer(CMD_W_REGISTER | REG_RF_CH);
    spi.transfer(txBuffer.dest->channel);
    spi.end();

    spi.begin();
    spi.transfer(CMD_W_REGISTER | REG_TX_ADDR);
    spi.txDma(txBuffer.dest->id, sizeof txBuffer.dest->id);
    while (spi.dmaInProgress())
        ;
    spi.end();

    spi.begin();
    spi.transfer(CMD_W_REGISTER | REG_RX_ADDR_P0);
    spi.txDma(txBuffer.dest->id, sizeof txBuffer.dest->id);
    while (spi.dmaInProgress())
        ;
    spi.end();

    /*
     * Enqueue the packet
     */

    spi.begin();
    spi.transfer(txBuffer.noAck ? CMD_W_TX_PAYLOAD_NO_ACK : CMD_W_TX_PAYLOAD);
    spi.txDma(txBuffer.packet.bytes, txBuffer.packet.len);
    while (spi.dmaInProgress())
        ;
    spi.end();

    // Start the transmitter!
    pulseCE();
}

void NRF24L01::pulseCE()
{
    /*
     * Pulse CE for at least 10us to start transmitting.
     *
     * XXX: Big hack. This should be asynchronous, and the timing
     *      should be based on the system clock. Right now I'm just
     *      using a dummy SPI transaction for timing purposes. Gross!
     *      Okay, well, maybe using SPI transactions for timing isn't
     *      the worst idea ever. But doing this all synchronously
     *      in the ISR is pretty bad!
     */

    ce.setHigh();
    for (unsigned i = 0; i < 10; i++)
        spi.transfer(0);
    ce.setLow();
}
