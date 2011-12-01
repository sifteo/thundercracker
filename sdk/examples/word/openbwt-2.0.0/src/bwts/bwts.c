/*
 * bwts.c for the OpenBWT project
 * Copyright (c) 2008-2010 Yuta Mori. All Rights Reserved.
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
#include "openbwt.h"

#ifdef __INTEL_COMPILER
#pragma warning(disable : 2259)
#endif


#define chr(i) (cs == sizeof(int) ? ((const int *)T)[i]:((const unsigned char *)T)[i])
#define isLMS(_a) ((bvec_get(F1, (_a)) != 0) || ((bvec_get(F2, (_a)) != 0) && (bvec_get(F2, (_a) - 1) == 0)))

static
int
bvec_get(const unsigned char *B, int i) {
  return (B[i >> 3] >> (i & 7)) & 1;
}

static
void
bvec_set(unsigned char *B, int i, int b) {
  if(b) { B[i >> 3] |= (unsigned char)(  1U << (i & 7)); }
  else  { B[i >> 3] &= (unsigned char)(~(1U << (i & 7))); }
}

static
int
bvec_next(const unsigned char *B, int i) {
  int j;
  unsigned int c;
  i += 1;
  j = i >> 3;
  c = B[j] >> (i & 7);
  if(c == 0) {
    i += 8 - (i & 7);
    j += 1;
    for(; (c = B[j]) == 0; ++j, i += 8) { }
  }
  for(; (c & 1) == 0; ++i, c >>= 1) { }
  return i;
}

static
int
bvec_prev(const unsigned char *B, int i) {
  int j;
  unsigned int c;
  i -= 1;
  if(0 <= i) {
    j = i >> 3;
    c = (B[j] << (7 - (i & 7))) & 0xff;
    if(c == 0) {
      i -= (i & 7) + 1;
      j -= 1;
      for(; (0 <= j) && ((c = B[j]) == 0); --j, i -= 8) { }
      if(c == 0) { c = 128; }
    }
    for(; (c & 128) == 0; --i, c <<= 1) { }
  }
  return i;
}

/* find the start or end of each bucket */
static
void
getCounts(const unsigned char *T, int *C, int n, int k, int cs) {
  int i;
  for(i = 0; i < k; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { ++C[chr(i)]; }
}
static
void
getBuckets(const int *C, int *B, int k, int end) {
  int i, sum = 0;
  if(end) { for(i = 0; i < k; ++i) { sum += C[i]; B[i] = sum; } }
  else { for(i = 0; i < k; ++i) { sum += C[i]; B[i] = sum - C[i]; } }
}

/* compute SA */
static
void
induceSA(const unsigned char *T, int *SA, int *C, int *B,
         const unsigned char *F1, const unsigned char *F2, const unsigned char *F3,
         int n, int k, int cs) {
  int i, j, p;

  /* compute SAl */
  if(C == B) { getCounts(T, C, n, k, cs); }
  getBuckets(C, B, k, 0); /* find starts of buckets */
  p = bvec_prev(F3, n);
  for(i = 0; i < n; ++i) {
    while((0 <= p) && (B[chr(p)] == i)) { /* unique LMS suffix */
      j = bvec_next(F1, p) - 1;
      SA[B[chr(j)]++] = j;
      p = bvec_prev(F3, p);
    }
    j = SA[i]; if(j < 0) { continue; }
    if(bvec_get(F1, j) == 0) { j -= 1; }
    else { j = bvec_next(F1, j) - 1; }
    if(bvec_get(F2, j) == 0) { SA[B[chr(j)]++] = j; }
  }

  /* compute SAs */
  if(C == B) { getCounts(T, C, n, k, cs); }
  getBuckets(C, B, k, 1); /* find ends of buckets */
  for(i = n - 1; 0 <= i; --i) {
    j = SA[i]; if(j < 0) { continue; }
    if(bvec_get(F1, j) == 0) {
      --j;
      if(bvec_get(F2, j) != 0) { SA[--B[chr(j)]] = j; }
    }
  }
}

static
int
lw_suffixsort(const unsigned char *T, int *SA, const unsigned char *F1,
              int fs, int n, int k, int cs) {
/*
F1: start position of each lyndon word
F2: type L(0) or S(1)
F3: unique LMS suffix
*/
  unsigned char *F2, *F3, *F4;
  int *C, *B, *RA;
  int i, j, c, m, p, q, plen, qlen, name;
  int c0, c1;
  int diff;

  F2 = (unsigned char *)calloc((n + 7) / 8, sizeof(unsigned char));
  F3 = (unsigned char *)calloc((n / 8 + 1), sizeof(unsigned char));
  if((F2 == NULL) || (F3 == NULL)) { free(F2); free(F3); return -2; }
  for(p = 0; p < n; p = q) {
    q = bvec_next(F1, p);
    for(i = q - 2, c = 0, c1 = chr(q - 1), m = 0; p <= i; --i, c1 = c0) {
      bvec_set(F2, i + 1, c);
      if((c0 = chr(i)) < (c1 + c)) { c = 1; }
      else if(c != 0) { m += 1; c = 0; }
    }
    bvec_set(F2, p, 1);
    if((m == 0) && (c == 0)) { bvec_set(F3, p, 1); } /* unique LMS suffix */
  }
  bvec_set(F3, n, 1);

  /* stage 1: reduce the problem by at least 1/2
     sort all the S-substrings */
  if(k <= fs) { C = SA + n; B = (k <= (fs - k)) ? C + k : C; }
  else if((C = B = (int *)malloc(k * sizeof(int))) == NULL) { free(F2); free(F3); return -2; }
  getCounts(T, C, n, k, cs); getBuckets(C, B, k, 1);
  for(i = 0; i < n; ++i) { SA[i] = -1; }
  for(i = 0; i < n; ++i) {
    if(isLMS(i) && (bvec_get(F3, i) == 0)) { SA[--B[chr(i)]] = i; }
  }
  induceSA(T, SA, C, B, F1, F2, F3, n, k, cs);
  if(fs < k) { free(C); }

  /* compact all the sorted substrings into the first m items of SA
     2*m must be not larger than n (proveable) */
  for(i = 0, m = 0; i < n; ++i) {
    p = SA[i];
    if(isLMS(p) && (bvec_get(F3, p) == 0)) { SA[m++] = p; }
  }
  for(i = m; i < n; ++i) { SA[i] = 0; } /* init the name array buffer */
  /* store the length of all substrings */
  for(p = 0; p < n; p = q) {
    q = bvec_next(F1, p);
    for(i = q - 2, c = 0, c1 = chr(q - 1), j = q; p <= i; --i, c1 = c0) {
      if((c0 = chr(i)) < (c1 + c)) { c = 1; }
      else if(c != 0) {
        SA[m + ((i + 1) >> 1)] = j - i - 1;
        j = i + 1;
        c = 0;
      }
    }
    if((j < q) || (c != 0)) { SA[m + (p >> 1)] = j - p; }
  }
  /* find the lexicographic names of all substrings */
  for(i = 0, name = 0, q = n, qlen = 0; i < m; ++i) {
    p = SA[i], plen = SA[m + (p >> 1)], diff = 1;
    if(plen == qlen) {
      for(j = 0; (j < plen) && (chr(p + j) == chr(q + j)); j++) { }
      if(j == plen) { diff = 0; }
    }
    if(diff != 0) { ++name, q = p, qlen = plen; }
    SA[m + (p >> 1)] = name;
  }

  /* stage 2: solve the reduced problem
     recurse if names are not yet unique */
  if(name < m) {
    F4 = (unsigned char *)calloc((m / 8 + 1), sizeof(unsigned char));
    if(F4 == NULL) { free(F2); free(F3); return -1; }
    for(i = 0, j = 0; i < n; ++i) {
      if(isLMS(i) && (bvec_get(F3, i) == 0)) {
        bvec_set(F4, j++, bvec_get(F1, i));
      }
    }
    bvec_set(F4, m, 1);
    RA = SA + n + fs - m;
    for(i = n - 1, j = m - 1; m <= i; --i) {
      if(SA[i] != 0) { RA[j--] = SA[i] - 1; }
    }
    if(lw_suffixsort((unsigned char *)RA, SA, F4, fs + n - m * 2, m, name, sizeof(int)) != 0) {
      free(F2); free(F3); free(F4);
      return -2;
    }
    free(F4);
    for(i = 0, j = 0; i < n; ++i) {
      if(isLMS(i) && (bvec_get(F3, i) == 0)) { RA[j++] = i; }
    }
    for(i = 0; i < m; ++i) { SA[i] = RA[SA[i]]; }
  }

  /* stage 3: induce the result for the original problem */
  if(k <= fs) { C = SA + n; B = (k <= (fs - k)) ? C + k : C; }
  else if((C = B = (int *)malloc(k * sizeof(int))) == NULL) { free(F2); free(F3); return -2; }
  getCounts(T, C, n, k, cs); getBuckets(C, B, k, 1);
  for(i = m; i < n; ++i) { SA[i] = -1; }
  for(i = m - 1; 0 <= i; --i) {
    j = SA[i], SA[i] = -1;
    SA[--B[chr(j)]] = j;
  }
  induceSA(T, SA, C, B, F1, F2, F3, n, k, cs);
  if(fs < k) { free(C); }

  free(F2); free(F3);
  return 0;
}

int
obwt_bwts(const unsigned char *T, unsigned char *U, int *A, int n) {
  unsigned char *F;
  int i, j, k, p, q;
  int c1, c2;

  if((T == NULL) || (U == NULL) || (A == NULL) || (n < 0)) { return -1; }
  if(n <= 1) { if(n == 1) { U[0] = T[0]; } return 0; }

  /* find split positions (Duval's algorithm) */
  F = (unsigned char *)calloc((n / 8 + 1), sizeof(unsigned char));
  if(F == NULL) { return -2; }
  for(i = 0, j = k = 1; j <= n;) {
    for(p = i, q = j;;) {
      c1 = (p < n) ? T[p] : -1;
      c2 = (q < n) ? T[q] : -1;
      ++p, ++q;
      if(c1 != c2) { break; }
      if(p == j) { k = q; p = i; }
    }
    if(c1 < c2) { j = k = q; }
    else { bvec_set(F, i, 1); i = k; j = (k += 1); }
  }
  bvec_set(F, n, 1);

  /* linear-time suffix sorting for lyndon-words */
  if(lw_suffixsort(T, A, F, 0, n, 256, sizeof(unsigned char)) != 0) { free(A); free(F); return -2; }

  /* transform */
  if(T == U) {
    for(i = 0; i < n; ++i) {
      p = A[i];
      if(bvec_get(F, p) == 0) { p -= 1; }
      else { p = bvec_next(F, p) - 1; }
      c1 = T[i];
      U[i] = (unsigned char)((i <= p) ? T[p] : A[p]);
      A[i] = c1;
    }
  } else if(((T < U) && (U < (T + n))) || ((U < T) && (T < (U + n)))) {
    for(i = 0; i < n; ++i) {
      p = A[i];
      if(bvec_get(F, p) == 0) { p -= 1; }
      else { p = bvec_next(F, p) - 1; }
      A[i] = T[p];
    }
    for(i = 0; i < n; ++i) { U[i] = (unsigned char)A[i]; }
  } else {
    for(i = 0; i < n; ++i) {
      p = A[i];
      if(bvec_get(F, p) == 0) { p -= 1; }
      else { p = bvec_next(F, p) - 1; }
      U[i] = T[p];
    }
  }

  free(F);

  return 0;
}


int
obwt_unbwts(const unsigned char *T, unsigned char *U, int n) {
  int C[256];
  int *LF;
  unsigned char *V;
  int i, j, p, t;

  if((T == NULL) || (U == NULL) || (n < 0)) { return -1; }
  if(n <= 1) { if(n == 1) { U[0] = T[0]; } return 0; }

  /* Allocate 5n bytes of memory. */
  LF = (int *)malloc(n * sizeof(int));
  V = (unsigned char *)malloc(n * sizeof(unsigned char));
  if((LF == NULL) || (V == NULL)) { free(LF); free(V); return -2; }

  /* Inverse BWTS. */
  for(i = 0; i < 256; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { C[V[i] = T[i]] += 1; }
  for(i = 0, j = 0; i < 256; ++i) { t = C[i]; C[i] = j; j += t; }
  for(i = 0; i < n; ++i) { LF[i] = C[V[i]]++; }
  for(i = 0, j = n - 1; 0 <= j; ++i) {
    if(0 <= LF[i]) {
      p = i;
      do {
        U[j--] = V[p];
        t = LF[p];
        LF[p] = -1;
        p = t;
      } while(0 <= LF[p]);
    }
  }

  /* Deallocate memory. */
  free(LF);
  free(V);

  return 0;
}
