/*
 * indexPSIv1.c for the OpenBWT project
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


/*
  PSI array + binary search algorithm v1
    space: 4n+4*256 bytes
     time: O(n log 256)
*/

int
obwt_unbwt_indexPSIv1(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[256];
  int *PSI;
  int i, t;
  int c, len, half;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4n bytes of memory. */
  PSI = (int *)malloc(n * sizeof(int));
  if(PSI == NULL) { return -2; }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 0; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  for(i = 0; i < pidx; ++i) { PSI[C[T[i]]++] = i - 1; }
  for(; i < n; ++i) { PSI[C[T[i]]++] = i; }
  for(i = 0, t = pidx - 1; i < n; ++i) {
    for(c = 0, len = 256 - 1, half = len >> 1; 0 < len; len = half, half >>= 1) {
      if(C[c + half] <= t) { c += half + 1; }
    }
    U[i] = (unsigned char)c;
    t = PSI[t];
  }

  /* Deallocate memory. */
  free(PSI);

  return 0;
}

int
obwt_unbwt_indexPSIv1_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[256];
  int *PSI;
  int i, s, t, pidx;
  int c;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((ISAs->numsamples == 1) || (numthreads <= 1)) { return obwt_unbwt_indexPSIv1(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4n bytes of memory. */
  PSI = (int *)malloc(n * sizeof(int));
  if(PSI == NULL) { return -2; }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 0; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  for(i = 0; i < pidx; ++i) { PSI[C[T[i]]++] = i - 1; }
  for(; i < n; ++i) { PSI[C[T[i]]++] = i; }
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, C, PSI) private(i, s, t, c) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    int len, half;
    first = s << ISAs->lgsamplerate;
    last = ((s + 1) < ISAs->numsamples) ? (s + 1) << ISAs->lgsamplerate : n;
    t = ISAs->samples[s] - 1;
    for(i = first; i < last; ++i) {
      for(c = 0, len = 256 - 1, half = len >> 1; 0 < len; len = half, half >>= 1) {
        if(C[c + half] <= t) { c += half + 1; }
      }
      U[i] = (unsigned char)c;
      t = PSI[t];
    }
  }

  /* Deallocate memory. */
  free(PSI);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_indexPSIv1(T, U, n, ISAs->samples[0]);
#endif
}
