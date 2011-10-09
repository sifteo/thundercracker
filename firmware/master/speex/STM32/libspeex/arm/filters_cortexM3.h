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

#ifndef __FILTERS_M3_H
#define __FILTERS_M3_H

#define OVERRIDE_FILTER_MEM16
 __asm void filter_mem16(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack)
{ 
x1       RN   0
num1     RN   1
den1     RN   2
y1       RN   3
N1       RN   12
mem1     RN   lr

mem1_0   RN   4
mem1_1   RN   5
mem1_2   RN   4
mem1_3   RN   5
mem1_4   RN   4
mem1_5   RN   5
mem1_6   RN   4
mem1_7   RN   5
mem1_8   RN   4
mem1_9   RN   5
 
num1_0   RN   7
num1_1   RN   7
num1_2   RN   7
num1_3   RN   7
num1_4   RN   7
num1_5   RN   7
num1_6   RN   7
num1_7   RN   7
num1_8   RN   7
num1_9   RN   7

den1_0   RN   8
den1_1   RN   8
den1_2   RN   8
den1_3   RN   8
den1_4   RN   8
den1_5   RN   8
den1_6   RN   8
den1_7   RN   8
den1_8   RN   8
den1_9   RN   8
  
var1     RN   9
x1_i     RN   10
y1_i     RN   11
ny1i     RN  11


    PUSH   {r4-r12,lr}
	LDR     N1, [sp,#40]
	LDR     mem1, [sp,#48]
Loop1

	LDR    mem1_0, [mem1]
	LDRSH  x1_i,[x1], #2

	ADD	   var1, mem1_0, #0x1000    ; mem[0] += 1<<LPC_SFIFT>>1; LPC_SFIFT == 13 
	ADD	   var1, x1_i, var1, ASR #13 ; mem[0]>>LPC_SFIFT + x[i]
		
	SSAT.w  y1_i, #16, var1   ; saturate(..., 32767)

	STRH   y1_i, [y1], #2
	 
	RSB	ny1i, y1_i, #0x00	 ;nyi = -yi
	SXTH   ny1i, ny1i

	LDRSH  num1_0, [num1]
	LDR    mem1_1, [mem1, #4]   
	MLA    mem1_1, num1_0, x1_i, mem1_1  ;mem0 = num0*x_i + mem1	
	LDRSH  den1_0, [den1]   
	MLA    mem1_0, den1_0, ny1i, mem1_1    ;mem0 = den0*nyi + mem1
	STR    mem1_0, [mem1]


	LDRSH  num1_1, [num1,#2]
	LDR    mem1_2, [mem1,#(2*4)]   
	MLA    mem1_2, num1_1, x1_i, mem1_2
	LDRSH  den1_1, [den1,#2]   
	MLA    mem1_1, den1_1, ny1i, mem1_2
	STR    mem1_1, [mem1,#4]

	LDRSH  num1_2, [num1,#(2*2)]
	LDR    mem1_3, [mem1, #(3*4)]   
	MLA    mem1_3, num1_2, x1_i, mem1_3
	LDRSH  den1_2, [den1,#(2*2)]   
	MLA    mem1_2, den1_2, ny1i, mem1_3
	STR    mem1_2, [mem1, #(2*4)]

	LDRSH  num1_3, [num1,#(3*2)]
	LDR    mem1_4, [mem1, #(4*4)]   
	MLA    mem1_4, num1_3, x1_i, mem1_4
	LDRSH  den1_3, [den1,#(3*2)]   
	MLA    mem1_3, den1_3, ny1i, mem1_4
	STR    mem1_3, [mem1, #(3*4)]

	LDRSH  num1_4, [num1,#(4*2)]
	LDR    mem1_5, [mem1, #(5*4)]   
	MLA    mem1_5, num1_4, x1_i, mem1_5
	LDRSH  den1_4, [den1,#(4*2)]   
	MLA    mem1_4, den1_4, ny1i, mem1_5
	STR    mem1_4, [mem1, #(4*4)]

	LDRSH  num1_5, [num1,#(5*2)]
	LDR    mem1_6, [mem1, #(6*4)]   
	MLA    mem1_6, num1_5, x1_i, mem1_6
	LDRSH  den1_5, [den1,#(5*2)]   
	MLA    mem1_5, den1_5, ny1i, mem1_6
	STR    mem1_5, [mem1, #(5*4)]

	LDRSH  num1_6, [num1,#(6*2)]
	LDR    mem1_7, [mem1, #(7*4)]   
	MLA    mem1_7, num1_6, x1_i, mem1_7
	LDRSH  den1_6, [den1,#(6*2)]   
	MLA    mem1_6, den1_6, ny1i, mem1_7
	STR    mem1_6, [mem1, #(6*4)]

	LDRSH  num1_7, [num1,#(7*2)]
	LDR    mem1_8, [mem1, #(8*4)]   
	MLA    mem1_8, num1_7, x1_i, mem1_8
	LDRSH  den1_7, [den1,#(7*2)]   
	MLA    mem1_7, den1_7, ny1i, mem1_8
	STR    mem1_7, [mem1, #(7*4)]

	LDRSH  num1_8, [num1,#(8*2)]
	LDR    mem1_9, [mem1, #(9*4)]   
	MLA    mem1_9, num1_8, x1_i, mem1_9
	LDRSH  den1_8, [den1,#(8*2)]   
	MLA    mem1_8, den1_8, ny1i, mem1_9
	STR    mem1_8, [mem1, #(8*4)]

	LDRSH num1_9, [num1,#(9*2)]
	MUL   num1_9, x1_i

	LDRSH den1_9, [den1,#(9*2)]
	MUL   den1_9, ny1i
	
	ADD   den1_9,num1_9
	STR   den1_9, [mem1, #(9*4)]

	SUBS  N1, N1, #1
	BNE   Loop1

	POP   {r4-r12,pc}
}

#define OVERRIDE_IIR_MEM16
__asm void iir_mem16(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack) 
{


x2       RN 0
den2     RN 1
y2       RN 2
N2       RN 3
mem2     RN lr

mem2_0   RN 5
mem2_1   RN 4
mem2_2   RN 5
mem2_3   RN 4
mem2_4   RN 5
mem2_5   RN 4
mem2_6   RN 5
mem2_7   RN 4
mem2_8   RN 5
mem2_9   RN 4
 
den2_0   RN 7
den2_1   RN 7
den2_2   RN 7
den2_3   RN 7
den2_4   RN 7
den2_5   RN 7
den2_6   RN 7
den2_7   RN 7
den2_8   RN 7
den2_9   RN 7
  
var2     RN 7
x2_i     RN 8
y2_i     RN 6
ny2i     RN 6
	 
	PUSH    {r4-r8,lr}
	LDR     mem2, [sp, #28]

Loop2
   	 
	LDR     mem2_0, [mem2]	 
	LDRSH   x2_i,[x2], #2

	ADD	var2, mem2_0, #0x1000        ; mem2[0] += 1<<LPC_SFIFT>>1, LPC_SFIFT == 13 
	ADD	x2_i, x2_i, var2, ASR #13     ; mem[0]>>LPC_SFIFT + x[i]							
		
	SSAT.w  y2_i, #16, x2_i    ; saturate(..., 32767)
		
	STRH   y2_i, [y2], #2
	 
	RSB	ny2i, y2_i, #0x00	 //nyi = -yi
	SXTH   ny2i, ny2i
	 
	LDRSH  den2_0, [den2]
	LDR    mem2_1, [mem2, #4]   
	MLA    mem2_0, den2_0, ny2i, mem2_1    //mem0 = den0*nyi + mem1
	STR    mem2_0, [mem2]
	 
	LDRSH  den2_1, [den2,#2]
	LDR    mem2_2, [mem2,#(2*4)]
	MLA    mem2_1, den2_1, ny2i, mem2_2    //mem1 = den1*nyi + mem2
	STR    mem2_1, [mem2,#4]
	           
	LDRSH  den2_2, [den2,#(2*2)]
	LDR    mem2_3, [mem2, #(3*4)]
	MLA    mem2_2, den2_2, ny2i, mem2_3
	STR    mem2_2, [mem2, #(2*4)]
	 
	LDRSH  den2_3, [den2,#(3*2)]
	LDR    mem2_4, [mem2, #(4*4)]
	MLA    mem2_3, den2_3, ny2i, mem2_4
	STR    mem2_3, [mem2, #(3*4)]
	 
	LDRSH  den2_4, [den2,#(4*2)]
	LDR    mem2_5, [mem2, #(5*4)]
	MLA    mem2_4, den2_4, ny2i, mem2_5
	STR    mem2_4, [mem2, #(4*4)]
	 
	LDRSH  den2_5, [den2,#(5*2)]
	LDR    mem2_6, [mem2, #(6*4)]
	MLA    mem2_5, den2_5, ny2i, mem2_6
	STR    mem2_5, [mem2, #(5*4)]
	 
	LDRSH  den2_6, [den2,#(6*2)]
	LDR    mem2_7, [mem2, #(7*4)]
	MLA    mem2_6, den2_6, ny2i, mem2_7
	STR    mem2_6, [mem2, #(6*4)]
	 
	LDRSH  den2_7, [den2,#(7*2)]
	LDR    mem2_8, [mem2, #(8*4)]
	MLA    mem2_7, den2_7, ny2i, mem2_8
	STR    mem2_7, [mem2, #(7*4)]
	 
	LDRSH  den2_8, [den2,#(8*2)]
	LDR    mem2_9, [mem2, #(9*4)]
	MLA    mem2_8, den2_8, ny2i, mem2_9
	STR    mem2_8, [mem2, #(8*4)]
	 
	LDRSH den2_9, [den2,#(9*2)]
	MUL   den2_9, ny2i
	STR   den2_9, [mem2, #(9*4)] 
	 
	SUBS  N2, N2, #1
	BNE   Loop2
	 
	POP   {r4-r8,pc}
}




#define OVERRIDE_FIR_MEM16
__inline __asm void fir_mem16(const spx_word16_t *x, const spx_coef_t *num, spx_word16_t *y, int N, int ord, spx_mem_t *mem, char *stack) 
{

x3       RN 0
num3     RN 1
y3       RN 2
N3       RN 3
mem3   	 RN lr
  
mem3_0   RN 4
mem3_1   RN 5
mem3_2   RN 4
mem3_3   RN 5
mem3_4   RN 4
mem3_5   RN 5
mem3_6   RN 4
mem3_7   RN 5
mem3_8   RN 4
mem3_9   RN 5
 
num3_0   RN 7
num3_1   RN 7
num3_2   RN 7
num3_3   RN 7
num3_4   RN 7
num3_5   RN 7
num3_6   RN 7
num3_7   RN 7
num3_8   RN 7
num3_9   RN 7
  
x3_i     RN 6
y3_i     RN 7
var3     RN 8
	 
	PUSH   {r4-r8,lr}
	LDR    mem3, [sp, #28]
	
Loop3
   	 
	LDR    mem3_0, [mem3]	 
	LDRSH  x3_i,[x3], #2

	ADD	   var3, mem3_0, #0x1000    ; mem3[0] += 1<<LPC_SFIFT>>1; LPC_SFIFT == 13 
	ADD	   var3, x3_i, var3, ASR #13 ; mem[0]>>LPC_SFIFT + x[i]
		
	SSAT.w  y3_i, #16, var3   ; saturate(..., 32767)

	STRH   y3_i, [y3], #2
	 
	LDRSH  num3_0, [num3]
	LDR    mem3_1, [mem3, #4]   
	MLA    mem3_0, num3_0, x3_i, mem3_1  ;mem0 = num0*x3_i + mem1
	STR    mem3_0, [mem3]
	        
	LDRSH  num3_1, [num3,#2]
	LDR    mem3_2, [mem3,#(2*4)]
	MLA    mem3_1, num3_1, x3_i, mem3_2  ;mem1 = num1*x_i + mem2
	STR    mem3_1, [mem3,#4]
	           
	LDRSH  num3_2, [num3,#(2*2)]
	LDR    mem3_3, [mem3, #(3*4)]
	MLA    mem3_2, num3_2, x3_i, mem3_3
	STR    mem3_2, [mem3, #(2*4)]
	 
	LDRSH  num3_3, [num3,#(3*2)]
	LDR    mem3_4, [mem3, #(4*4)]
	MLA    mem3_3, num3_3, x3_i, mem3_4
	STR    mem3_3, [mem3, #(3*4)]
	 
	LDRSH  num3_4, [num3,#(4*2)]
	LDR    mem3_5, [mem3, #(5*4)]
	MLA    mem3_4, num3_4, x3_i, mem3_5
	STR    mem3_4, [mem3, #(4*4)]
	 
	LDRSH  num3_5, [num3,#(5*2)]
	LDR    mem3_6, [mem3, #(6*4)]
	MLA    mem3_5, num3_5, x3_i, mem3_6
	STR    mem3_5, [mem3, #(5*4)]
	 
	LDRSH  num3_6, [num3,#(6*2)]
	LDR    mem3_7, [mem3, #(7*4)]
	MLA    mem3_6, num3_6, x3_i, mem3_7
	STR    mem3_6, [mem3, #(6*4)]
	 
	LDRSH  num3_7, [num3,#(7*2)]
	LDR    mem3_8, [mem3, #(8*4)]
	MLA    mem3_7, num3_7, x3_i, mem3_8
	STR    mem3_7, [mem3, #(7*4)]
	 
	LDRSH  num3_8, [num3,#(8*2)]
	LDR    mem3_9, [mem3, #(9*4)]
	MLA    mem3_8, num3_8, x3_i, mem3_9
	STR    mem3_8, [mem3, #(8*4)]
	 
	LDRSH num3_9, [num3,#(9*2)]
	MUL   num3_9, x3_i
	STR   num3_9, [mem3, #(9*4)] 
	 
	SUBS  N3, N3, #1
	BNE   Loop3
	 
	POP   {r4-r8,pc}
}

#define OVERRIDE_COMPUTE_IMPULSE_RESPONSE
void compute_impulse_response(const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_word16_t *y, int N, int ord, char *stack)
{
   int i;
   spx_word16_t y1, ny1i, ny2i;
   spx_mem_t mem1[10];
   spx_mem_t mem2[10];

   y1 = ADD16(LPC_SCALING, EXTRACT16(PSHR32(0,LPC_SHIFT)));
   ny1i = NEG16(y1);
   y[0] = PSHR32(SHL32(EXTEND32(y1),LPC_SHIFT+1),LPC_SHIFT);
   ny2i = NEG16(y[0]);
   
   mem1[0] = MULT16_16(awk2[0],ny1i);
   mem2[0] = MULT16_16(ak[0],ny2i);
      
   mem1[1] = MULT16_16(awk2[1],ny1i);
   mem2[1] = MULT16_16(ak[1],ny2i);
    
   mem1[2] = MULT16_16(awk2[2],ny1i);
   mem2[2] = MULT16_16(ak[2],ny2i);
    
   mem1[3] = MULT16_16(awk2[3],ny1i);
   mem2[3] = MULT16_16(ak[3],ny2i);
      
   mem1[4] = MULT16_16(awk2[4],ny1i);
   mem2[4] = MULT16_16(ak[4],ny2i);
      
   mem1[5] = MULT16_16(awk2[5],ny1i);
   mem2[5] = MULT16_16(ak[5],ny2i);
      
   mem1[6] = MULT16_16(awk2[6],ny1i);
   mem2[6] = MULT16_16(ak[6],ny2i);
      
   mem1[7] = MULT16_16(awk2[7],ny1i);
   mem2[7] = MULT16_16(ak[7],ny2i);
         
   mem1[8] = MULT16_16(awk2[8],ny1i);
   mem2[8] = MULT16_16(ak[8],ny2i);  
            
   mem1[9] = MULT16_16(awk2[9],ny1i);
   mem2[9] = MULT16_16(ak[9],ny2i);
   
   for (i=1;i<11;i++)
   {
      y1 = ADD16(awk1[i-1], EXTRACT16(PSHR32(mem1[0],LPC_SHIFT)));
      ny1i = NEG16(y1);
      y[i] = PSHR32(ADD32(SHL32(EXTEND32(y1),LPC_SHIFT+1),mem2[0]),LPC_SHIFT);
      ny2i = NEG16(y[i]);
      
      mem1[0] = MAC16_16(mem1[1], awk2[0],ny1i);
      mem2[0] = MAC16_16(mem2[1], ak[0],ny2i);
      
      mem1[1] = MAC16_16(mem1[2], awk2[1],ny1i);
      mem2[1] = MAC16_16(mem2[2], ak[1],ny2i);
      
      mem1[2] = MAC16_16(mem1[3], awk2[2],ny1i);
      mem2[2] = MAC16_16(mem2[3], ak[2],ny2i);
      
      mem1[3] = MAC16_16(mem1[4], awk2[3],ny1i);
      mem2[3] = MAC16_16(mem2[4], ak[3],ny2i);
      
      mem1[4] = MAC16_16(mem1[5], awk2[4],ny1i);
      mem2[4] = MAC16_16(mem2[5], ak[4],ny2i);
      
      mem1[5] = MAC16_16(mem1[6], awk2[5],ny1i);
      mem2[5] = MAC16_16(mem2[6], ak[5],ny2i);
      
      mem1[6] = MAC16_16(mem1[7], awk2[6],ny1i);
      mem2[6] = MAC16_16(mem2[7], ak[6],ny2i);
      
      mem1[7] = MAC16_16(mem1[8], awk2[7],ny1i);
      mem2[7] = MAC16_16(mem2[8], ak[7],ny2i);
         
      mem1[8] = MAC16_16(mem1[9], awk2[8],ny1i);
      mem2[8] = MAC16_16(mem2[9], ak[8],ny2i);  
            
      mem1[9] = MULT16_16(awk2[9],ny1i);
      mem2[9] = MULT16_16(ak[9],ny2i);
   }
   
   for (i=11;i<40;i++)
   {
      y1 = EXTRACT16(PSHR32(mem1[0],LPC_SHIFT));
      ny1i = NEG16(y1);
      y[i] = PSHR32(ADD32(SHL32(EXTEND32(y1),LPC_SHIFT+1),mem2[0]),LPC_SHIFT);
      ny2i = NEG16(y[i]);

      
      mem1[0] = MAC16_16(mem1[1], awk2[0],ny1i);
      mem2[0] = MAC16_16(mem2[1], ak[0],ny2i);
      
      mem1[1] = MAC16_16(mem1[2], awk2[1],ny1i);
      mem2[1] = MAC16_16(mem2[2], ak[1],ny2i);
      
      mem1[2] = MAC16_16(mem1[3], awk2[2],ny1i);
      mem2[2] = MAC16_16(mem2[3], ak[2],ny2i);
      
      mem1[3] = MAC16_16(mem1[4], awk2[3],ny1i);
      mem2[3] = MAC16_16(mem2[4], ak[3],ny2i);
      
      mem1[4] = MAC16_16(mem1[5], awk2[4],ny1i);
      mem2[4] = MAC16_16(mem2[5], ak[4],ny2i);
      
      mem1[5] = MAC16_16(mem1[6], awk2[5],ny1i);
      mem2[5] = MAC16_16(mem2[6], ak[5],ny2i);
      
      mem1[6] = MAC16_16(mem1[7], awk2[6],ny1i);
      mem2[6] = MAC16_16(mem2[7], ak[6],ny2i);
      
      mem1[7] = MAC16_16(mem1[8], awk2[7],ny1i);
      mem2[7] = MAC16_16(mem2[8], ak[7],ny2i);
         
      mem1[8] = MAC16_16(mem1[9], awk2[8],ny1i);
      mem2[8] = MAC16_16(mem2[9], ak[8],ny2i);  
            
      mem1[9] = MULT16_16(awk2[9],ny1i);
      mem2[9] = MULT16_16(ak[9],ny2i);
   }
}

#endif
