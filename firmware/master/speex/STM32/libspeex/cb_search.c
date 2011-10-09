/* Copyright (C) 2002-2006 Jean-Marc Valin 
   File: cb_search.c

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cb_search.h"
#include "filters.h"
#include "stack_alloc.h"
#include "vq.h"
#include "arch.h"
#include "math_approx.h"
#include "os_support.h"

#ifdef _USE_SSE
#include "cb_search_sse.h"
#elif defined(ARM4_ASM) || defined(ARM5E_ASM)
#include "cb_search_arm4.h"
#elif defined(BFIN_ASM)
#include "cb_search_bfin.h"
#endif

//added by MCD Application Team June 2008
#include "cb_search_cortexM3.h"


#ifndef OVERRIDE_COMPUTE_WEIGHTED_CODEBOOK
static void compute_weighted_codebook(const signed char *shape_cb, const spx_word16_t *r, spx_word16_t *resp, spx_word16_t *resp2, spx_word32_t *E, int shape_cb_size, int subvect_size, char *stack)
{
   int i, j, k;
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
      for(j=0;j<subvect_size;j++)
      {
         spx_word32_t resj=0;
         spx_word16_t res16;
         for (k=0;k<=j;k++)
            resj = MAC16_16(resj,shape[k],r[j-k]);
#ifdef FIXED_POINT
         res16 = EXTRACT16(SHR32(resj, 13));
#else
         res16 = 0.03125f*resj;
#endif
         /* Compute codeword energy */
         E[i]=MAC16_16(E[i],res16,res16);
         res[j] = res16;
         /*printf ("%d\n", (int)res[j]);*/
      }
   }

}
#endif

#ifndef OVERRIDE_TARGET_UPDATE
static inline void target_update(spx_word16_t *t, spx_word16_t g, spx_word16_t *r, int len)
{
   int n;
   for (n=0;n<len;n++)
      t[n] = SUB16(t[n],PSHR32(MULT16_16(g,r[n]),13));
}
#endif

/********************************************************************************************/
/* This function has been modified by STMicroelectronics, MCD Application team, June 2008.  */
/********************************************************************************************/
extern const signed char exc_10_32_table[];
void split_cb_search_shape_sign(
spx_word16_t target[],			/* target vector */
spx_coef_t ak[],			/* LPCs for this subframe */
spx_coef_t awk1[],			/* Weighted LPCs for this subframe */
spx_coef_t awk2[],			/* Weighted LPCs for this subframe */
const void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
spx_sig_t *exc,
spx_word16_t *r,
SpeexBits *bits,
char *stack,
int   complexity,
int   update_target
)
{
   int i,j,m,q;
   const signed char *shape_cb;
   int shape_cb_size = 32, subvect_size = 10;
   int best_index;
   spx_word32_t best_dist;
   spx_word16_t resp[320];
   spx_word16_t *resp2 = resp;
   spx_word32_t E[32];
   spx_word16_t t[40];
   spx_sig_t  e[40];
   shape_cb=exc_10_32_table;

   
   /* FIXME: Do we still need to copy the target? */
   SPEEX_COPY(t, target, nsf);

   //compute_weighted_codebook
   {
     int i, k;
     spx_word16_t shape[10];
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

   for (i=0;i<4;i++)
   {
      spx_word16_t *x=t+subvect_size*i;
      /*Find new n-best based on previous n-best j*/
      vq_nbest(x, resp2, subvect_size, shape_cb_size, E, 1, &best_index, &best_dist, stack);
      
      speex_bits_pack(bits,best_index,5);
      
      {
         int rind;
         spx_word16_t *res;
         spx_word16_t sign=1;
         rind = best_index;
         if (rind>=shape_cb_size)
         {
            sign=-1;
            rind-=shape_cb_size;
         }
         res = resp+rind*subvect_size;
         if (sign>0)
            for (m=0;m<subvect_size;m++)
               t[subvect_size*i+m] = SUB16(t[subvect_size*i+m], res[m]);
         else
            for (m=0;m<subvect_size;m++)
               t[subvect_size*i+m] = ADD16(t[subvect_size*i+m], res[m]);

         if (sign==1)
         {
            for (j=0;j<subvect_size;j++)
               e[subvect_size*i+j]=SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5);
         } else {
            for (j=0;j<subvect_size;j++)
               e[subvect_size*i+j]=NEG32(SHL32(EXTEND32(shape_cb[rind*subvect_size+j]),SIG_SHIFT-5));
         }
      
      }
            
      for (m=0;m<subvect_size;m++)
      {
         spx_word16_t g;
         int rind;
         spx_word16_t sign=1;
         rind = best_index;
         if (rind>=shape_cb_size)
         {
            sign=-1;
            rind-=shape_cb_size;
         }
         
         q=subvect_size-m;
         g=sign*shape_cb[rind*subvect_size+m];

         target_update(t+subvect_size*(i+1), g, r+q, nsf-subvect_size*(i+1));
      }
   }

   /* Update excitation */
   /* FIXME: We could update the excitation directly above */
   for (j=0;j<nsf;j++)
      exc[j]=ADD32(exc[j],e[j]);

}

void split_cb_shape_sign_unquant(
spx_sig_t *exc,
const void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack,
spx_int32_t *seed
)
{
   int i,j;
   VARDECL(int *ind);
   VARDECL(int *signs);
   const signed char *shape_cb;
   int subvect_size, nb_subvect;
   const split_cb_params *params;
   int have_sign;

   params = (const split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;

   ALLOC(ind, nb_subvect, int);
   ALLOC(signs, nb_subvect, int);

   /* Decode codewords and gains */
   for (i=0;i<nb_subvect;i++)
   {
      if (have_sign)
         signs[i] = speex_bits_unpack_unsigned(bits, 1);
      else
         signs[i] = 0;
      ind[i] = speex_bits_unpack_unsigned(bits, params->shape_bits);
   }
   /* Compute decoded excitation */
   for (i=0;i<nb_subvect;i++)
   {
      spx_word16_t s=1;
      if (signs[i])
         s=-1;
#ifdef FIXED_POINT
      if (s==1)
      {
         for (j=0;j<subvect_size;j++)
            exc[subvect_size*i+j]=SHL32(EXTEND32(shape_cb[ind[i]*subvect_size+j]),SIG_SHIFT-5);
      } else {
         for (j=0;j<subvect_size;j++)
            exc[subvect_size*i+j]=NEG32(SHL32(EXTEND32(shape_cb[ind[i]*subvect_size+j]),SIG_SHIFT-5));
      }
#else
      for (j=0;j<subvect_size;j++)
         exc[subvect_size*i+j]+=s*0.03125*shape_cb[ind[i]*subvect_size+j];      
#endif
   }
}

void noise_codebook_quant(
spx_word16_t target[],			/* target vector */
spx_coef_t ak[],			/* LPCs for this subframe */
spx_coef_t awk1[],			/* Weighted LPCs for this subframe */
spx_coef_t awk2[],			/* Weighted LPCs for this subframe */
const void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
spx_sig_t *exc,
spx_word16_t *r,
SpeexBits *bits,
char *stack,
int   complexity,
int   update_target
)
{
   int i;
   VARDECL(spx_word16_t *tmp);
   ALLOC(tmp, nsf, spx_word16_t);
   residue_percep_zero16(target, ak, awk1, awk2, tmp, nsf, p, stack);

   for (i=0;i<nsf;i++)
      exc[i]+=SHL32(EXTEND32(tmp[i]),8);
   SPEEX_MEMSET(target, 0, nsf);
}


void noise_codebook_unquant(
spx_sig_t *exc,
const void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack,
spx_int32_t *seed
)
{
   int i;
   /* FIXME: This is bad, but I don't think the function ever gets called anyway */
   for (i=0;i<nsf;i++)
      exc[i]=SHL32(EXTEND32(speex_rand(1, seed)),SIG_SHIFT);
}
