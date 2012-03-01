/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "spi.h"
#include "board.h"
#include "dma.h"

void SPIMaster::init()
{
    if (hw == &SPI1) {
        #if BOARD == BOARD_TC_MASTER_REV1
        AFIO.MAPR |= (0x4 << 24) |  // disable JTAG so we can talk to flash
                     (1 << 0);      // remap SPI1 to PB3-5
        #endif
        RCC.APB2ENR |= (1 << 12);

        dmaRxChan = &DMA1.channels[1];  // DMA1, channel 2
        Dma::registerHandler(&DMA1, 1, dmaCallback, this);

        dmaTxChan = &DMA1.channels[2];  // DMA1, channel 3
        Dma::registerHandler(&DMA1, 2, dmaCallback, this);
    }
    else if (hw == &SPI2) {
        RCC.APB1ENR |= (1 << 14);

        dmaRxChan = &DMA1.channels[3];  // DMA1, channel 4
        Dma::registerHandler(&DMA1, 3, dmaCallback, this);

        dmaTxChan = &DMA1.channels[4];  // DMA1, channel 5
        Dma::registerHandler(&DMA1, 4, dmaCallback, this);
    }
    else if (hw == &SPI3) {
        #if BOARD == BOARD_TC_MASTER_REV1
        AFIO.MAPR |= (1 << 28);     // remap SPI3 to PC10-12
        #endif
        RCC.APB1ENR |= (1 << 15);

        dmaRxChan = &DMA2.channels[0];  // DMA2, channel 1
        Dma::registerHandler(&DMA2, 0, dmaCallback, this);

        dmaTxChan = &DMA2.channels[1];  // DMA2, channel 2
        Dma::registerHandler(&DMA2, 1, dmaCallback, this);
    }

    csn.setHigh();
    csn.setControl(GPIOPin::OUT_10MHZ);
    sck.setControl(GPIOPin::OUT_ALT_50MHZ);
    miso.setControl(GPIOPin::IN_FLOAT);
    mosi.setControl(GPIOPin::OUT_ALT_50MHZ);

    // point DMA channels at data register
    dmaRxChan->CPAR = (uint32_t)&hw->DR;
    dmaTxChan->CPAR = (uint32_t)&hw->DR;

    dmaRxChan->CCR = 0;
    dmaTxChan->CCR = 0;

    hw->CR1 =   (1 << 2);   // MSTR - master configuration

    hw->CR2 =   (1 << 2);   // SSOE

    hw->CR1 |=  (1 << 6);   // SPE - enable the spi device
}

void SPIMaster::begin()
{
    csn.setLow();
}

void SPIMaster::end()
{
    csn.setHigh();
}

uint8_t SPIMaster::transfer(uint8_t b)
{
    /*
     * XXX: This is slow, ugly, and power hungry. We should be
     *      doing DMA, and keeping the FIFOs full! And NOT
     *      busy-looping ever!
     */
    hw->DR = b;
    while (!(hw->SR & 1));      // Wait for RX-not-empty
    return hw->DR;
}

/*
    transfer a chunk of data byte-wise, but avoid function call overhead
    for each byte.
    It is acceptable to pass in the same buffer for txbuf & rxbuf params.
*/
void SPIMaster::transfer(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len)
{
    while (len--) {
//        while (!(hw->SR & (1 << 1)));       // wait for previous byte sent
        hw->DR = *txbuf++;          // write next byte

        while (!(hw->SR & 1));      // wait for response
        *rxbuf++ = hw->DR;          // read it
    }
}

void SPIMaster::transferTable(const uint8_t *table)
{
    /*
     * Table-driven transfers: Each is prefixed by a length byte.
     * Terminated by a zero-length transfer.
     */

    uint8_t len;
    while ((len = *table)) {
        table++;

        begin();
        do {
            transfer(*table);
            table++;
        } while (--len);
        end();
    }
}

/*
    Transfer a block of data.
    It is acceptable to pass the same buffer as txbuf and rxbuf.

    NOTE: the error interrupt is enabled for both RX and TX channels,
    but the transfer-complete ISR is only enabled on the RX channel.
    This is because TX always finishes first, so the whole transaction
    is considered complete when the RX is finished.
*/
void SPIMaster::transferDma(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len)
{
    dmaRxChan->CNDTR = len;
    dmaRxChan->CMAR = (uint32_t)rxbuf;
    dmaRxChan->CCR =    (2 << 12)|  // PL - priority level, 2 == HIGH
                        (1 << 7) |  // MINC - memory pointer increment
                        (0 << 4) |  // DIR - direction, 0 == read from peripheral
                        (1 << 3) |  // TEIE - transfer error ISR enable
                        (0 << 2) |  // HTIE - half complete ISR enable
                        (1 << 1) |  // TCIE - transfer complete ISR enable
                        (1 << 0);   // EN - enable DMA channel

    dmaTxChan->CNDTR = len;
    dmaTxChan->CMAR = (uint32_t)txbuf;
    dmaTxChan->CCR =    (2 << 12)|  // PL - priority level, 2 == HIGH
                        (1 << 7) |  // MINC - memory pointer increment
                        (1 << 4) |  // DIR - direction, 1 == read from memory
                        (1 << 3) |  // TEIE - transfer error ISR enable
                        (0 << 1) |  // TCIE - transfer complete ISR enable
                        (1 << 0);   // EN - enable DMA channel

    hw->CR2 |= (1 << 0) |           // RXDMAEN
               (1 << 1);            // TXDMAEN
}

/*
    possible TODO: I attempted to enable only the TX DMA channel in this case,
    which mostly works, but there are some scenarios in which a TX DMA transaction
    following an RX DMA transaction would never fire the completion ISR.

    For now, enable both channels, and provide a dummy mem pointer for RX and
    disable its MINC bit.
*/
void SPIMaster::txDma(const uint8_t *txbuf, unsigned len)
{
    static uint8_t dummy;
    dmaRxChan->CNDTR = len;
    dmaRxChan->CMAR = (uint32_t)&dummy;
    dmaRxChan->CCR =    (0 << 12)|  // PL - priority level, 0 == LOW
                        (0 << 7) |  // MINC - memory pointer increment
                        (0 << 4) |  // DIR - direction, 0 == read from peripheral
                        (1 << 3) |  // TEIE - transfer error ISR enable
                        (1 << 1) |  // TCIE - transfer complete ISR enable
                        (1 << 0);   // EN - enable DMA channel

    dmaTxChan->CNDTR = len;
    dmaTxChan->CMAR = (uint32_t)txbuf;
    dmaTxChan->CCR =    (2 << 12)|  // PL - priority level, 2 == HIGH
                        (1 << 7) |  // MINC - memory pointer increment
                        (1 << 4) |  // DIR - direction, 1 == read from memory
                        (1 << 3) |  // TEIE - transfer error ISR enable
                        (0 << 1) |  // TCIE - transfer complete ISR enable
                        (1 << 0);   // EN - enable DMA channel

    hw->CR2 |= (1 << 0) |           // RXDMAEN
               (1 << 1);            // TXDMAEN
}

/*
    This isn't typically that useful since slaves will not generally spew out
    data unsolicited, but it's here for completeness' sake.
    Typically use transferDma() instead,
    and just reuse the same buffer for both rx & tx.
*/
void SPIMaster::rxDma(uint8_t *rxbuf, unsigned len)
{
    dmaRxChan->CNDTR = len;
    dmaRxChan->CMAR = (uint32_t)rxbuf;

    dmaRxChan->CCR =    (2 << 12)|  // PL - priority level, 2 == HIGH
                        (1 << 7) |  // MINC - memory pointer increment
                        (0 << 4) |  // DIR - direction, 0 == read from peripheral
                        (1 << 3) |  // TEIE - transfer error ISR enable
                        (0 << 2) |  // HTIE - half complete ISR enable
                        (1 << 1) |  // TCIE - transfer complete ISR enable
                        (1 << 0);   // EN - enable DMA channel
    hw->CR2 |= (1 << 0);            // RXDMAEN
}

bool SPIMaster::dmaInProgress() const
{
    // better way to poll this?
    return hw->CR2 & ((1 << 1) | (1 << 0));
}

/*
    Static routine to dispatch DMA events to the appropriate SPIMaster
    instance. It's assumed that the instance was passed as the param to
    Dma::registerHandler().

    We also assume that we're only getting called here on either transfer complete
    events or error events, but not half transfer events.
*/
void SPIMaster::dmaCallback(void *p, uint8_t flags)
{
    SPIMaster *spi = static_cast<SPIMaster*>(p);
    // TODO: error handling

    // if this transfer was TX only our RX data register likely has an overrun error.
    // we need to read a dummy element out of the data register
    // to clear the status register so subsequent operations can proceed
    if (spi->hw->SR & (1 << 6)) {   // OVR - overrun flag
        (void)spi->hw->DR;
    }

    spi->dmaTxChan->CCR = 0;
    spi->dmaRxChan->CCR = 0;
    spi->hw->CR2 &= ~((1 << 1) | (1 << 0)); // disable DMA RX & TX

#if 0
    // debugging
    Usart::Dbg.write("dma cb!\r\n");
    if (flags & Dma::Error) {
        Usart::Dbg.write("  error\r\n");
    }
    if (flags & Dma::Complete) {
        Usart::Dbg.write("  complete\r\n");
    }
    if (flags & Dma::HalfComplete) {
        Usart::Dbg.write("  half complete\r\n");
    }
#endif
}
