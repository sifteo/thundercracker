/*
 * LRI.c for the OpenBWT project
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
  LR-I algorithm
    space: n+4n/b+nlgb/8+4*257+4*256 bytes
     time: O(n lg(n/b))

  Reference:
    Juha Kärkkäinen and Simon Puglisi,
    "Medium-Space Algorithms for Inverse BWT",
    Proceedings of the 18th European Symposium on Algorithms, LNCS 6346,
    Springer-Verlag, pp. 451-462, 2010.
*/

#define LRI_LGB 4

/*
  log b   space (in bytes, excluding T and U)
    0     5.0000n
    1     3.1250n
    2     2.2500n
    3     1.8750n
    4     1.7500n
    5     1.7500n
    6     1.8125n
    7     1.9062n
    8     2.0156n
    9     2.1328n
   10     2.2539n
   11     2.3770n
   12     2.5010n
   13     2.6255n
   14     2.7502n
   15     2.8751n
   16     3.0001n
   17     3.1250n
   18     3.2500n
   19     3.3750n
   20     3.5000n
   21     3.6250n
   22     3.7500n
   23     3.8750n
   24     4.0000n
   25     4.1250n
   26     4.2500n
   27     4.3750n
   28     4.5000n
   29     4.6250n
   30     4.7500n
   31     4.8750n
   32     5.0000n
*/

#if LRI_LGB > 32
# undef LRI_LGB
# define LRI_LGB 32
#elif LRI_LGB < 0
# undef LRI_LGB
# define LRI_LGB 0
#endif
#define LRI_MASK ((1U << LRI_LGB) - 1)
#if LRI_LGB == 4
# define LRI_SIZE (n / 8 * LRI_LGB + (n % 8 * LRI_LGB + 7) / 8)
# define LRI_RTYPE unsigned char
# define LRI_GETVAL(_i) \
    (((_i) & 1) ? ((R[(_i) >> 1] >> 4) & 0x0f) : (R[(_i) >> 1] & 0x0f))
# define LRI_SETVAL(_i, _v) \
  do { \
    if((_i) & 1) { R[(_i) >> 1] |= (unsigned char)(((_v) & 0x0f) << 4); } \
    else { R[(_i) >> 1] = (unsigned char)((_v) & 0x0f); } \
  } while(0)
#elif LRI_LGB == 8
# define LRI_SIZE (n / 8 * LRI_LGB + (n % 8 * LRI_LGB + 7) / 8)
# define LRI_RTYPE unsigned char
# define LRI_GETVAL(_i) (R[(_i)] & 0xff)
# define LRI_SETVAL(_i, _v) do { R[(_i)] = (_v); } while(0)
#elif LRI_LGB == 16
# define LRI_SIZE (n / 16 * LRI_LGB + (n % 16 * LRI_LGB + 15) / 16)
# define LRI_RTYPE unsigned short
# define LRI_GETVAL(_i) (R[(_i)] & 0xffff)
# define LRI_SETVAL(_i, _v) do { R[(_i)] = (_v); } while(0)
#else
# define LRI_SIZE (n / 32 * LRI_LGB + (n % 32 * LRI_LGB + 31) / 32)
# define LRI_RTYPE unsigned int
# define LRI_GETVAL(_i) lri_getval(R, (_i))
# define LRI_SETVAL(_i, _v) lri_setval(R, (_i), (_v))
static const unsigned int
lri_mask_table[33] = {
  0x00000000U, 0x00000001U, 0x00000003U, 0x00000007U,
  0x0000000fU, 0x0000001fU, 0x0000003fU, 0x0000007fU,
  0x000000ffU, 0x000001ffU, 0x000003ffU, 0x000007ffU,
  0x00000fffU, 0x00001fffU, 0x00003fffU, 0x00007fffU,
  0x0000ffffU, 0x0001ffffU, 0x0003ffffU, 0x0007ffffU,
  0x000fffffU, 0x001fffffU, 0x003fffffU, 0x007fffffU,
  0x00ffffffU, 0x01ffffffU, 0x03ffffffU, 0x07ffffffU,
  0x0fffffffU, 0x1fffffffU, 0x3fffffffU, 0x7fffffffU,
  0xffffffffU
};
static INLINE
unsigned int
lri_getval(const unsigned int *R, unsigned int i) {
  unsigned int val;
  unsigned int shift, len, off;
  R += (i >> 5) * LRI_LGB + (((i & 31) * LRI_LGB) >> 5);
  off = ((i & 31) * LRI_LGB) & 31;
  len = LRI_LGB;
  if((32 - off) < len) {
    val = *R >> off;
    len -= 32 - off, shift = 32 - off, ++R;
    for(; 32 < len; len -= 32, shift += 32, ++R) { val |= *R << shift; }
    val |= (*R & lri_mask_table[len]) << shift;
  } else {
    val = (*R >> off) & lri_mask_table[len];
  }
  return val;
}
static INLINE
void
lri_setval(unsigned int *R, unsigned int i, unsigned int val) {
  unsigned int off, len;
  R += (i >> 5) * LRI_LGB + (((i & 31) * LRI_LGB) >> 5);
  off = ((i & 31) * LRI_LGB) & 31;
  len = LRI_LGB;
  if((32 - off) < len) {
    *R = (*R & lri_mask_table[off]) | ((val & lri_mask_table[32 - off]) << off);
    val >>= 32 - off, len -= 32 - off, ++R;
    *R = (*R & ~lri_mask_table[len]) | (val & lri_mask_table[len]);
  } else {
    *R = (*R & ~(lri_mask_table[len] << off)) | ((val & lri_mask_table[len]) << off);
  }
}
#endif

int
obwt_unbwt_LRI(const unsigned char *T, unsigned char *U, int n, int pidx) {
  int C[257];
  int D[256];
  int *S, *Sc;
  LRI_RTYPE *R;
  unsigned char *V;
  int i, p, t, len, half;
  int c;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate n+4n/b+nlgb/8 bytes of memory. */
  R = (LRI_RTYPE *)malloc(LRI_SIZE * sizeof(LRI_RTYPE));
  V = (unsigned char *)malloc(n * sizeof(unsigned char));
#if LRI_LGB < 32
  S = (int *)malloc((n >> LRI_LGB) * sizeof(int));
#else
  S = (int *)malloc(1 * sizeof(int));
#endif
  if((R == NULL) || (V == NULL) || (S == NULL)) { free(R); free(V); free(S); return -2; }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0; i < n; ++i) {
    c = V[i] = T[i];
    LRI_SETVAL(i, C[c]); C[c]++;
  }
  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c];
    C[c] = i;
#if LRI_LGB < 32
    if(0 < t) { i += (t - 1) >> LRI_LGB; }
#endif
  }
  C[c] = i;
  for(i = 0; i < n; ++i) {
    c = T[i];
    t = D[c]++;
#if LRI_LGB < 32
    if((t & LRI_MASK) == 0) {
      t >>= LRI_LGB;
      if(0 < t) { S[C[c] + t - 1] = i; }
    }
#endif
  }
  for(c = 0, i = 0; c < 256; ++c) { t = D[c]; D[c] = i; i += t; }
  for(i = n - 1, t = 0; 0 <= i; --i) {
    c = V[t];
    U[i] = (unsigned char)c;
    Sc = S + C[c];
    for(p = 0, len = C[c + 1] - C[c], half = len >> 1;
        0 < len;
        len = half, half >>= 1) {
      if(Sc[p + half] <= t) { p += half + 1; half -= (len & 1) ^ 1; }
    }
#if LRI_LGB < 32
    t = (p << LRI_LGB) + LRI_GETVAL(t) + D[c];
#else
    t = LRI_GETVAL(t) + D[c];
#endif
    if(t < pidx) { ++t; }
  }

  /* Deallocate memory. */
  free(S);
  free(R);
  free(V);

  return 0;
}

int
obwt_unbwt_LRI_omp(const obwt_ISAs_t *ISAs, const unsigned char *T, unsigned char *U, int n, int numthreads) {
#ifdef _OPENMP
  int C[257];
  int D[256];
  int *S;
  LRI_RTYPE *R;
  unsigned char *V;
  int i, p, s, t, pidx;
  int c;

  /* Check arguments. */
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  pidx = ISAs->samples[0];
  if((numthreads <= 1) || (ISAs->numsamples == 1)) { return obwt_unbwt_LRI(T, U, n, pidx); }
  if((T == NULL) || (U == NULL) || (n < 0) || (pidx < 0) ||
     (n < pidx) || ((0 < n) && (pidx == 0))) { return -1; }
  if(n <= 1) { if(T != U) { U[0] = T[0]; } return 0; }

  /* Allocate n+4n/b+nlgb/8 bytes of memory. */
  R = (LRI_RTYPE *)malloc(LRI_SIZE * sizeof(LRI_RTYPE));
  V = (unsigned char *)malloc(n * sizeof(unsigned char));
#if LRI_LGB < 32
  S = (int *)malloc((n >> LRI_LGB) * sizeof(int));
#else
  S = (int *)malloc(1 * sizeof(int));
#endif
  if((R == NULL) || (V == NULL) || (S == NULL)) { free(R); free(V); free(S); return -2; }

  /* Inverse BW-transform. */
  for(c = 0; c < 256; ++c) { C[c] = D[c] = 0; }
  for(i = 0; i < n; ++i) {
    c = V[i] = T[i];
    LRI_SETVAL(i, C[c]); C[c]++;
  }
  for(c = 0, i = 0; c < 256; ++c) {
    t = C[c];
    C[c] = i;
#if LRI_LGB < 32
    if(0 < t) { i += (t - 1) >> LRI_LGB; }
#endif
  }
  C[c] = i;
  for(i = 0; i < n; ++i) {
    c = T[i];
    t = D[c]++;
#if LRI_LGB < 32
    if((t & LRI_MASK) == 0) {
      t >>= LRI_LGB;
      if(0 < t) { S[C[c] + t - 1] = i; }
    }
#endif
  }
  for(c = 0, i = 0; c < 256; ++c) { t = D[c]; D[c] = i; i += t; }
# pragma omp parallel for schedule(guided) default(none) shared(ISAs, U, n, C, D, S, R, V, pidx) private(i, p, s, t, c) num_threads(numthreads)
  for(s = 0; s < ISAs->numsamples; ++s) {
    int *LS;
    int first, last;
    int len, half;
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
      c = V[t];
      U[i] = (unsigned char)c;
      LS = S + C[c];
      for(p = 0, len = C[c + 1] - C[c], half = len >> 1;
          0 < len;
          len = half, half >>= 1) {
        if(LS[p + half] <= t) { p += half + 1; half -= (len & 1) ^ 1; }
      }
  #if LRI_LGB < 32
      t = (p << LRI_LGB) + LRI_GETVAL(t) + D[c];
  #else
      t = LRI_GETVAL(t) + D[c];
  #endif
      if(t < pidx) { ++t; }
    }
  }

  /* Deallocate memory. */
  free(S);
  free(R);
  free(V);

  return 0;
#else
  (void)numthreads;
  if((ISAs == NULL) || (ISAs->samples == NULL) || (ISAs->numsamples <= 0)) { return -1; }
  return obwt_unbwt_LRI(T, U, n, ISAs->samples[0]);
#endif
}
