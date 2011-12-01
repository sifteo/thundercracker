OpenBWT version 2.0.0

== Introduction ==
OpenBWT is a small open source library for the Burrows-Wheeler transformation.
It currently provides a linear-time BWT/BWTS, inverse BWT/BWTS and several
second stage transform algorithms for BWT-based compression.

== License ==
OpenBWT is licensed under the MIT license. See the file COPYING.txt for details.

== C Interfaces ==

/*
  Burrows-Wheeler Transform
  @param T[0..n-1] The input string.
  @param U[0..n-1] The output BWTed-string.
  @param A[0..n-1] The temporary array.
  @param n The length of the string.
  @return The primary index if no error occurred, -1 or -2 otherwise.
*/
int
obwt_bwt(const unsigned char *T, unsigned char *U, int *A, int n);

/*
  Inverse Burrows-Wheeler Transform
  @param T[0..n-1] The input BWTed-string.
  @param U[0..n-1] The output string. (can be T)
  @param n The length of the string.
  @param pidx The primary index.
  @return 0 if no error occurred, -1 or -2 otherwise.
*/
int
obwt_unbwt_indexPSIv2(const unsigned char *T, unsigned char *U, int n, int pidx);

See the file openbwt.h for more details.
