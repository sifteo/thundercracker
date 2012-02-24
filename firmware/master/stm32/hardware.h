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

extern volatile uint32_t DBGMCU_CR;

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

struct CRC_t {
  uint32_t DR;
  uint32_t IDR;
  uint32_t CR;
};

extern volatile CRC_t CRC;

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
 * USART / UART
 */

struct USART_t {
    uint32_t SR;
    uint32_t DR;
    uint32_t BRR;
    uint32_t CR1;
    uint32_t CR2;
    uint32_t CR3;
    uint32_t GTPR;
};

extern volatile USART_t USART1;
extern volatile USART_t USART2;
extern volatile USART_t USART3;
extern volatile USART_t UART4;
extern volatile USART_t UART5;

/*
 * Timers 1-5 & 8
 */
struct TIM_t {
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
    uint32_t RCR;       // only valid for TIM1 & TIM8
    uint32_t CCR[4];
    uint32_t BDTR;      // only valid for TIM1 & TIM8
    uint32_t DCR;
    uint32_t DMAR;
};

extern volatile TIM_t TIM1;
extern volatile TIM_t TIM2;
extern volatile TIM_t TIM3;
extern volatile TIM_t TIM4;
extern volatile TIM_t TIM5;
extern volatile TIM_t TIM8;


/*
 * Digital to Analog Converter
 */

struct DACChannel_t {
    uint32_t DHR12R;
    uint32_t DHR12L;
    uint32_t DHR8R1;
};

struct DAC_t {
    uint32_t CR;
    uint32_t SWTRIG;
    struct DACChannel_t channels[2];
    uint32_t DHR8R2;
    uint32_t DHR12R;
    uint32_t DHR12L;
    uint32_t DHR8RD;
    uint32_t DOR1;
    uint32_t DOR2;
};

extern volatile DAC_t DAC;


/*
  Analog to Digial Converter
*/

struct ADC_t {
    uint32_t SR;
    uint32_t CR1;
    uint32_t CR2;
    uint32_t SMPR1;
    uint32_t SMPR2;
    uint32_t JOFR1;
    uint32_t JOFR2;
    uint32_t JOFR3;
    uint32_t JOFR4;
    uint32_t HTR;
    uint32_t LTR;
    uint32_t SQR1;
    uint32_t SQR2;
    uint32_t SQR3;
    uint32_t JSQR;
    uint32_t JDR1;
    uint32_t JDR2;
    uint32_t JDR3;
    uint32_t JDR4;
    uint32_t DR;
};

extern volatile ADC_t ADC1;
extern volatile ADC_t ADC2;
extern volatile ADC_t ADC3;

/*
 * Direct memory access
 */

struct DMAChannel_t {
    uint32_t CCR;
    uint32_t CNDTR;
    uint32_t CPAR;
    uint32_t CMAR;
    uint32_t _reserved;
};

struct DMA_t {
    uint32_t ISR;
    uint32_t IFCR;
    struct DMAChannel_t channels[7]; // 7 for DMA1, 5 for DMA2
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


struct FLASH_t {
    uint32_t ACR;
    uint32_t KEYR;
    uint32_t OPTKEYR;
    uint32_t SR;
    uint32_t CR;
    uint32_t AR;
    uint32_t _reserved1;
    uint32_t OBR;
    uint32_t WRPR;
};

extern volatile FLASH_t FLASH;

/*
 * Universal Seral Bus - High/medium density devices
 */

struct USB_t {
    uint32_t endpoints[8];
    uint32_t reserved[8];
    uint32_t CNTR;
    uint32_t ISTR;
    uint32_t FNR;
    uint32_t DADDR;
    uint32_t BTABLE;
};

struct USBDescriptor_t {
    uint32_t     TXADDR0;
    uint16_t     TXCOUNT0;
    uint16_t     TXCOUNT1;
    uint32_t     RXADDR0;
    uint16_t     RXCOUNT0;
    uint16_t     RXCOUNT1;
};

extern volatile USB_t USB;

/*
 * USB on-the-go - connectivity line devices
 */

// global component
struct USBOTG_GLOBAL_t {
    uint32_t GOTGCTL;
    uint32_t GOTGINT;
    uint32_t GAHBCFG;
    uint32_t GUSBCFG;
    uint32_t GRSTCTL;
    uint32_t GINTSTS;
    uint32_t GINTMSK;
    uint32_t GRXSTSR;
    uint32_t GRXSTSP;
    uint32_t GRXFSIZ;
    uint32_t DIEPTXF0_HNPTXFSIZ;
    uint32_t HNPTXSTS;
    uint32_t reserved0x30;
    uint32_t reserved0x34;
    uint32_t GCCFG;
    uint32_t CID;
    uint32_t reserved0x40[48];
    uint32_t HPTXFSIZ;
    uint32_t DIEPTXF[3];
};

// host component
struct USBOTG_HOST_t {
    uint32_t HCFG;
    uint32_t HFIR;
    uint32_t HFNUM;
    uint32_t reserved0x40C;
    uint32_t HPTXSTS;
    uint32_t HAINT;
    uint32_t HAINTMSK;
    uint32_t reserved0x422[9];
    uint32_t HPRT;
};

// IN endpoints
struct USBOTG_IN_EP_t {
  uint32_t DIEPCTL;
  uint32_t reserved0x04;
  uint32_t DIEPINT;
  uint32_t reserved0xC;
  uint32_t DIEPTSIZ;
  uint32_t DIEPDMA;
  uint32_t DTXFSTS;
  uint32_t reserved0x18;
};

// OUT endpoints
struct USBOTG_OUT_EP_t {
  uint32_t DOEPCTL;
  uint32_t DOUTEPFRM;
  uint32_t DOEPINT;
  uint32_t reserved0xC;
  uint32_t DOEPTSIZ;
  uint32_t DOEPDMA;
  uint32_t reserved0x1C[2];
};

// host channel
struct USBOTG_HC_t {
  uint32_t HCCHAR;
  uint32_t HCSPLT;
  uint32_t HCINT;
  uint32_t HCGINTMSK;
  uint32_t HCTSIZ;
  uint32_t HCDMA;
  uint32_t reserved[2];
};

// device component
struct USBOTG_DEVICE_t {
    uint32_t DCFG;
    uint32_t DCTL;
    uint32_t DSTS;
    uint32_t reserved0x80C;
    uint32_t DIEPMSK;
    uint32_t DOEPMSK;
    uint32_t DAINT;
    uint32_t DAINTMSK;
    uint32_t reserved0x820;
    uint32_t reserved0x824;
    uint32_t DVBUSDIS;
    uint32_t DVBUSPULSE;
    uint32_t DTHRCTL;
    uint32_t DIEPEMPMSK;
    uint32_t DEACHINT;
    uint32_t DEACHMSK;
    uint32_t reserved0x840;
    uint32_t DINEP1MSK;
    uint32_t reserved0x844[15];
    uint32_t DOUTEP1MSK;
};

// TODO - make sure each segment is really at the appropriate offset
struct USBOTG_t {
    struct USBOTG_GLOBAL_t  global;             // offset 0x0
    struct USBOTG_HOST_t    host;               // offset 0x400
    struct USBOTG_HC_t      hostChannels[8];    // offset 0x500
    struct USBOTG_DEVICE_t  device;             // offset 0x800
    struct USBOTG_IN_EP_t   inEps[4];           // offset 0x900
    struct USBOTG_OUT_EP_t  outEps[4];          // offset 0xB00
    uint32_t PCGCCTL;                           // offset 0xE00
    // TODO - pad this out
    uint32_t DFIFO[4][0x1000];                 // offset 0x1000
};

extern volatile USBOTG_t USBOTG;

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

    void systemReset() volatile {
        appInterruptControl =   (0x5FA << 16) |                     // reset key
                                (appInterruptControl & (7 << 8)) |  // priority group unchanged
                                (1 << 2);                           // issue system reset
        __asm volatile ("dsb");                                     // paranoia - ensure memory access
        while (1);                                                  // wait to reset
    }
};

extern volatile NVIC_t NVIC;


#endif
