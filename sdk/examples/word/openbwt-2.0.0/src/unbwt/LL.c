/*
 * LL.c for the OpenBWT project
 * Copyright (c) 2010 Yuta Mori. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif
#include <assert.h>
#include <stdio.h>
#if defined(HAVE_STDLIB_H)
# include <stdlib.h>
#else
# if defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
#endif
#ifdef _OPENMP
# include <omp.h>
#endif
#include "openbwt.h"

#ifdef __INTEL_COMPILER
#pragma warning(disable : 2259)
#endif

/*
  Lauther and Lukovszki algorithm (L=12+1)
    space: 2.625n+8*256 bytes
     time: O(n)

  Reference:
    Ulrich Lauther and Tamas Lukovszki,
    "Space Efficient Algorithms for the Burrows-Wheeler Backtransformation",
    Proceedings of the 13th Annual European Symposium on Algorithms, LNCS 3669,
    Springer-Verlag, pp. 293-304, 2005.
*/

int
obwt_unbwt_LL(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[256], D[256];
  int *LFU;
  unsigned short *LFM;
  unsigned char *LFL;
  int i, j, k, l, t, p;
  int c;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.625n bytes of memory. */
  LFU = (int *)malloc(((((n + 8191) >> 13) + 1) << 8) * sizeof(int)); /* 0.125n bytes */
  LFM = (unsigned short *)malloc(n * sizeof(unsigned short)); /* 2n bytes */
  LFL = (unsigned char *)malloc(((n + 1) >> 1) * sizeof(unsigned char)); /* 0.5n bytes */
  if((LFU == NULL) || (LFM == NULL) || (LFL == NULL)) {
    free(LFU); free(LFM); free(LFL);
    return -2;
  }
  LFL[(n - 1) >> 1] = 0;

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0, p = 0; i < n; p += 256, i = l) {
    k = i + 4096; if(n <= k) { k = n; }
    l = k + 4096; if(n <= l) { l = n; }
    for(c = 0; c < 256; ++c) { LFU[p + c] = C[c] += D[c], D[c] = 0; }
    for(j = i; j < k; ++j) {
      t = D[c = T[j]]++;
      LFM[j] = (unsigned short)(((t & 0x0ff0) << 4) | c);
      if(j & 1) { LFL[j >> 1] |= (unsigned char)((t & 0x0f) << 4); }
      else { LFL[j >> 1] = (unsigned char)(t & 0x0f); }
    }
    for(c = 0; c < 256; ++c) { C[c] += D[c], D[c] = 0; }
    for(j = l - 1; k <= j; --j) {
      t = D[c = T[j]]++;
      LFM[j] = (unsigned short)(((t & 0x0ff0) << 4) | c);
      if(j & 1) { LFL[j >> 1] = (unsigned char)((t & 0x0f) << 4); }
      else { LFL[j >> 1] |= (unsigned char)(t & 0x0f); }
    }
  }

  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c] + D[c];
    LFU[p + c] = t;
    C[c] = i;
    i += t;
  }

  for(i = n - 1, t = 0; 0 <= i; --i) {
    U[i] = (unsigned char)(c = LFM[t] & 0xff);
    p = (t & 1) ? (LFL[t >> 1] >> 4) & 0x0f : (LFL[t >> 1] & 0x0f);
    p |= (LFM[t] & 0xff00) >> 4;
    t = ((t & 0x1fff) < 4096) ?
      (LFU[((t & ~0x1fff) >> 5) + c] + p + C[c]) :
      (LFU[((t & ~0x1fff) >> 5) + c + 256] - p + C[c] - 1);
    if(t < pidx) { ++t; }
  }

  /* Deallocate memory. */
  free(LFU);
  free(LFM);
  free(LFL);

  return 0;
}

int
obwt_unbwt_LL_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[256], D[256];
  int *LFU;
  unsigned short *LFM;
  unsigned char *LFL;
  int i, j, k, l, s, t, p, pidx;
  int c;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((ISAs->numsamples == 1) || (numthreads <= 1)) { return obwt_unbwt_LL(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 2.625n bytes of memory. */
  LFU = (int *)malloc(((((n + 8191) >> 13) + 1) << 8) * sizeof(int)); /* 0.125n bytes */
  LFM = (unsigned short *)malloc(n * sizeof(unsigned short)); /* 2n bytes */
  LFL = (unsigned char *)malloc(((n + 1) >> 1) * sizeof(unsigned char)); /* 0.5n bytes */
  if((LFU == NULL) || (LFM == NULL) || (LFL == NULL)) {
    free(LFU); free(LFM); free(LFL);
    return -2;
  }
  LFL[(n - 1) >> 1] = 0;

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0, p = 0; i < n; p += 256, i = l) {
    k = i + 4096; if(n <= k) { k = n; }
    l = k + 4096; if(n <= l) { l = n; }
    for(c = 0; c < 256; ++c) { LFU[p + c] = C[c] += D[c], D[c] = 0; }
    for(j = i; j < k; ++j) {
      t = D[c = T[j]]++;
      LFM[j] = (unsigned short)(((t & 0x0ff0) << 4) | c);
      if(j & 1) { LFL[j >> 1] |= (unsigned char)((t & 0x0f) << 4); }
      else { LFL[j >> 1] = (unsigned char)(t & 0x0f); }
    }
    for(c = 0; c < 256; ++c) { C[c] += D[c], D[c] = 0; }
    for(j = l - 1; k <= j; --j) {
      t = D[c = T[j]]++;
      LFM[j] = (unsigned short)(((t & 0x0ff0) << 4) | c);
      if(j & 1) { LFL[j >> 1] = (unsigned char)((t & 0x0f) << 4); }
      else { LFL[j >> 1] |= (unsigned char)(t & 0x0f); }
    }
  }

  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c] + D[c];
    LFU[p + c] = t;
    C[c] = i;
    i += t;
  }

# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, C, LFU, LFM, LFL, pidx) private(i, s, t, p, c) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    if(0 < s) {
      first = (s - 1) << ISAs->lgsamplerate;
      last = s << ISAs->lgsamplerate;
      t = ISAs->samples[s] - 1;
      if(t < pidx) { ++t; }
    } else {
      first = (ISAs->numsamples - s - 1) << ISAs->lgsamplerate;
      last = n;
      t = 0;
    }
    for(i = last - 1; first <= i; --i) {
      U[i] = (unsigned char)(c = LFM[t] & 0xff);
      p = (t & 1) ? (LFL[t >> 1] >> 4) & 0x0f : (LFL[t >> 1] & 0x0f);
      p |= (LFM[t] & 0xff00) >> 4;
      t = ((t & 0x1fff) < 4096) ?
        (LFU[((t & ~0x1fff) >> 5) + c] + p + C[c]) :
        (LFU[((t & ~0x1fff) >> 5) + c + 256] - p + C[c] - 1);
      if(t < pidx) { ++t; }
    }
  }

  /* Deallocate memory. */
  free(LFU);
  free(LFM);
  free(LFL);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_LL(T, U, n, ISAs->samples[0]);
#endif
}
