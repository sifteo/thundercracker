/*
 * saLF.c for the OpenBWT project
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
  SA from LF algorithm
    space: 4(n+1)+4*256 bytes
     time: O(n)
*/

int
obwt_unbwt_saLF(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[256];
  int *LF, *SA;
  int i, t, u;
  int c;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4(n+1) bytes of memory. */
  LF = (int *)malloc((n + 1) * sizeof(int));
  if(LF == NULL) { return -2; }
  SA = LF;

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 1; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  for(i = 0; i < pidx; ++i) { LF[i] = C[T[i]]++; }
  LF[pidx] = 0;
  for(i = pidx + 1; i <= n; ++i) { LF[i] = C[T[i - 1]]++; }
  for(i = n, t = 0; 0 < i; --i) { u = t; t = LF[u]; SA[u] = i; }
  SA[pidx] = 0;
  for(c = 0; c < 256; ++c) {
    for(i = (0 < c) ? C[c - 1] : 1; i < C[c]; ++i) { U[SA[i]] = (unsigned char)c; }
  }

  /* Deallocate memory. */
  free(LF);

  return 0;
}

int
obwt_unbwt_saLF_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[256];
  int *LF, *SA;
  int i, s, t, u, pidx;
  int c;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((ISAs->numsamples == 1) || (numthreads <= 1)) { return obwt_unbwt_saLF(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4(n+1) bytes of memory. */
  LF = (int *)malloc((n + 1) * sizeof(int));
  if(LF == NULL) { free(LF); return -2; }
  SA = LF;

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 1; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  for(i = 0; i < pidx; ++i) { LF[i] = C[T[i]]++; }
  LF[pidx] = 0;
  for(i = pidx + 1; i <= n; ++i) { LF[i] = C[T[i - 1]]++; }

# pragma omp parallel for schedule(guided) default(none) shared(ISAs, n, LF, SA) private(i, s, t, u) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    if(0 < s) {
      first = (s - 1) << ISAs->lgsamplerate;
      last = s << ISAs->lgsamplerate;
      t = ISAs->samples[s];
    } else {
      first = (ISAs->numsamples - s - 1) << ISAs->lgsamplerate;
      last = n;
      t = 0;
    }
    for(i = last; first < i; --i) { u = t; t = LF[u]; SA[u] = i; }
  }
  SA[pidx] = 0;

# pragma omp parallel for schedule(guided) default(none) shared(U, SA, C, numthreads) private(i, c) num_threads(numthreads)
  for(c = 0; c < 256; ++c) {
//# pragma omp parallel for schedule(static) default(none) shared(U, SA, C, c) private(i) num_threads(numthreads)
    for(i = (0 < c) ? C[c - 1] : 1; i < C[c]; ++i) { U[SA[i]] = (unsigned char)c; }
  }

  /* Deallocate memory. */
  free(LF);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_saLF(T, U, n, ISAs->samples[0]);
#endif
}
