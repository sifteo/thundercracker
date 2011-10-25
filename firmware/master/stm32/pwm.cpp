
#include "pwm.h"
#include "hardware.h"
#include "dma.h"


#define IS_ADVANCED(t) ((t) == &TIM1 || (t) == &TIM8)

void Pwm::init(int period, int prescaler)
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

        NVIC.irqEnable(IVT.DMA1_Channel3);
        NVIC.irqPrioritize(IVT.DMA1_Channel3, 0x81);
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

        NVIC.irqEnable(IVT.DMA2_Channel4_5);
        NVIC.irqPrioritize(IVT.DMA2_Channel4_5, 0x81);
    }
    else if (tim == &TIM1) {
        RCC.APB2ENR |= (1 << 11); // TIM1 enable
        RCC.APB2RSTR = (1 << 11); // TIM1 reset
        RCC.APB2RSTR = 0;
    }

    output.setControl(GPIOPin::OUT_ALT_50MHZ);
    complementaryOutput.setControl(GPIOPin::OUT_ALT_50MHZ);

    // Timer configuration
    tim->PSC  = (uint16_t)prescaler;
    tim->ARR  = (uint16_t)period;
    tim->EGR = (1 << 0);

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

void Pwm::end()
{
    tim->CR1  = 0;  // control disabled
    tim->DIER = 0;  // IRQs disabled
    tim->SR   = 0;  // clear status

    if (tim == &TIM2) {
        RCC.APB1ENR &= ~(1 << 0); // TIM2 disable
    }
    else if (tim == &TIM3) {
        RCC.APB1ENR &= ~(1 << 1); // TIM3 disable
        NVIC.irqDisable(IVT.DMA1_Channel3);
    }
    else if (tim == &TIM4) {
        RCC.APB1ENR &= ~(1 << 2); // TIM4 disable
    }
    else if (tim == &TIM5) {
        RCC.APB1ENR &= ~(1 << 3); // TIM5 disable
        NVIC.irqDisable(IVT.DMA2_Channel4_5);
    }
    else if (tim == &TIM1) {
        RCC.APB2ENR &= ~(1 << 11); // TIM1 disable
    }
}

// channels are numbered 1-4
void Pwm::enableChannel(int ch, enum Polarity polarity, enum OutputMode outmode, enum DmaMode dmamode)
{
    uint8_t mode, pol;

//    tim->CCR[ch - 1] = 10;

    mode = (1 << 3) |   // OCxPE - output compare preload enable
           (6 << 4);    // OCxM  - output compare mode, PWM mode 1
    if (ch <= 2) {
        tim->CCMR1 |= mode << ((ch - 1) * 8);
    }
    else {
        tim->CCMR2 |= mode << ((ch - 3) * 8);
    }

    pol = ((unsigned)polarity << 1) | 0x1;      // enable OCxE (and OCxP based on polarity)
    if (IS_ADVANCED(tim) && outmode == ComplementaryOutput) {
        pol |= 1 << 2;                          // enable OCxNE
    }
    tim->CCER |= (pol << ((ch - 1) * 4));
    tim->CCR[ch - 1] = 10;

    if (dmamode == DmaEnabled) {
        volatile DMA_t *dma;
        volatile DMAChannel_t *dmachan;
        int dmaChannel = 0;

        switch (ch) {
        case 1:
            NVIC.irqEnable(IVT.DMA1_Channel2);
            NVIC.irqPrioritize(IVT.DMA1_Channel2, 0x81);
            break;
        case 2:
            NVIC.irqEnable(IVT.DMA1_Channel3);
            NVIC.irqPrioritize(IVT.DMA1_Channel3, 0x81);
            break;
        case 3:
            NVIC.irqEnable(IVT.DMA1_Channel6);
            NVIC.irqPrioritize(IVT.DMA1_Channel6, 0x81);
            break;
        case 4:
            NVIC.irqEnable(IVT.DMA1_Channel4);
            NVIC.irqPrioritize(IVT.DMA1_Channel4, 0x81);
            break;
        }


        getDmaDetails(ch, &dma, &dmaChannel);
        Dma::registerHandler(dma, dmaChannel, Pwm::staticDmaHandler, this);
        dmachan = &dma->channels[dmaChannel];
        // point DMA at the duty cycle
        dmachan->CPAR = (uint32_t)(&tim->CCR[0] + (ch - 1));
    }
    tim->DIER |= 1 << ch;
}

void Pwm::staticDmaHandler(void *p, uint32_t flags)
{
    Pwm *pwm = static_cast<Pwm*>(p);
    pwm->dmaHandler(flags);
}

void Pwm::dmaHandler(uint32_t flags)
{
    if (flags & (1 << 3)) {
        // transfer error
    }
    if (flags & (1 << 1)) {
        // transfer complete
    }
}

void Pwm::setDuty(int ch, int duty)
{
//    tim->EGR = (1 << 0); // resets counter and updates the registers
    tim->CCR[ch - 1] = (uint16_t)duty;
}

// set duty via a buffer of data
// TODO - more control over this - loop, completion handlers, etc
void Pwm::setDutyDma(int ch, const uint16_t *data, uint16_t len)
{
    volatile DMA_t *dma;
    int dmaChannel;
    volatile struct DMAChannel_t *dmachan;

    getDmaDetails(ch, &dma, &dmaChannel);

    dmachan = &dma->channels[dmaChannel];
    dmachan->CCR &= ~(1 << 0);      // must be disabled to reconfigure
    dmachan->CCR =  (0 << 14) |     // mem to mem disabled
                    (2 << 12) |     // high priority
                    (1 << 10) |     // 16-bit memory size
                    (1 << 8)  |     // 16-bit peripheral size
                    (1 << 7)  |     // memory increment enabled
                    (0 << 6)  |     // peripheral increment disable
                    (1 << 5)  |     // circular mode enabled
                    (1 << 4)  |     // direction: memory -> peripheral
                    (1 << 3)  |     // transfer error interrupt enable
                    (0 << 2)  |     // half transfer interrupt enable
                    (1 << 1);       // transfer complete interrupt enable
    dmachan->CNDTR = len;
    dmachan->CMAR = (uint32_t)data;
    dmachan->CCR |= (1 << 0);       // enable

    tim->DIER |= (1 << 8);          // fire off the DMA request
}

void Pwm::disableChannel(int ch)
{
    tim->CCER &= ~(0x1 << ((ch - 1) * 4));
    tim->CCR[ch - 1] = 0;
}

int Pwm::period() const
{
	return tim->ARR;
}

void Pwm::setPeriod(int period)
{
    tim->ARR = (uint16_t)period;
}

// a bit gross - store this as a const table somewhere?
bool Pwm::getDmaDetails(int pwmchan, volatile DMA_t **dma, int *dmaChannel)
{
    if (tim == &TIM5) {
        *dma = &DMA2;
        switch (pwmchan) {
        // DMA channels are 1-based
        case 1: *dmaChannel = (5 - 1); break;
        case 2: *dmaChannel = (4 - 1); break;
        case 3: *dmaChannel = (2 - 1); break;
        case 4: *dmaChannel = (1 - 1); break;
        }
        return true;
    }
    else if (tim == &TIM3) {
        *dma = &DMA1;
        switch (pwmchan) {
        // DMA channels are 1-based
        case 1: *dmaChannel = (3 - 1); break;
        // no DMA support for ch2
        case 3: *dmaChannel = (2 - 1); break;
        case 4: *dmaChannel = (3 - 1); break;
        }
        return true;
    }
    else if (tim == &TIM1) {
        *dma = &DMA1;
        switch (pwmchan) {
        // DMA channels are 1-based
        case 1: *dmaChannel = (2 - 1); break;
        case 2: *dmaChannel = (3 - 1); break;
        case 3: *dmaChannel = (6 - 1); break;
        case 4: *dmaChannel = (4 - 1); break;
        }
        return true;
    }
    // TODO - support other timers
    return false;
}
