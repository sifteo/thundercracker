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

#ifndef __LTP_M3_H
#define __LTP_M3_H


#define OVERRIDE_INNER_PROD
 __asm spx_word32_t inner_prod(const spx_word16_t *x, const spx_word16_t *y, int len)
{
x       RN    0    // input array x[]
c       RN    1    // input array c[]
N       RN    2    // number of samples (a multiple of 4) // 40 samples: the lpcsize == 40 for narrowband mode, submode3
acc     RN    3    // accumulator
sum     RN    6    // sum
x_0     RN    4    // elements from array x[]
x_1     RN    5
c_0     RN    7    // elements from array c[]
c_1     RN    8		

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
}


#define OVERRIDE_COMPUTE_PITCH_ERROR
#define OVERRIDE_PITCH_GAIN_SEARCH_3TAP_VQ
int pitch_gain_search_3tap_vq(
  const signed char *gain_cdbk,
  int                gain_cdbk_size,
  spx_word16_t      *C16,
  spx_word16_t       max_gain
)
{
  int                best_cdbk=0;
  spx_word32_t       best_sum=-VERY_LARGE32;

  int                i;


spx_word16_t g_0[32]={
0, 1, -9, -24, 
19, 28, -9, 24, 
33, 10, -6, 11, 
32, -21, -5, 13, 
38, -1, -8, 15,
1, 9, -11, -2, 
16, 18, 1, 11,
-15, -14, 4, 22};

spx_word16_t g_1[32]={
0, -26, 8, 10, 
65, -7, 47, 17, 
34, -34, 60, 46, 
53, -39, 31, 27, 
97, -16, 89, 36, 
70, 60, 61, 45, 
47, -50, 57, 37, 
-31, 44, 15, 46};

spx_word16_t g_2[32]={
0, 16, -11, -23, 
-9, 23, 20, 20, 
-12, -10, 9, -5, 
-18, 5, 13, 4, 
-12, -1, 18, -13, 
-1, -8, 20, 9, 
5, 17, 0, 27, 
-19, 35, 3, -8};

spx_word16_t gain_sum[32]={
0, 22, 14, 29, 
47, 29, 38, 31, 
40, 27, 38, 31, 
52, 33, 25, 22, 
74, 9, 58, 32, 
36, 39, 46, 28, 
34, 43, 29, 38, 
33, 47, 11, 38};

  for (i=0;i<32;i++) 
  {
    
	spx_word32_t sum = 0;     
         
    sum = ADD32(sum,MULT16_16(MULT16_16_16(g_0[i],64),C16[0]));
    sum = ADD32(sum,MULT16_16(MULT16_16_16(g_1[i],64),C16[1]));
    sum = ADD32(sum,MULT16_16(MULT16_16_16(g_2[i],64),C16[2]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_0[i],g_1[i]),C16[3]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_2[i],g_1[i]),C16[4]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_2[i],g_0[i]),C16[5]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_0[i],g_0[i]),C16[6]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_1[i],g_1[i]),C16[7]));
    sum = SUB32(sum,MULT16_16(MULT16_16_16(g_2[i],g_2[i]),C16[8]));
         
    if (sum>best_sum && gain_sum[i]<=max_gain) {
      best_sum=sum;
      best_cdbk=i;
    }
  }

  return best_cdbk;
}


#endif
