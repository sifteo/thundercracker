/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_VECTORS_H
#define _STM32_VECTORS_H

#include <stdint.h>

/*
 * Functions referenced in the vector table.
 */

#define IRQ_HANDLER     extern "C" void __attribute__ ((interrupt ("IRQ")))
#define NAKED_HANDLER   extern "C" void __attribute__ ((naked))

extern "C" void _start();

IRQ_HANDLER ISR_NMI();
IRQ_HANDLER ISR_HardFault();
IRQ_HANDLER ISR_MemManage();
IRQ_HANDLER ISR_BusFault();
IRQ_HANDLER ISR_UsageFault();
NAKED_HANDLER ISR_SVCall();
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
IRQ_HANDLER ISR_CAN1_TX();
IRQ_HANDLER ISR_CAN1_RX0();
IRQ_HANDLER ISR_CAN1_RX1();
IRQ_HANDLER ISR_CAN1_SCE();
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
IRQ_HANDLER ISR_UsbOtg_FS_Wakeup();
IRQ_HANDLER ISR_TIM5();
IRQ_HANDLER ISR_SPI3();
IRQ_HANDLER ISR_UART4();
IRQ_HANDLER ISR_UART5();
IRQ_HANDLER ISR_TIM6();
IRQ_HANDLER ISR_TIM7();
IRQ_HANDLER ISR_DMA2_Channel1();
IRQ_HANDLER ISR_DMA2_Channel2();
IRQ_HANDLER ISR_DMA2_Channel3();
IRQ_HANDLER ISR_DMA2_Channel4();
IRQ_HANDLER ISR_DMA2_Channel5();
IRQ_HANDLER ISR_Ethernet();
IRQ_HANDLER ISR_EthernetWakeup();
IRQ_HANDLER ISR_CAN2_TX();
IRQ_HANDLER ISR_CAN2_RX0();
IRQ_HANDLER ISR_CAN2_RX1();
IRQ_HANDLER ISR_CAN2_SCE();
IRQ_HANDLER ISR_UsbOtg_FS();

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
    ISR_t CAN1_TX;
    ISR_t CAN1_RX0;
    ISR_t CAN1_RX1;
    ISR_t CAN1_SCE;
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
    ISR_t UsbOtg_FS_Wakeup;
    uintptr_t _res2[7];
    ISR_t TIM5;
    ISR_t SPI3;
    ISR_t UART4;
    ISR_t UART5;
    ISR_t TIM6;
    ISR_t TIM7;
    ISR_t DMA2_Channel1;
    ISR_t DMA2_Channel2;
    ISR_t DMA2_Channel3;
    ISR_t DMA2_Channel4;
    ISR_t DMA2_Channel5;
    ISR_t Ethernet;
    ISR_t EthernetWakeup;
    ISR_t CAN2_TX;
    ISR_t CAN2_RX0;
    ISR_t CAN2_RX1;
    ISR_t CAN2_SCE;
    ISR_t UsbOtg_FS;
};

extern const struct IVT_t IVT;


#endif // _STM32_VECTORS_H
