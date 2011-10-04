/* Copyright (C) 2008 STMicroelectronics, MCD */
/*

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

   SECTION .text:CODE(2)
   EXPORT  inner_prod
   
   

#define x       R0    // input array x[]
#define c       R1    // input array c[]
#define N       R2    // number of samples (a multiple of 4) // 40 samples: the lpcsize == 40 for narrowband mode, submode3
#define acc     R3    // accumulator
#define sum     R6    // sum
#define x_0     R4    // elements from array x[]
#define x_1     R5
#define c_0     R7    // elements from array c[]
#define c_1     R8





inner_prod

    PUSH    {r4-r8, lr}
    MOV     sum, #0
        
Loop
        
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc     
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc       
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc       
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc       
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc       
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6
    
    MOV     acc, #0
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    LDRSH   x_0, [x], #2
    LDRSH   c_0, [c], #2
    MLA     acc, x_0, c_0, acc        
    LDRSH   x_1, [x], #2
    LDRSH   c_1, [c], #2
    MLA     acc, x_1, c_1, acc        
    ADD     sum,sum,acc,ASR #6		
    MOV     r0, sum
    
    POP   {r4-r8, pc}
    END