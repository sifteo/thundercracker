
#include "hwtimer.h"

#define IS_ADVANCED(t) ((t) == &TIM1 || (t) == &TIM8)

void HwTimer::init(int period, int prescaler)
{
    if (tim == &TIM2) {
        RCC.APB1ENR |= (1 << 0); // TIM2 enable
        RCC.APB1RSTR = (1 << 0); // TIM2 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == &TIM3) {
        RCC.APB1ENR |= (1 << 1); // TIM3 enable
        RCC.APB1RSTR = (1 << 1); // TIM3 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == &TIM4) {
        RCC.APB1ENR |= (1 << 2); // TIM4 enable
        RCC.APB1RSTR = (1 << 2); // TIM4 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == &TIM5) {
        RCC.APB1ENR |= (1 << 3); // TIM5 enable
        RCC.APB1RSTR = (1 << 3); // TIM5 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == &TIM1) {
        RCC.APB2ENR |= (1 << 11); // TIM1 enable
        RCC.APB2RSTR = (1 << 11); // TIM1 reset
        RCC.APB2RSTR = 0;
    }

    // Timer configuration
    tim->PSC  = (uint16_t)prescaler;
    tim->ARR  = (uint16_t)period;

    if (IS_ADVANCED(tim)) {
        tim->BDTR = (1 << 15);  // MOE - main output enable
    }

    tim->CR2  = 0;
    tim->CCER = 0;
    tim->SR   = 0;              // clear status register
    tim->CR1  = (1 << 7) |      // ARPE - auto reload preload enable
                (1 << 2) |      // URS - update request source
                (1 << 0);       // EN - enable
}

void HwTimer::setUpdateIsrEnabled(bool enabled)
{
    if (enabled) {
        tim->DIER |= (1 << 0);
    }
    else {
        tim->DIER &= ~(1 << 0);
    }
}

void HwTimer::end()
{
    tim->CR1  = 0;  // control disabled
    tim->DIER = 0;  // IRQs disabled
    tim->SR   = 0;  // clear status

    if (tim == &TIM2) {
        RCC.APB1ENR &= ~(1 << 0); // TIM2 disable
    }
    else if (tim == &TIM3) {
        RCC.APB1ENR &= ~(1 << 1); // TIM3 disable
    }
    else if (tim == &TIM4) {
        RCC.APB1ENR &= ~(1 << 2); // TIM4 disable
    }
    else if (tim == &TIM5) {
        RCC.APB1ENR &= ~(1 << 3); // TIM5 disable
    }
    else if (tim == &TIM1) {
        RCC.APB2ENR &= ~(1 << 11); // TIM1 disable
    }
}

// channels are numbered 1-4
void HwTimer::configureChannel(int ch, Polarity polarity, TimerMode timmode, OutputMode outmode, DmaMode dmamode)
{
    uint8_t mode, pol;

    mode = (1 << 3) |       // OCxPE - output compare preload enable
           (timmode << 4);  // OCxM  - output compare mode
    if (ch <= 2) {
        tim->CCMR1 |= mode << ((ch - 1) * 8);
    }
    else {
        tim->CCMR2 |= mode << ((ch - 3) * 8);
    }

    pol = ((unsigned)polarity << 1);        // enable OCxE (and OCxP based on polarity)
    if (IS_ADVANCED(tim) && outmode == ComplementaryOutput) {
        pol |= 1 << 2;                      // enable OCxNE
    }
    tim->CCER |= (pol << ((ch - 1) * 4));
    tim->CCR[ch - 1] = 10;
#if 0
    if (dmamode == DmaEnabled) {
        volatile DMA_t *dma;
        volatile DMAChannel_t *dmachan;
        int dmaChannel = 0;

        switch (ch) {
        case 1:
            NVIC_SIFTEO.irqEnable(IVT.DMA1_Channel2);
            NVIC_SIFTEO.irqPrioritize(IVT.DMA1_Channel2, 0x81);
            break;
        case 2:
            NVIC_SIFTEO.irqEnable(IVT.DMA1_Channel3);
            NVIC_SIFTEO.irqPrioritize(IVT.DMA1_Channel3, 0x81);
            break;
        case 3:
            NVIC_SIFTEO.irqEnable(IVT.DMA1_Channel6);
            NVIC_SIFTEO.irqPrioritize(IVT.DMA1_Channel6, 0x81);
            break;
        case 4:
            NVIC_SIFTEO.irqEnable(IVT.DMA1_Channel4);
            NVIC_SIFTEO.irqPrioritize(IVT.DMA1_Channel4, 0x81);
            break;
        }


        getDmaDetails(ch, &dma, &dmaChannel);
        Dma::registerHandler(dma, dmaChannel, Pwm::staticDmaHandler, this);
        dmachan = &dma->channels[dmaChannel];
        // point DMA at the duty cycle
        dmachan->CPAR = (uint32_t)(&tim->CCR[0] + (ch - 1));
    }
#endif
    tim->DIER |= 1 << ch;
}

void HwTimer::enableChannel(int ch)
{
    tim->CCER |= 1 << ((ch - 1) * 4);
}

void HwTimer::disableChannel(int ch)
{
    tim->CCER &= ~(0x1 << ((ch - 1) * 4));
}

bool HwTimer::channelIsEnabled(int ch)
{
    return tim->CCER & (1 << ((ch - 1) * 4));
}

void HwTimer::setDuty(int ch, int duty)
{
    tim->CCR[ch - 1] = (uint16_t)duty;
}

int HwTimer::period() const
{
    return tim->ARR;
}

void HwTimer::setPeriod(int period, int prescaler)
{
    tim->ARR = (uint16_t)period;
    tim->PSC  = (uint16_t)prescaler;
}
