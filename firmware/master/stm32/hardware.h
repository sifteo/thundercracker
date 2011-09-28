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
 * GPIO
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

extern volatile GPIO_t GPIOA;
extern volatile GPIO_t GPIOB;
extern volatile GPIO_t GPIOC;
extern volatile GPIO_t GPIOD;
extern volatile GPIO_t GPIOE;
extern volatile GPIO_t GPIOF;
extern volatile GPIO_t GPIOG;

#endif
