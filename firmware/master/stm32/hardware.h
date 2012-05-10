/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    uint32_t AHBRSTR;
    uint32_t CFGR2;
};

extern volatile RCC_t RCC;


/*
 * Power Control
 */

struct PWR_t {
    uint32_t CR;
    uint32_t CSR;
};

extern volatile PWR_t PWR;

/*
 * Backup Domain
 */

struct BKP_t {
    uint32_t  _res0;
    uint16_t DR1;
    uint16_t  _res1;
    uint16_t DR2;
    uint16_t  _res2;
    uint16_t DR3;
    uint16_t  _res3;
    uint16_t DR4;
    uint16_t  _res4;
    uint16_t DR5;
    uint16_t  _res5;
    uint16_t DR6;
    uint16_t  _res6;
    uint16_t DR7;
    uint16_t  _res7;
    uint16_t DR8;
    uint16_t  _res8;
    uint16_t DR9;
    uint16_t  _res9;
    uint16_t DR10;
    uint16_t  _res10;
    uint16_t RTCCR;
    uint16_t  _res11;
    uint16_t CR;
    uint16_t  _res12;
    uint16_t CSR;
    uint16_t  _res13[5];
    uint16_t DR11;
    uint16_t  _res14;
    uint16_t DR12;
    uint16_t  _res15;
    uint16_t DR13;
    uint16_t  _res16;
    uint16_t DR14;
    uint16_t  _res17;
    uint16_t DR15;
    uint16_t  _res18;
    uint16_t DR16;
    uint16_t  _res19;
    uint16_t DR17;
    uint16_t  _res20;
    uint16_t DR18;
    uint16_t  _res21;
    uint16_t DR19;
    uint16_t  _res22;
    uint16_t DR20;
    uint16_t  _res23;
    uint16_t DR21;
    uint16_t  _res24;
    uint16_t DR22;
    uint16_t  _res25;
    uint16_t DR23;
    uint16_t  _res26;
    uint16_t DR24;
    uint16_t  _res27;
    uint16_t DR25;
    uint16_t  _res28;
    uint16_t DR26;
    uint16_t  _res29;
    uint16_t DR27;
    uint16_t  _res30;
    uint16_t DR28;
    uint16_t  _res31;
    uint16_t DR29;
    uint16_t  _res32;
    uint16_t DR30;
    uint16_t  _res33;
    uint16_t DR31;
    uint16_t  _res34;
    uint16_t DR32;
    uint16_t  _res35;
    uint16_t DR33;
    uint16_t  _res36;
    uint16_t DR34;
    uint16_t  _res37;
    uint16_t DR35;
    uint16_t  _res38;
    uint16_t DR36;
    uint16_t  _res39;
    uint16_t DR37;
    uint16_t  _res40;
    uint16_t DR38;
    uint16_t  _res41;
    uint16_t DR39;
    uint16_t  _res42;
    uint16_t DR40;
    uint16_t  _res43;
    uint16_t DR41;
    uint16_t  _res44;
    uint16_t DR42;
    uint16_t  _res45;
};

extern volatile BKP_t BKP;

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
    uint16_t CR1;
    uint16_t _res0;
    uint16_t CR2;
    uint16_t _res1;
    uint16_t SR;
    uint16_t _res2;
    uint16_t DR;
    uint16_t _res3;
    uint16_t CRCPR;
    uint16_t _res4;
    uint16_t RXCRCR;
    uint16_t _res5;
    uint16_t TXCRCR;
    uint16_t _res6;
    uint16_t I2SCFGR;
    uint16_t _res7;
    uint16_t I2SPR;
    uint16_t _res8;
};

extern volatile SPI_t SPI1;
extern volatile SPI_t SPI2;
extern volatile SPI_t SPI3;

/*
 * I2C
 */

struct I2C_t {
    uint16_t CR1;
    uint16_t _res0;
    uint16_t CR2;
    uint16_t _res1;
    uint16_t OAR1;
    uint16_t _res2;
    uint16_t OAR2;
    uint16_t _res3;
    uint16_t DR;
    uint16_t _res4;
    uint16_t SR1;
    uint16_t _res5;
    uint16_t SR2;
    uint16_t _res6;
    uint16_t CCR;
    uint16_t _res7;
    uint16_t TRISE;
    uint16_t _res8;
};

extern volatile I2C_t I2C1;
extern volatile I2C_t I2C2;

/*
 * USART / UART
 */

struct USART_t {
    uint16_t SR;
    uint16_t _res0;
    uint16_t DR;
    uint16_t _res1;
    uint16_t BRR;
    uint16_t _res2;
    uint16_t CR1;
    uint16_t _res3;
    uint16_t CR2;
    uint16_t _res4;
    uint16_t CR3;
    uint16_t _res5;
    uint16_t GTPR;
    uint16_t _res6;
};

extern volatile USART_t USART1;
extern volatile USART_t USART2;
extern volatile USART_t USART3;
extern volatile USART_t UART4;
extern volatile USART_t UART5;

struct TIM_CCR_t {
    uint16_t CCR;
    uint16_t _res;
};

/*
 * Timers 1-5 & 8
 */
struct TIM_t {
    uint16_t CR1;
    uint16_t _res0;
    uint16_t CR2;
    uint16_t _res1;
    uint16_t SMCR;
    uint16_t _res2;
    uint16_t DIER;
    uint16_t _res3;
    uint16_t SR;
    uint16_t _res4;
    uint16_t EGR;
    uint16_t _res5;
    uint16_t CCMR1;
    uint16_t _res6;
    uint16_t CCMR2;
    uint16_t _res7;
    uint16_t CCER;
    uint16_t _res8;
    uint16_t CNT;
    uint16_t _res9;
    uint16_t PSC;
    uint16_t _res10;
    uint16_t ARR;
    uint16_t _res11;
    uint16_t RCR;
    uint16_t _res12;
    TIM_CCR_t compareCapRegs[4];
    uint16_t BDTR;
    uint16_t _res17;
    uint16_t DCR;
    uint16_t _res18;
    uint16_t DMAR;
    uint16_t _res19;
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

union DACChannel_t {
    uint32_t DHR[3];
    struct {
        uint32_t DHR12R;
        uint32_t DHR12L;
        uint32_t DHR8R1;
    };
};

struct DAC_t {
    uint32_t CR;
    uint32_t SWTRIG;
    DACChannel_t channels[2];
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

    uint32_t res0[188];
};

// host channel
struct USBOTG_HC_t {
    uint32_t HCCHAR;
    uint32_t res0;
    uint32_t HCINT;
    uint32_t HCGINTMSK;
    uint32_t HCTSIZ;
    uint32_t res[3];
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
    uint32_t reserved0x41C[9];
    uint32_t HPRT;
    uint32_t reserved[47];

    struct USBOTG_HC_t channels[8]; // offset 0x500
    uint32_t res2[128];
};

// IN endpoints
struct USBOTG_IN_EP_t {
    uint32_t DIEPCTL;
    uint32_t reserved0x04;
    uint32_t DIEPINT;
    uint32_t reserved0xC;
    uint32_t DIEPTSIZ;
    uint32_t DTXFSTS;
    uint32_t reserved0x18[2];
};

// OUT endpoints
struct USBOTG_OUT_EP_t {
    uint32_t DOEPCTL;
    uint32_t reserved0x04;
    uint32_t DOEPINT;
    uint32_t reserved0xC;
    uint32_t DOEPTSIZ;
    uint32_t reserved0x1C[3];
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
    uint32_t reserved0x830;
    uint32_t DIEPEMPMSK;
    uint32_t res[50];

    struct USBOTG_IN_EP_t   inEps[8];           // offset 0x900
    uint32_t res4[64];

    struct USBOTG_OUT_EP_t  outEps[8];          // offset 0xB00
    uint32_t res5[64];
};

struct USBOTG_t {
    struct USBOTG_GLOBAL_t  global;             // offset 0x0
    struct USBOTG_HOST_t    host;               // offset 0x400
    struct USBOTG_DEVICE_t  device;             // offset 0x800
    uint32_t res5[64];
    uint32_t PCGCCTL;                           // offset 0xE00
    uint32_t res6[127];
    uint32_t epFifos[4][0x400];                 // offset 0x1000
};

extern volatile USBOTG_t OTG;

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
        asm volatile ("dsb");                                       // paranoia - ensure memory access
        while (1);                                                  // wait to reset
    }

    void deinit() volatile {
        irqClearEnable[0]  = 0xFFFFFFFF;
        irqClearEnable[1]  = 0x0FFFFFFF;
        irqClearPending[0] = 0xFFFFFFFF;
        irqClearPending[1] = 0x0FFFFFFF;

        for (unsigned i = 0; i < 0x40; i++) {
            irqPriority[i] = 0x0;
        }
    }
};

extern volatile NVIC_t NVIC;


#endif
