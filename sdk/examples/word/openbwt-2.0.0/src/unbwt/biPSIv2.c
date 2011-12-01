/*
 * biPSIv2.c for the OpenBWT project
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
  bi-gram based PSI array + binary search algorithm v2 (t=NUM_TOPBITS)
    space: 4(n+1)+4*257+4*256*256+4*(2**t) bytes
     time: O(n log 256)?
*/

#define NUM_TOPBITS (8)

int
obwt_unbwt_biPSIv2(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[257];
  unsigned int *PSI;
  int *D, *LD;
  unsigned int *B2S;
  unsigned int u;
  int i, t, v, w, x;
  int c, d, e, mask, lastc, len, half;
  int lowshift, highshift;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4(n+1)+262144+4(2**NUM_TOPBITS) bytes of memory. */
  PSI = (unsigned int *)malloc((n + 1) * sizeof(unsigned int));
  D = (int *)malloc(256 * 256 * sizeof(int));
  B2S = (unsigned int *)malloc((1U << NUM_TOPBITS) * sizeof(unsigned int));
  if((PSI == NULL) || (D == NULL) || (B2S == NULL)) {
    free(PSI); free(D); free(B2S);
    return -2;
  }
#if 0 < NUM_TOPBITS
  B2S[0] = 0;
#endif

  for(i = 0; 0 < (n >> i); ++i) { }
  highshift = (NUM_TOPBITS <= i) ? i - NUM_TOPBITS : 0;
  lowshift = (16 < i) ? (32 - i) : 16;
  mask = (1U << lowshift) - 1;

  /* Compute the cumulative frequency of uni-grams */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(c = 0; c < 256 * 256; ++c) { D[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 1; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  C[c] = i;

  /* Count the frequency of bi-grams */
  for(c = 0; c < 256; ++c) {
    LD = D + (c << 8);
    for(i = C[c]; i < C[c + 1]; ++i) {
      if(i != pidx) {
        d = T[i - (pidx < i)];
        ++LD[d];
      }
    }
  }

  /* Compute the cumulative frequency of bi-grams */
  lastc = T[0];
  for(c = 0, i = 1, w = 0; c < 256; ++c) {
    if(c == lastc) { i += 1; }
    LD = D + c;
    for(d = 0; d < 256; ++d) {
      t = LD[d << 8];
      if(0 < t) {
        e = (c << 8) | d;
        for(v = i >> highshift; v < w; ++v) {
          B2S[v] = (B2S[v] & 0xffff) | (((unsigned int )e) << 16);
        }
        for(w = ((i + t - 1) >> highshift) + 1; v < w; ++v) {
          B2S[v] = ((unsigned int )e) | (((unsigned int )e) << 16);
        }
      }
      LD[d << 8] = i; i += t;
    }
  }
  for(v = 0; v < w; ++v) {
    c = (B2S[v] >> 16) ^ (B2S[v] & 0xffff);
    for(x = 0; c != 0; ++x, c >>= 1) { }
    B2S[v] = (unsigned int)(((B2S[v] & 0xffff) & (0xffffU << x)) | (x << 16));
  }

  /* Compute bi-PSI array */
  for(i = 0; i < n; ++i) {
    d = T[i];
    t = C[d]++;
    if(t != pidx) {
      c = T[t - (pidx < t)];
      t = D[(d << 8) | c]++;
      u = i + (pidx <= i);
      PSI[t] = (u << lowshift) | (((c << 8) | d) & mask);
    }
  }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) {
    for(d = 0; d < c; ++d) {
      t = D[(d << 8) | c];
      D[(d << 8) | c] = D[(c << 8) | d];
      D[(c << 8) | d] = t;
    }
  }
  for(i = 1, t = pidx; i < n; i += 2) {
    assert(0 <= t);
    assert(t <= n);
    w = t >> highshift;
    c = B2S[w] & 0xffff;
    len = (1U << (B2S[w] >> 16)) - 1;
    for(half = len >> 1; mask < len; len = half, half >>= 1) {
      if(D[c + half] <= t) { c += half + 1; }
    }
    u = PSI[t];
    c |= u & mask;
    U[i - 1] = (unsigned char)((c >> 8) & 0xff);
    U[i] = (unsigned char)(c & 0xff);
    t = u >> lowshift;
  }
  if(i == n) { U[n - 1] = (unsigned char)lastc; }

  /* Deallocate memory. */
  free(PSI);
  free(D);
  free(B2S);

  return 0;
}

int
obwt_unbwt_biPSIv2_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[257];
  unsigned int *PSI;
  int *D, *LD;
  unsigned int *B2S;
  unsigned int u;
  int i, s, t, v, w, x, pidx;
  int c, d, e, mask, lastc;
  int lowshift, highshift;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((ISAs->numsamples == 1) || (numthreads <= 1)) { return obwt_unbwt_biPSIv2(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4(n+1)+262144+4(2**NUM_TOPBITS) bytes of memory. */
  PSI = (unsigned int *)malloc((n + 1) * sizeof(unsigned int));
  D = (int *)malloc(256 * 256 * sizeof(int));
  B2S = (unsigned int *)malloc((1U << NUM_TOPBITS) * sizeof(unsigned int));
  if((PSI == NULL) || (D == NULL) || (B2S == NULL)) {
    free(PSI); free(D); free(B2S);
    return -2;
  }
#if 0 < NUM_TOPBITS
  B2S[0] = 0;
#endif

  for(i = 0; 0 < (n >> i); ++i) { }
  highshift = (NUM_TOPBITS <= i) ? i - NUM_TOPBITS : 0;
  lowshift = (16 < i) ? (32 - i) : 16;
  mask = (1U << lowshift) - 1;

  /* Compute the cumulative frequency of uni-grams */
  for(c = 0; c < 256; ++c) { C[c] = 0; }
  for(c = 0; c < 256 * 256; ++c) { D[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, i = 1; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
  C[c] = i;

  /* Count the frequency of bi-grams */
  for(c = 0; c < 256; ++c) {
    LD = D + (c << 8);
    for(i = C[c]; i < C[c + 1]; ++i) {
      if(i != pidx) {
        d = T[i - (pidx < i)];
        ++LD[d];
      }
    }
  }

  /* Compute the cumulative frequency of bi-grams */
  lastc = T[0];
  for(c = 0, i = 1, w = 0; c < 256; ++c) {
    if(c == lastc) { i += 1; }
    LD = D + c;
    for(d = 0; d < 256; ++d) {
      t = LD[d << 8];
      if(0 < t) {
        e = (c << 8) | d;
        for(v = i >> highshift; v < w; ++v) {
          B2S[v] = (B2S[v] & 0xffff) | (((unsigned int )e) << 16);
        }
        for(w = ((i + t - 1) >> highshift) + 1; v < w; ++v) {
          B2S[v] = ((unsigned int )e) | (((unsigned int )e) << 16);
        }
      }
      LD[d << 8] = i; i += t;
    }
  }
  for(v = 0; v < w; ++v) {
    c = (B2S[v] >> 16) ^ (B2S[v] & 0xffff);
    for(x = 0; c != 0; ++x, c >>= 1) { }
    B2S[v] = (unsigned int)(((B2S[v] & 0xffff) & (0xffffU << x)) | (x << 16));
  }

  /* Compute bi-PSI array */
  for(i = 0; i < n; ++i) {
    d = T[i];
    t = C[d]++;
    if(t != pidx) {
      c = T[t - (pidx < t)];
      t = D[(d << 8) | c]++;
      u = i + (pidx <= i);
      PSI[t] = (u << lowshift) | (((c << 8) | d) & mask);
    }
  }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) {
    for(d = 0; d < c; ++d) {
      t = D[(d << 8) | c];
      D[(d << 8) | c] = D[(c << 8) | d];
      D[(c << 8) | d] = t;
    }
  }
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, B2S, PSI, D, mask, lastc, lowshift, highshift) private(u, i, s, t, w, c) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    int len, half;
    first = s << ISAs->lgsamplerate;
    last = ((s + 1) < ISAs->numsamples) ? (s + 1) << ISAs->lgsamplerate : n;
    t = ISAs->samples[s];
    for(i = first + 1; i < last; i += 2) {
      assert(0 <= t);
      assert(t <= n);
      w = t >> highshift;
      c = B2S[w] & 0xffff;
      len = (1U << (B2S[w] >> 16)) - 1;
      for(half = len >> 1; mask < len; len = half, half >>= 1) {
        if(D[c + half] <= t) { c += half + 1; }
      }
      u = PSI[t];
      c |= u & mask;
      U[i - 1] = (unsigned char)((c >> 8) & 0xff);
      U[i] = (unsigned char)(c & 0xff);
      t = u >> lowshift;
    }
    if(i == last) {
      if(i != n) {
        w = t >> highshift;
        c = B2S[w] & 0xffff;
        len = (1U << (B2S[w] >> 16)) - 1;
        for(half = len >> 1; mask < len; len = half, half >>= 1) {
          if(D[c + half] <= t) { c += half + 1; }
        }
        u = PSI[t];
        c |= u & mask;
        U[i - 1] = (unsigned char)((c >> 8) & 0xff);
      } else {
        U[n - 1] = (unsigned char)lastc;
      }
    }
  }

  /* Deallocate memory. */
  free(PSI);
  free(D);
  free(B2S);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_biPSIv2(T, U, n, ISAs->samples[0]);
#endif
}

int
obwt_unbwt_biPSIv2_fullomp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  unsigned int *PSI;
  int *C, *LC, *D, *LD;
  unsigned int *B2S;
  unsigned int u;
  int i, s, t, w, pidx;
  int c, d, mask, lastc;
  int lowshift, highshift;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if(numthreads <= 1) { return obwt_unbwt_biPSIv2(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate 4(n+1)+263168numthreads+4(2**NUM_TOPBITS) bytes of memory. */
  PSI = (unsigned int *)malloc((n + 1) * sizeof(unsigned int));
  C = (int *)malloc(256 * numthreads * sizeof(int));
  D = (int *)malloc(256 * 256 * numthreads * sizeof(int));
  B2S = (unsigned int *)malloc((1U << NUM_TOPBITS) * sizeof(unsigned int));
  if((PSI == NULL) || (C == NULL) || (D == NULL) || (B2S == NULL)) {
    free(PSI); free(C); free(D); free(B2S);
    return -2;
  }
#if 0 < NUM_TOPBITS
  B2S[0] = 0;
#endif

  for(i = 0; 0 < (n >> i); ++i) { }
  highshift = (NUM_TOPBITS <= i) ? i - NUM_TOPBITS : 0;
  lowshift = (16 < i) ? (32 - i) : 16;
  mask = (1U << lowshift) - 1;
  lastc = T[0];
#pragma omp parallel default(none) shared(T, n, pidx, numthreads, PSI, C, D, B2S, mask, lastc, lowshift, highshift) private(LC, LD, u, i, t, w, c, d)
  {
    int j, k, th_id = omp_get_thread_num();
    int v, x;
    int e;
    LC = C + th_id * 256;
    LD = D + th_id * 256 * 256;
#   pragma omp single nowait
    numthreads = omp_get_num_threads();

    /* Count the freqiency of uni-grams */
    for(c = 0; c < 256; ++c) { LC[c] = 0; }
#   pragma omp for schedule(static)
    for(i = 0; i < n; ++i) { ++LC[T[i]]; }

    /* Compute the cumulative frequency of uni-grams */
#   pragma omp for schedule(static)
    for(c = 0; c < 256; ++c) {
      i = C[c];
      for(j = 1, k = c + 256; j < numthreads; ++j, k += 256) {
        t = C[k]; C[k] = i; i += t;
      }
      C[c] = i;
    }
#   pragma omp single
    for(c = 0, i = 1; c < 256; ++c) { t = C[c]; C[c] = i; i += t; }
    if(0 < th_id) { for(c = 0; c < 256; ++c) { LC[c] += C[c]; } }

    /* Count the freqiency of bi-grams */
    for(c = 0; c < 256 * 256; ++c) { LD[c] = 0; }
#   pragma omp barrier
#   pragma omp for schedule(static)
    for(i = 0; i < n; ++i) {
      d = T[i];
      t = LC[d]++;
      if(t != pidx) { ++LD[(d << 8) | T[t - (pidx < t)]]; }
    }

    /* Compute the cumulative frequency of bi-grams */
#   pragma omp for schedule(static)
    for(c = 0; c < 256 * 256; ++c) {
      i = D[c];
      for(j = 1, k = c + 256 * 256; j < numthreads; ++j, k += 256 * 256) {
        t = D[k]; D[k] = i; i += t;
      }
      D[c] = i;
    }
#   pragma omp single
    {
      for(c = 0, i = 1, w = 0; c < 256; ++c) {
        if(c == lastc) { i += 1; }
        for(d = 0; d < 256; ++d) {
          t = D[(d << 8) | c];
          if(0 < t) {
            e = (c << 8) | d;
            for(v = i >> highshift; v < w; ++v) {
              B2S[v] = (B2S[v] & 0xffff) | (((unsigned int )e) << 16);
            }
            for(w = ((i + t - 1) >> highshift) + 1; v < w; ++v) {
              B2S[v] = ((unsigned int )e) | (((unsigned int )e) << 16);
            }
          }
          D[(d << 8) | c] = i; i += t;
        }
      }
      for(v = 0; v < w; ++v) {
        c = (B2S[v] >> 16) ^ (B2S[v] & 0xffff);
        for(x = 0; c != 0; ++x, c >>= 1) { }
        B2S[v] = (unsigned int)(((B2S[v] & 0xffff) & (0xffffU << x)) | (x << 16));
      }
    }
    if(0 < th_id) { for(c = 0; c < 256 * 256; ++c) { LD[c] += D[c]; } }

    /* Recompute the cumulative frequency of uni-grams */
#   pragma omp single
    for(c = 0, i = 1; c < 256; ++c) {
      for(j = 0, k = c; j < numthreads; ++j, k += 256) {
        t = C[k]; C[k] = i; i = t;
      }
    }

    /* Compute bi-PSI array */
#   pragma omp for schedule(static)
    for(i = 0; i < n; ++i) {
      d = T[i];
      t = LC[d]++;
      if(t != pidx) {
        c = T[t - (pidx < t)];
        t = LD[(d << 8) | c]++;
        u = i + (pidx <= i);
        PSI[t] = (u << lowshift) | (((c << 8) | d) & mask);
      }
    }
    LD = D + (numthreads - 1) * 256 * 256;
#   pragma omp for schedule(guided) nowait
    for(c = 0; c < 256; ++c) {
      for(d = 0; d < c; ++d) {
        t = LD[(d << 8) | c];
        LD[(d << 8) | c] = LD[(c << 8) | d];
        LD[(c << 8) | d] = t;
      }
    }
  }

  /* Inverse BW-transform. */
  LD = D + (numthreads - 1) * 256 * 256;
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, B2S, PSI, LD, mask, lastc, lowshift, highshift) private(u, i, s, t, w, c) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int first, last;
    int len, half;
    first = s << ISAs->lgsamplerate;
    last = ((s + 1) < ISAs->numsamples) ? (s + 1) << ISAs->lgsamplerate : n;
    t = ISAs->samples[s];
    for(i = first + 1; i < last; i += 2) {
      assert(0 <= t);
      assert(t <= n);
      w = t >> highshift;
      c = B2S[w] & 0xffff;
      len = (1U << (B2S[w] >> 16)) - 1;
      for(half = len >> 1; mask < len; len = half, half >>= 1) {
        if(LD[c + half] <= t) { c += half + 1; }
      }
      u = PSI[t];
      c |= u & mask;
      U[i - 1] = (unsigned char)((c >> 8) & 0xff);
      U[i] = (unsigned char)(c & 0xff);
      t = u >> lowshift;
    }
    if(i == last) {
      if(i != n) {
        w = t >> highshift;
        c = B2S[w] & 0xffff;
        len = (1U << (B2S[w] >> 16)) - 1;
        for(half = len >> 1; mask < len; len = half, half >>= 1) {
          if(LD[c + half] <= t) { c += half + 1; }
        }
        u = PSI[t];
        c |= u & mask;
        U[i - 1] = (unsigned char)((c >> 8) & 0xff);
      } else {
        U[n - 1] = (unsigned char)lastc;
      }
    }
  }

  /* Deallocate memory. */
  free(PSI);
  free(C);
  free(D);
  free(B2S);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_biPSIv2(T, U, n, ISAs->samples[0]);
#endif
}
