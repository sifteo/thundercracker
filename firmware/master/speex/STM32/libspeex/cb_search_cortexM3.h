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

#ifndef __CB_SEARCH_M3_H
#define __CB_SEARCH_M3_H


extern const signed char exc_10_32_table[];
#define OVERRIDE_COMPUTE_WEIGHTED_CODEBOOK
#if 0
static void compute_weighted_codebook(const signed char *shape_cb, const spx_word16_t *r, spx_word16_t *resp, spx_word16_t *resp2, spx_word32_t *E, int shape_cb_size, int subvect_size, char *stack)
{
   int i, k;
   VARDECL(spx_word16_t *shape);
   ALLOC(shape, subvect_size, spx_word16_t);
   for (i=0;i<shape_cb_size;i++)
   {
      spx_word16_t *res;
      
      res = resp+i*subvect_size;
      for (k=0;k<subvect_size;k++)
         shape[k] = (spx_word16_t)shape_cb[i*subvect_size+k];
      E[i]=0;

      /* Compute codeword response using convolution with impulse response */
      {
	         spx_word32_t resj;
             spx_word16_t res16;
	  	 
		     // 0          
             resj = MULT16_16(shape[0],r[0]);
		     res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[0] = res16;
             //++++++++++++++++++++++++++
         
		     // 1          
             resj = MULT16_16(shape[0],r[1]);    
		     resj = MAC16_16(resj,shape[1],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[1] = res16;
             //++++++++++++++++++++++++++
         
             // 2         
             resj = MULT16_16(shape[0],r[2]);    
		     resj = MAC16_16(resj,shape[1],r[1]);
             resj = MAC16_16(resj,shape[2],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[2] = res16;
             //++++++++++++++++++++++++++
         
             // 3          
             resj = MULT16_16(shape[0],r[3]);
             resj = MAC16_16(resj,shape[1],r[2]);
             resj = MAC16_16(resj,shape[2],r[1]);
		     resj = MAC16_16(resj,shape[3],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[3] = res16;
             //++++++++++++++++++++++++++
         
             // 4        
             resj = MULT16_16(shape[0],r[4]);
             resj = MAC16_16(resj,shape[1],r[3]);
             resj = MAC16_16(resj,shape[2],r[2]);
             resj = MAC16_16(resj,shape[3],r[1]);
		     resj = MAC16_16(resj,shape[4],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[4] = res16;
             //++++++++++++++++++++++++++
         
             // 5   
             resj = MULT16_16(shape[0],r[5]);
             resj = MAC16_16(resj,shape[1],r[4]);
             resj = MAC16_16(resj,shape[2],r[3]);
             resj = MAC16_16(resj,shape[3],r[2]);
             resj = MAC16_16(resj,shape[4],r[1]);
		     resj = MAC16_16(resj,shape[5],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[5] = res16;
             //++++++++++++++++++++++++++
         
             // 6         
             resj = MULT16_16(shape[0],r[6]);
             resj = MAC16_16(resj,shape[1],r[5]);
             resj = MAC16_16(resj,shape[2],r[4]);
             resj = MAC16_16(resj,shape[3],r[3]);
             resj = MAC16_16(resj,shape[4],r[2]);
             resj = MAC16_16(resj,shape[5],r[1]);
		     resj = MAC16_16(resj,shape[6],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[6] = res16;
             //++++++++++++++++++++++++++
         
             // 7 
             resj = MULT16_16(shape[0],r[7]);
             resj = MAC16_16(resj,shape[1],r[6]);
             resj = MAC16_16(resj,shape[2],r[5]);
             resj = MAC16_16(resj,shape[3],r[4]);
             resj = MAC16_16(resj,shape[4],r[3]);
             resj = MAC16_16(resj,shape[5],r[2]);
             resj = MAC16_16(resj,shape[6],r[1]);
		     resj = MAC16_16(resj,shape[7],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[7] = res16;
             //++++++++++++++++++++++++++
         
             // 8          
             resj = MULT16_16(shape[0],r[8]);
             resj = MAC16_16(resj,shape[1],r[7]);
             resj = MAC16_16(resj,shape[2],r[6]);
             resj = MAC16_16(resj,shape[3],r[5]);
             resj = MAC16_16(resj,shape[4],r[4]);
             resj = MAC16_16(resj,shape[5],r[3]);
             resj = MAC16_16(resj,shape[6],r[2]);
             resj = MAC16_16(resj,shape[7],r[1]);
		     resj = MAC16_16(resj,shape[8],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[8] = res16;
             //++++++++++++++++++++++++++
         
             // 9       
             resj = MULT16_16(shape[0],r[9]);
             resj = MAC16_16(resj,shape[1],r[8]);
             resj = MAC16_16(resj,shape[2],r[7]);
             resj = MAC16_16(resj,shape[3],r[6]);
             resj = MAC16_16(resj,shape[4],r[5]);
             resj = MAC16_16(resj,shape[5],r[4]);
             resj = MAC16_16(resj,shape[6],r[3]);
             resj = MAC16_16(resj,shape[7],r[2]);
             resj = MAC16_16(resj,shape[8],r[1]);
		     resj = MAC16_16(resj,shape[9],r[0]);
             res16 = EXTRACT16(SHR32(resj, 13));
             // Compute codeword energy 
             E[i]=MAC16_16(E[i],res16,res16);
             res[9] = res16;
             //++++++++++++++++++++++++++
     }
   }

}
#endif

#endif
