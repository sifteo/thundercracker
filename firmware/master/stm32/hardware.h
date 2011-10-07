/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_HARDWARE_H
#define _STM32_HARDWARE_H

#include <stdint.h>
#include "vectors.h"


/*
 * Reset / Clock Control
 */

struct RCC_t {
    uint32_t CR;
    uint32_t CFGR;
    uint32_t CIR;
    uint32_t APB2RSTR;
    uint32_t APB1RSTR;
    uint32_t AHBENR;
    uint32_t APB2ENR;
    uint32_t APB1ENR;
    uint32_t BDCR;
    uint32_t CSR;
    uint32_t AHBSTR;
    uint32_t CFGR2;
};

extern volatile RCC_t RCC;


/*
 * GPIOs.
 */

struct GPIO_t {
    uint32_t CRL;
    uint32_t CRH;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t BRR;
    uint32_t LCRK;
};

struct AFIO_t {
    uint32_t EVCR;
    uint32_t MAPR;
    uint32_t EXTICR[4];
};

extern volatile GPIO_t GPIOA;
extern volatile GPIO_t GPIOB;
extern volatile GPIO_t GPIOC;
extern volatile GPIO_t GPIOD;
extern volatile GPIO_t GPIOE;
extern volatile GPIO_t GPIOF;
extern volatile GPIO_t GPIOG;

extern volatile AFIO_t AFIO;

/*
 * SPI / I2S
 */

struct SPI_t {
    uint32_t CR1;
    uint32_t CR2;
    uint32_t SR;
    uint32_t DR;
    uint32_t CRCPR;
    uint32_t RXCRCR;
    uint32_t TXCRCR;
    uint32_t I2SCFGR;
    uint32_t I2SPR;
};

extern volatile SPI_t SPI1;
extern volatile SPI_t SPI2;
extern volatile SPI_t SPI3;

/*
 * Timers 2-5
 */

struct TIM2_5_t {
    uint32_t CR1;
    uint32_t CR2;
    uint32_t SMCR;
    uint32_t DIER;
    uint32_t SR;
    uint32_t EGR;
    uint32_t CCMR1;
    uint32_t CCMR2;
    uint32_t CCER;
    uint32_t CNT;
    uint32_t PSC;
    uint32_t ARR;
    uint32_t _res1;
    uint32_t CCR[4];
    uint32_t _res2;
    uint32_t DCR;
    uint32_t DMAR;
};


// NOTE - using this style def now (as opposed to referencing the symbol in the
//          linker script, so they can be used in conditionals
#define TIM2 ((volatile TIM2_5_t*)0x40000000)
#define TIM3 ((volatile TIM2_5_t*)0x40000400)
#define TIM4 ((volatile TIM2_5_t*)0x40000800)
#define TIM5 ((volatile TIM2_5_t*)0x40000c00)


/*
 * Direct memory access
 */

struct DMA_t {
    uint32_t ISR;
    uint32_t IFCR;
    uint32_t CCR1;
    uint32_t CNDTR1;
    uint32_t CPAR1;
    uint32_t CMAR1;
    uint32_t _reserved1;
    uint32_t CCR2;
    uint32_t CNDTR2;
    uint32_t CPAR2;
    uint32_t CMAR2;
    uint32_t _reserved2;
    uint32_t CCR3;
    uint32_t CNDTR3;
    uint32_t CPAR3;
    uint32_t CMAR3;
    uint32_t _reserved3;
    uint32_t CCR4;
    uint32_t CNDTR4;
    uint32_t CPAR4;
    uint32_t CMAR4;
    uint32_t _reserved4;
    uint32_t CCR5;
    uint32_t CNDTR5;
    uint32_t CPAR5;
    uint32_t CMAR5;
    uint32_t _reserved5;
    uint32_t CCR6;
    uint32_t CNDTR6;
    uint32_t CPAR6;
    uint32_t CMAR6;
    uint32_t _reserved6;
    uint32_t CCR7;
    uint32_t CNDTR7;
    uint32_t CPAR7;
    uint32_t CMAR7;
//    uint32_t _reserved7;
};

extern volatile DMA_t DMA1;
extern volatile DMA_t DMA2;

/*
 * External interrupt controller
 */

struct EXTI_t {
    uint32_t IMR;
    uint32_t EMR;
    uint32_t RTSR;
    uint32_t FTSR;
    uint32_t SWIER;
    uint32_t PR;
};

extern volatile EXTI_t EXTI;


/*
 * Cortex-M3 Nested Vectored Interrupt Controller
 */

struct NVIC_t {
    uint32_t _res0;
    uint32_t ICT;
    uint32_t _res1[2];
    uint32_t SysTick_CS;
    uint32_t SysTick_RELOAD;
    uint32_t SysTick;
    uint32_t SysTick_CAL;
    uint32_t _res2[56];

    uint32_t irqSetEnable[32];
    uint32_t irqClearEnable[32];
    uint32_t irqSetPending[32];
    uint32_t irqClearPending[32];
    uint32_t irqActive[64];
    uint32_t irqPriority[64];
    uint32_t _res3[512];

    uint32_t CPUID;
    uint32_t interruptControlState;
    uint32_t vectorTableOffset;
    uint32_t appInterruptControl;
    uint32_t sysControl;
    uint32_t configControl;
    uint32_t sysHandlerPriority[3];
    uint32_t sysHandlerControl;
    uint32_t cfgFaultStatus;
    uint32_t hardFaultStatus;
    uint32_t debugFaultStatus;
    uint32_t memManageAddress;
    uint32_t busFaultAddress;
    uint32_t auxFaultStatus;
    uint32_t PFR0;
    uint32_t PFR1;
    uint32_t DFR0;
    uint32_t AFR0;
    uint32_t MMFR[4];
    uint32_t ISAR[5];
    uint32_t _res4[33];
    uint16_t DCRDR_l;
    uint16_t DCRDR_h;
    uint32_t _res5[65];

    uint32_t softIrqTrigger;
    uint32_t _res6[51];

    uint32_t PID4;
    uint32_t PID5;
    uint32_t PID6;
    uint32_t PID7;
    uint32_t PID0;
    uint32_t PID1;
    uint32_t PID2;
    uint32_t PID3;
    uint32_t CID0;
    uint32_t CID1;
    uint32_t CID2;
    uint32_t CID3;

    void irqEnable(const ISR_t &vector) volatile {
        unsigned id = ((uintptr_t)&vector - (uintptr_t)&IVT.WWDG) >> 2;
        irqSetEnable[id >> 5] = 1 << (id & 31);
    }

    void irqDisable(const ISR_t &vector) volatile {
        unsigned id = ((uintptr_t)&vector - (uintptr_t)&IVT.WWDG) >> 2;
        irqClearEnable[id >> 5] = 1 << (id & 31);
    }

    void irqPrioritize(const ISR_t &vector, uint8_t prio) volatile {
        unsigned id = ((uintptr_t)&vector - (uintptr_t)&IVT.WWDG) >> 2;
        irqPriority[id >> 2] |= prio << ((id & 3) << 3);
    }
    
    void sysHandlerPrioritize(const ISR_t &vector, uint8_t prio) volatile {
        unsigned id = ((uintptr_t)&vector - (uintptr_t)&IVT.MemManage) >> 2;
        sysHandlerPriority[id >> 2] |= prio << ((id & 3) << 3);
    }
};

extern volatile NVIC_t NVIC;


#endif
