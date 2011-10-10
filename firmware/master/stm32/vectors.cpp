/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Vector table. Defined in C rather than C++, so that we
 * can use C99 struct initializers.
 */

#include "vectors.h"
#include "debug.h"

extern unsigned _stack;

IRQ_HANDLER ISR_Default() {
    Debug::log("Unexpected IRQ");
}

IRQ_HANDLER ISR_DefaultTrap() {
    /* XXX: Do something less power-hungry, like rebooting or halting. */
    while (1);
}

#pragma weak ISR_NMI = ISR_DefaultTrap
#pragma weak ISR_HardFault = ISR_DefaultTrap
#pragma weak ISR_MemManage = ISR_DefaultTrap
#pragma weak ISR_BusFault = ISR_DefaultTrap
#pragma weak ISR_UsageFault = ISR_DefaultTrap
#pragma weak ISR_SVCall = ISR_DefaultTrap
#pragma weak ISR_Debug = ISR_DefaultTrap
#pragma weak ISR_PendSV = ISR_Default
#pragma weak ISR_SysTick = ISR_Default
#pragma weak ISR_WWDG = ISR_Default
#pragma weak ISR_PVD = ISR_Default
#pragma weak ISR_Tamper = ISR_Default
#pragma weak ISR_RTC = ISR_Default
#pragma weak ISR_Flash = ISR_Default
#pragma weak ISR_RCC = ISR_Default
#pragma weak ISR_EXTI0 = ISR_Default
#pragma weak ISR_EXTI1 = ISR_Default
#pragma weak ISR_EXTI2 = ISR_Default
#pragma weak ISR_EXTI3 = ISR_Default
#pragma weak ISR_EXTI4 = ISR_Default
#pragma weak ISR_DMA1_Channel1 = ISR_Default
#pragma weak ISR_DMA1_Channel2 = ISR_Default
#pragma weak ISR_DMA1_Channel3 = ISR_Default
#pragma weak ISR_DMA1_Channel4 = ISR_Default
#pragma weak ISR_DMA1_Channel5 = ISR_Default
#pragma weak ISR_DMA1_Channel6 = ISR_Default
#pragma weak ISR_DMA1_Channel7 = ISR_Default
#pragma weak ISR_ADC1_2 = ISR_Default
#pragma weak ISR_USB_HP_CAN_TX = ISR_Default
#pragma weak ISR_USB_LP_CAN_RX0 = ISR_Default
#pragma weak ISR_CAN_RX1 = ISR_Default
#pragma weak ISR_CAN_SCE = ISR_Default
#pragma weak ISR_EXTI9_5 = ISR_Default
#pragma weak ISR_TIM1_BRK = ISR_Default
#pragma weak ISR_TIM1_UP = ISR_Default
#pragma weak ISR_TIM1_TRG_COM = ISR_Default
#pragma weak ISR_TIM1_CC = ISR_Default
#pragma weak ISR_TIM2 = ISR_Default
#pragma weak ISR_TIM3 = ISR_Default
#pragma weak ISR_TIM4 = ISR_Default
#pragma weak ISR_I2C1_EV = ISR_Default
#pragma weak ISR_I2C1_ER = ISR_Default
#pragma weak ISR_I2C2_EV = ISR_Default
#pragma weak ISR_I2C2_ER = ISR_Default
#pragma weak ISR_SPI1 = ISR_Default
#pragma weak ISR_SPI2 = ISR_Default
#pragma weak ISR_USART1 = ISR_Default
#pragma weak ISR_USART2 = ISR_Default
#pragma weak ISR_USART3 = ISR_Default
#pragma weak ISR_EXTI15_10 = ISR_Default
#pragma weak ISR_RTCAlarm = ISR_Default
#pragma weak ISR_USBWakeup = ISR_Default
#pragma weak ISR_TIM8_BRK = ISR_Default
#pragma weak ISR_TIM8_UP = ISR_Default
#pragma weak ISR_TIM8_TRG_COM = ISR_Default
#pragma weak ISR_TIM8_CC = ISR_Default
#pragma weak ISR_ADC3 = ISR_Default
#pragma weak ISR_FSMC = ISR_Default
#pragma weak ISR_SDIO = ISR_Default
#pragma weak ISR_TIM5 = ISR_Default
#pragma weak ISR_SPI3 = ISR_Default
#pragma weak ISR_UART4 = ISR_Default
#pragma weak ISR_UART5 = ISR_Default
#pragma weak ISR_TIM6 = ISR_Default
#pragma weak ISR_TIM7 = ISR_Default
#pragma weak ISR_DMA2_Channel1 = ISR_Default
#pragma weak ISR_DMA2_Channel2 = ISR_Default
#pragma weak ISR_DMA2_Channel3 = ISR_Default
#pragma weak ISR_DMA2_Channel4_5 = ISR_Default

__attribute__ ((section (".vectors"))) const struct IVT_t IVT =
{
    &_stack,
    _start,
    ISR_NMI,
    ISR_HardFault,
    ISR_MemManage,
    ISR_BusFault,
    ISR_UsageFault,
    { 0 },
    ISR_SVCall,
    ISR_Debug,
    0,
    ISR_PendSV,
    ISR_SysTick,
    ISR_WWDG,
    ISR_PVD,
    ISR_Tamper,
    ISR_RTC,
    ISR_Flash,
    ISR_RCC,
    ISR_EXTI0,
    ISR_EXTI1,
    ISR_EXTI2,
    ISR_EXTI3,
    ISR_EXTI4,
    ISR_DMA1_Channel1,
    ISR_DMA1_Channel2,
    ISR_DMA1_Channel3,
    ISR_DMA1_Channel4,
    ISR_DMA1_Channel5,
    ISR_DMA1_Channel6,
    ISR_DMA1_Channel7,
    ISR_ADC1_2,
    ISR_USB_HP_CAN_TX,
    ISR_USB_LP_CAN_RX0,
    ISR_CAN_RX1,
    ISR_CAN_SCE,
    ISR_EXTI9_5,
    ISR_TIM1_BRK,
    ISR_TIM1_UP,
    ISR_TIM1_TRG_COM,
    ISR_TIM1_CC,
    ISR_TIM2,
    ISR_TIM3,
    ISR_TIM4,
    ISR_I2C1_EV,
    ISR_I2C1_ER,
    ISR_I2C2_EV,
    ISR_I2C2_ER,
    ISR_SPI1,
    ISR_SPI2,
    ISR_USART1,
    ISR_USART2,
    ISR_USART3,
    ISR_EXTI15_10,
    ISR_RTCAlarm,
    ISR_USBWakeup,
    ISR_TIM8_BRK,
    ISR_TIM8_UP,
    ISR_TIM8_TRG_COM,
    ISR_TIM8_CC,
    ISR_ADC3,
    ISR_FSMC,
    ISR_SDIO,
    ISR_TIM5,
    ISR_SPI3,
    ISR_UART4,
    ISR_UART5,
    ISR_TIM6,
    ISR_TIM7,
    ISR_DMA2_Channel1,
    ISR_DMA2_Channel2,
    ISR_DMA2_Channel3,
    ISR_DMA2_Channel4_5,
};

