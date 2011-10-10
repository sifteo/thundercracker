
#include "pwm.h"
#include "hardware.h"

Pwm testTmr(TIM5, GPIOPin(&GPIOA, 1));

Pwm::Pwm(volatile TIM2_5_t *_hw, GPIOPin _output) :
    tim(_hw), output(_output)
{
}

void Pwm::init(int freq, int period)
{
    if (tim == TIM2) {
        RCC.APB1ENR |= (1 << 0); // TIM2 enable
        RCC.APB1RSTR = (1 << 0); // TIM2 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == TIM3) {
        RCC.APB1ENR |= (1 << 1); // TIM3 enable
        RCC.APB1RSTR = (1 << 1); // TIM3 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == TIM4) {
        RCC.APB1ENR |= (1 << 2); // TIM4 enable
        RCC.APB1RSTR = (1 << 2); // TIM4 reset
        RCC.APB1RSTR = 0;
    }
    else if (tim == TIM5) {
        RCC.APB1ENR |= (1 << 3); // TIM5 enable
        RCC.APB1RSTR = (1 << 3); // TIM5 reset
        RCC.APB1RSTR = 0;
    }

    output.setControl(GPIOPin::OUT_ALT_50MHZ);

    // Timer configuration
    tim->PSC  = 0; //(uint16_t)(clock / freq) - 1; // TODO - compute frequency based on sys clk
    tim->ARR  = (uint16_t)period;

    tim->CR2 = 0;
    tim->CCER = 0;

    tim->EGR  = (1 << 0);       // UG - update generation
    tim->DIER = 0; //(1 << 0);       // UIE - update interrupt enable
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

    if (tim == TIM2) {
        RCC.APB1ENR &= ~(1 << 0); // TIM2 disable
    }
    else if (tim == TIM3) {
        RCC.APB1ENR &= ~(1 << 1); // TIM3 disable
    }
    else if (tim == TIM4) {
        RCC.APB1ENR &= ~(1 << 2); // TIM4 disable
    }
    else if (tim == TIM5) {
        RCC.APB1ENR &= ~(1 << 3); // TIM5 disable
    }
}

// channels are numbered 1-4
// duty is in ticks
void Pwm::enableChannel(int ch, enum Polarity p, int duty)
{
    uint8_t mode, pol;

    mode = (1 << 3) |   // OCxPE - output compare preload enable
           (6 << 4);    // OCxM  - output compare mode, PWM mode 1
    if (ch <= 2) {
        tim->CCMR1 |= mode << ((ch - 1) * 8);
    }
    else if (ch <= 4) {
        tim->CCMR2 |= mode << ((ch - 3) * 8);
    }

    setDuty(ch, duty);

    pol = ((unsigned)p << 1) | 0x1;
    tim->CCER |= (pol << ((ch - 1) * 4));
    tim->EGR = 1;
}

void Pwm::setDuty(int ch, int duty)
{
    tim->CCR[ch - 1] = (uint16_t)duty;
}

void Pwm::disableChannel(int ch)
{
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
