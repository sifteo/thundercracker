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
   
   EXPORT  filter_mem16
   EXPORT  iir_mem16
   EXPORT  fir_mem16
   
   
#define x1       R0
#define num1     R1
#define den1     R2
#define y1       R3
#define N1       R12
#define mem1     LR

#define mem1_0   R4
#define mem1_1   R5
#define mem1_2   R4
#define mem1_3   R5
#define mem1_4   R4
#define mem1_5   R5
#define mem1_6   R4
#define mem1_7   R5
#define mem1_8   R4
#define mem1_9   R5
 
#define num1_0   R7
#define num1_1   R7
#define num1_2   R7
#define num1_3   R7
#define num1_4   R7
#define num1_5   R7
#define num1_6   R7
#define num1_7   R7
#define num1_8   R7
#define num1_9   R7

#define den1_0   R8
#define den1_1   R8
#define den1_2   R8
#define den1_3   R8
#define den1_4   R8
#define den1_5   R8
#define den1_6   R8
#define den1_7   R8
#define den1_8   R8
#define den1_9   R8
  
#define var1     R9
#define x1_i     R10
#define y1_i     R11
#define ny1i     R11
/*******************/
#define x2       R0
#define den2     R1
#define y2       R2
#define N2       R3
#define mem2     LR

#define mem2_0   R5
#define mem2_1   R4
#define mem2_2   R5
#define mem2_3   R4
#define mem2_4   R5
#define mem2_5   R4
#define mem2_6   R5
#define mem2_7   R4
#define mem2_8   R5
#define mem2_9   R4
 
#define den2_0   R7
#define den2_1   R7
#define den2_2   R7
#define den2_3   R7
#define den2_4   R7
#define den2_5   R7
#define den2_6   R7
#define den2_7   R7
#define den2_8   R7
#define den2_9   R7
  
#define var2     R7
#define x2_i     R8
#define y2_i     R6
#define ny2i     R6
/******************/
#define x3       R0
#define num3     R1
#define y3       R2
#define N3       R3
#define mem3   	 LR
  
#define mem3_0   R4
#define mem3_1   R5
#define mem3_2   R4
#define mem3_3   R5
#define mem3_4   R4
#define mem3_5   R5
#define mem3_6   R4
#define mem3_7   R5
#define mem3_8   R4
#define mem3_9   R5
 
#define num3_0   R7
#define num3_1   R7
#define num3_2   R7
#define num3_3   R7
#define num3_4   R7
#define num3_5   R7
#define num3_6   R7
#define num3_7   R7
#define num3_8   R7
#define num3_9   R7
  
#define x3_i     R6
#define y3_i     R7
#define var3     R8

filter_mem16

        PUSH   {r4-r12,lr}
	LDR     N1, [sp,#40]
	LDR     mem1, [sp,#48]
Loop1

	LDR    mem1_0, [mem1]
	LDRSH  x1_i,[x1], #2

	ADD	   var1, mem1_0, #0x1000   // mem[0] += 1<<LPC_SFIFT>>1; LPC_SFIFT == 13 
	ADD	   var1, x1_i, var1, ASR #13 // mem[0]>>LPC_SFIFT + x[i]
		
	SSAT.w  y1_i, #16, var1   //saturate(..., 32767)

	STRH   y1_i, [y1], #2
	 
	RSB	ny1i, y1_i, #0x00	 //nyi = -yi
	SXTH   ny1i, ny1i

	LDRSH  num1_0, [num1]
	LDR    mem1_1, [mem1, #4]   
	MLA    mem1_1, num1_0, x1_i, mem1_1  //mem0 = num0*x_i + mem1	
	LDRSH  den1_0, [den1]   
	MLA    mem1_0, den1_0, ny1i, mem1_1    //mem0 = den0*nyi + mem1
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

	LDRSH  num1_9, [num1, #(9*2)]
	//MUL    num1_9, x1_i
        MUL    num1_9, num1_9, x1_i

	LDRSH  den1_9, [den1, #(9*2)]
	//MUL    den1_9, ny1i
        MUL    den1_9, den1_9, ny1i
	
	ADD   den1_9,num1_9
	STR   den1_9, [mem1, #(9*4)]

	SUBS  N1, N1, #1
	BNE   Loop1

	POP   {r4-r12,pc}
        
iir_mem16        
        
        PUSH    {r4-r8,lr}
	LDR     mem2, [sp, #28]

Loop2
   	 
	LDR     mem2_0, [mem2]	 
	LDRSH   x2_i,[x2], #2

	ADD	var2, mem2_0, #0x1000        
	ADD	x2_i, x2_i, var2, ASR #13     							
		
	SSAT.w  y2_i, #16, x2_i    
		
	STRH   y2_i, [y2], #2
	 
	RSB	ny2i, y2_i, #0x00	
	SXTH   ny2i, ny2i
	 
	LDRSH  den2_0, [den2]
	LDR    mem2_1, [mem2, #4]   
	MLA    mem2_0, den2_0, ny2i, mem2_1    
	STR    mem2_0, [mem2]
	 
	LDRSH  den2_1, [den2,#2]
	LDR    mem2_2, [mem2,#(2*4)]
	MLA    mem2_1, den2_1, ny2i, mem2_2    
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
	//MUL   den2_9, ny2i
        MUL   den2_9, den2_9, ny2i
	STR   den2_9, [mem2, #(9*4)] 
	 
	SUBS  N2, N2, #1
	BNE   Loop2
	 
	POP   {r4-r8,pc}





fir_mem16
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
	//MUL   num3_9, x3_i
        MUL   num3_9, num3_9, x3_i
	STR   num3_9, [mem3, #(9*4)] 
	 
	SUBS  N3, N3, #1
	BNE   Loop3
	 
	POP   {r4-r8,pc}
        
        END