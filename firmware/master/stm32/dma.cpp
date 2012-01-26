
#include "dma.h"

/*
 * Several peripherals can be served by a single DMA channel, so this module
 * provides a centralized way to manage which peripherals are signed up
 * for particular DMA channels, and ensure only one is registered at a time.
 */

uint32_t Dma::Ch1Mask = 0;
uint32_t Dma::Ch2Mask = 0;
Dma::DmaHandler_t Dma::Ch1Handlers[7];
Dma::DmaHandler_t Dma::Ch2Handlers[5];

void Dma::registerHandler(volatile DMA_t *dma, int channel, DmaIsr_t func, void *param)
{
    if (dma == &DMA1) {
        if (Ch1Mask & (1 << channel)) {
            // already registered :(
            return;
        }
        if (Ch1Mask == 0) {
            RCC.AHBENR |= (1 << 0);
            dma->IFCR = 0x0FFFFFFF; // clear all ISRs
        }
        Ch1Mask |= (1 << channel); // mark it as enabled
        Ch1Handlers[channel].isrfunc = func;
        Ch1Handlers[channel].param = param;
    }
    else if (dma == &DMA2) {
        if (Ch2Mask & (1 << channel)) {
            // already registered :(
            return;
        }
        if (Ch1Mask == 0) {
            RCC.AHBENR |= (1 << 1);
            dma->IFCR = 0x0FFFFFFF; // clear all ISRs
        }
        Ch2Mask |= (1 << channel); // mark it as enabled
        Ch2Handlers[channel].isrfunc = func;
        Ch2Handlers[channel].param = param;
    }
}

void Dma::unregisterHandler(volatile DMA_t *dma, int channel)
{
    if (dma == &DMA1) {
        Ch1Mask &= ~(1 << channel); // mark it as disabled
        if (Ch1Mask == 0) { // last one? shut it down
            RCC.AHBENR &= ~(1 << 0);
        }
        Ch1Handlers[channel].isrfunc = 0;
        Ch1Handlers[channel].param = 0;
    }
    else if (dma == &DMA2) {
        Ch2Mask &= ~(1 << channel); // mark it as disabled
        if (Ch2Mask == 0) { // last one? shut it down
            RCC.AHBENR &= ~(1 << 1);
        }
        Ch2Handlers[channel].isrfunc = 0;
        Ch2Handlers[channel].param = 0;
    }
}

void Dma::serveIsr(volatile DMA_t *dma, int ch, DmaHandler_t *handlers)
{
    uint32_t flags = (dma->ISR >> (ch * 4)) & 0xF;  // only bottom 4 bits are relevant to a single channel
    dma->IFCR = 1 << (ch * 4); // clear the channel
    DmaHandler_t &handler = handlers[ch];
    if (handler.isrfunc) {
        handler.isrfunc(handler.param, flags);
    }
}

/*
 * DMA1
 */
IRQ_HANDLER ISR_DMA1_Channel1()
{
    Dma::serveIsr(&DMA1, 0, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel2()
{
    Dma::serveIsr(&DMA1, 1, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel3()
{
    Dma::serveIsr(&DMA1, 2, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel4()
{
    Dma::serveIsr(&DMA1, 3, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel5()
{
    Dma::serveIsr(&DMA1, 4, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel6()
{
    Dma::serveIsr(&DMA1, 5, Dma::Ch1Handlers);
}

IRQ_HANDLER ISR_DMA1_Channel7()
{
    Dma::serveIsr(&DMA1, 6, Dma::Ch1Handlers);
}

/*
 * DMA2
 */

IRQ_HANDLER ISR_DMA2_Channel1()
{
    Dma::serveIsr(&DMA2, 0, Dma::Ch2Handlers);
}

IRQ_HANDLER ISR_DMA2_Channel2()
{
    Dma::serveIsr(&DMA2, 1, Dma::Ch2Handlers);
}

IRQ_HANDLER ISR_DMA2_Channel3()
{
    Dma::serveIsr(&DMA2, 2, Dma::Ch2Handlers);
}

IRQ_HANDLER ISR_DMA2_Channel4()
{
    Dma::serveIsr(&DMA2, 3, Dma::Ch2Handlers);
}

IRQ_HANDLER ISR_DMA2_Channel5() {
    Dma::serveIsr(&DMA2, 4, Dma::Ch2Handlers);
}

