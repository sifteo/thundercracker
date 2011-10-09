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
