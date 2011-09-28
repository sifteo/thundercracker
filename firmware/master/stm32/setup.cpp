/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Low level hardware setup for the STM32 board.
 */

#include <sifteo/abi.h>
#include "radio.h"
#include "runtime.h"
#include "hardware.h"

/* Addresses defined by our linker script */
extern "C" unsigned _stack;
extern "C" unsigned __bss_start;
extern "C" unsigned __bss_end;
extern "C" unsigned __data_start;
extern "C" unsigned __data_end;
extern "C" unsigned __data_src;


extern "C" void _start()
{
    // Initialize data
    memset(&__bss_start, 0, (uintptr_t)&__bss_end - (uintptr_t)&__bss_start);
    memcpy(&__data_start, &__data_src, (uintptr_t)&__data_end - (uintptr_t)&__data_start);

    // Enable peripheral clocks
    RCC.APB2ENR = 0x0000003c;

    // Configure GPIOs
    GPIOA.CRL = 0x44424444;

    Radio::open();
    Runtime::run();
}

extern "C" void *_sbrk(intptr_t increment)
{
    return NULL;
}

__attribute__ ((section (".vectors"))) uintptr_t vector_table[] = {
    (uintptr_t) &_stack,
    (uintptr_t) &_start,
};

