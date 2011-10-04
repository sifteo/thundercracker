/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_VECTORS_H
#define _STM32_VECTORS_H

#include <stdint.h>

/*
 * Functions referenced in the vector table.
 */

#define IRQ_HANDLER     extern "C" void __attribute__ ((interrupt ("IRQ")))

extern "C" void _start();

IRQ_HANDLER ISR_NMI();
IRQ_HANDLER ISR_HardFault();
IRQ_HANDLER ISR_MemManage();
IRQ_HANDLER ISR_BusFault();
IRQ_HANDLER ISR_UsageFault();
IRQ_HANDLER ISR_SVCall();
IRQ_HANDLER ISR_Debug();
IRQ_HANDLER ISR_PendSV();
IRQ_HANDLER ISR_SysTick();
IRQ_HANDLER ISR_WWDG();
IRQ_HANDLER ISR_PVD();
IRQ_HANDLER ISR_Tamper();
IRQ_HANDLER ISR_RTC();
IRQ_HANDLER ISR_Flash();
IRQ_HANDLER ISR_RCC();
IRQ_HANDLER ISR_EXTI0();
IRQ_HANDLER ISR_EXTI1();
IRQ_HANDLER ISR_EXTI2();
IRQ_HANDLER ISR_EXTI3();
IRQ_HANDLER ISR_EXTI4();
IRQ_HANDLER ISR_DMA1_Channel1();
IRQ_HANDLER ISR_DMA1_Channel2();
IRQ_HANDLER ISR_DMA1_Channel3();
IRQ_HANDLER ISR_DMA1_Channel4();
IRQ_HANDLER ISR_DMA1_Channel5();
IRQ_HANDLER ISR_DMA1_Channel6();
IRQ_HANDLER ISR_DMA1_Channel7();
IRQ_HANDLER ISR_ADC1_2();
IRQ_HANDLER ISR_USB_HP_CAN_TX();
IRQ_HANDLER ISR_USB_LP_CAN_RX0();
IRQ_HANDLER ISR_CAN_RX1();
IRQ_HANDLER ISR_CAN_SCE();
IRQ_HANDLER ISR_EXTI9_5();
IRQ_HANDLER ISR_TIM1_BRK();
IRQ_HANDLER ISR_TIM1_UP();
IRQ_HANDLER ISR_TIM1_TRG_COM();
IRQ_HANDLER ISR_TIM1_CC();
IRQ_HANDLER ISR_TIM2();
IRQ_HANDLER ISR_TIM3();
IRQ_HANDLER ISR_TIM4();
IRQ_HANDLER ISR_I2C1_EV();
IRQ_HANDLER ISR_I2C1_ER();
IRQ_HANDLER ISR_I2C2_EV();
IRQ_HANDLER ISR_I2C2_ER();
IRQ_HANDLER ISR_SPI1();
IRQ_HANDLER ISR_SPI2();
IRQ_HANDLER ISR_USART1();
IRQ_HANDLER ISR_USART2();
IRQ_HANDLER ISR_USART3();
IRQ_HANDLER ISR_EXTI15_10();
IRQ_HANDLER ISR_RTCAlarm();
IRQ_HANDLER ISR_USBWakeup();
IRQ_HANDLER ISR_TIM8_BRK();
IRQ_HANDLER ISR_TIM8_UP();
IRQ_HANDLER ISR_TIM8_TRG_COM();
IRQ_HANDLER ISR_TIM8_CC();
IRQ_HANDLER ISR_ADC3();
IRQ_HANDLER ISR_FSMC();
IRQ_HANDLER ISR_SDIO();
IRQ_HANDLER ISR_TIM5();
IRQ_HANDLER ISR_SPI3();
IRQ_HANDLER ISR_UART4();
IRQ_HANDLER ISR_UART5();
IRQ_HANDLER ISR_TIM6();
IRQ_HANDLER ISR_TIM7();
IRQ_HANDLER ISR_DMA2_Channel1();
IRQ_HANDLER ISR_DMA2_Channel2();
IRQ_HANDLER ISR_DMA2_Channel3();
IRQ_HANDLER ISR_DMA2_Channel4_5();

/*
 * Interrupt vector layout
 */

typedef void (*ISR_t)();

struct IVT_t {
    void *stack;
    ISR_t Reset;
    ISR_t NMI;
    ISR_t HardFault;
    ISR_t MemManage;
    ISR_t BusFault;
    ISR_t UsageFault;
    uintptr_t _res0[4];
    ISR_t SVCall;
    ISR_t Debug;
    uintptr_t _res1;
    ISR_t PendSV;
    ISR_t SysTick;
    ISR_t WWDG;
    ISR_t PVD;
    ISR_t Tamper;
    ISR_t RTC;
    ISR_t Flash;
    ISR_t RCC;
    ISR_t EXTI0;
    ISR_t EXTI1;
    ISR_t EXTI2;
    ISR_t EXTI3;
    ISR_t EXTI4;
    ISR_t DMA1_Channel1;
    ISR_t DMA1_Channel2;
    ISR_t DMA1_Channel3;
    ISR_t DMA1_Channel4;
    ISR_t DMA1_Channel5;
    ISR_t DMA1_Channel6;
    ISR_t DMA1_Channel7;
    ISR_t ADC1_2;
    ISR_t USB_HP_CAN_TX;
    ISR_t USB_LP_CAN_RX0;
    ISR_t CAN_RX1;
    ISR_t CAN_SCE;
    ISR_t EXTI9_5;
    ISR_t TIM1_BRK;
    ISR_t TIM1_UP;
    ISR_t TIM1_TRG_COM;
    ISR_t TIM1_CC;
    ISR_t TIM2;
    ISR_t TIM3;
    ISR_t TIM4;
    ISR_t I2C1_EV;
    ISR_t I2C1_ER;
    ISR_t I2C2_EV;
    ISR_t I2C2_ER;
    ISR_t SPI1;
    ISR_t SPI2;
    ISR_t USART1;
    ISR_t USART2;
    ISR_t USART3;
    ISR_t EXTI15_10;
    ISR_t RTCAlarm;
    ISR_t USBWakeup;
    ISR_t TIM8_BRK;
    ISR_t TIM8_UP;
    ISR_t TIM8_TRG_COM;
    ISR_t TIM8_CC;
    ISR_t ADC3;
    ISR_t FSMC;
    ISR_t SDIO;
    ISR_t TIM5;
    ISR_t SPI3;
    ISR_t UART4;
    ISR_t UART5;
    ISR_t TIM6;
    ISR_t TIM7;
    ISR_t DMA2_Channel1;
    ISR_t DMA2_Channel2;
    ISR_t DMA2_Channel3;
    ISR_t DMA2_Channel4_5;
};

extern const struct IVT_t IVT;


#endif // _STM32_VECTORS_H
