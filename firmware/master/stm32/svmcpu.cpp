/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmcpu.h"


#if 0
     // this gets saved at interrupt by HW on cortex-m3
     struct HwStackFrame {
         reg_t r0;
         reg_t r1;
         reg_t r2;
         reg_t r3;
         reg_t r12;
         reg_t lr;   // link register
         reg_t pc;   // program counter
         reg_t psr;  // program status register
     };

     // the rest of the stack that must be saved by software
     struct SwStackFrame {
         reg_t r4;
         reg_t r5;
         reg_t r6;
         reg_t r7;
         reg_t r8;
         reg_t r9;
         reg_t r10;
         reg_t r11;
     };
#endif
