/*********************************************************************/
/* Copyright 2009, 2010 The University of Texas at Austin.           */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include <stdio.h>
#include "common.h"

#ifndef CONJ
#define GEMM_KERNEL	GEMM_KERNEL_R
#define GEMM_KERNEL_B0	GEMM_KERNEL_R_B0
#else
#define GEMM_KERNEL	GEMM_KERNEL_L
#define GEMM_KERNEL_B0	GEMM_KERNEL_L_B0
#endif

int CNAME(BLASLONG m, BLASLONG n, BLASLONG k, FLOAT alpha_r,
	   FLOAT *a, FLOAT *b, FLOAT *c, BLASLONG ldc, BLASLONG offset){

  BLASLONG i, j;
  BLASLONG loop;
  FLOAT *cc, *ss;
  FLOAT subbuffer[GEMM_UNROLL_MN * (GEMM_UNROLL_MN + 1) * COMPSIZE];

  if (m + offset < 0) {
#ifndef LOWER
    GEMM_KERNEL(m, n, k,
		alpha_r, ZERO,
		a, b, c, ldc);
#endif
    return 0;
  }

  if (n < offset) {
#ifdef LOWER
    GEMM_KERNEL(m, n, k,
		alpha_r, ZERO,
		a, b, c, ldc);
#endif
    return 0;
  }


  if (offset > 0) {
#ifdef LOWER
    GEMM_KERNEL(m, offset, k,
		alpha_r, ZERO,
		a, b, c, ldc);
#endif
    b += offset * k   * COMPSIZE;
    c += offset * ldc * COMPSIZE;
    n -= offset;
    offset = 0;

    if (n <= 0) return 0;
  }

  if (n > m + offset) {
#ifndef LOWER
      GEMM_KERNEL(m, n - m - offset, k,
		  alpha_r, ZERO,
		  a,
		  b + (m + offset) * k   * COMPSIZE,
		  c + (m + offset) * ldc * COMPSIZE, ldc);
#endif

    n = m + offset;
    if (n <= 0) return 0;
  }


  if (offset < 0) {
#ifndef LOWER
    GEMM_KERNEL(-offset, n, k,
		alpha_r, ZERO,
		a, b, c, ldc);
#endif
    a -= offset * k   * COMPSIZE;
    c -= offset       * COMPSIZE;
    m += offset;
    offset = 0;

  if (m <= 0) return 0;
  }

  if (m > n - offset) {
#ifdef LOWER
    GEMM_KERNEL(m - n + offset, n, k,
		alpha_r, ZERO,
		a + (n - offset) * k * COMPSIZE,
		b,
		c + (n - offset)     * COMPSIZE, ldc);
#endif
    m = n + offset;
  if (m <= 0) return 0;
  }

  for (loop = 0; loop < n; loop += GEMM_UNROLL_MN) {

    int mm, nn;

    mm = (loop & ~(GEMM_UNROLL_MN - 1));
    nn = MIN(GEMM_UNROLL_MN, n - loop);

#ifndef LOWER
    GEMM_KERNEL(mm, nn, k,
		alpha_r, ZERO,
		a, b + loop * k * COMPSIZE, c + loop * ldc * COMPSIZE, ldc);
#endif

    GEMM_BETA(nn, nn, 0, ZERO,  ZERO,
	      NULL, 0, NULL, 0, subbuffer, nn);

    GEMM_KERNEL(nn, nn, k,
		alpha_r, ZERO,
		a + loop * k * COMPSIZE, b + loop * k * COMPSIZE, subbuffer, nn);

    cc = c + (loop + loop * ldc) * COMPSIZE;
    ss = subbuffer;

#ifndef LOWER
 for (j = 0; j < nn; j ++) {

    for (i = 0; i <j; i ++) {
      cc[i * 2 + 0] += ss[i * 2 + 0];
      cc[i * 2 + 1] += ss[i * 2 + 1];
    }

    cc[j * 2 + 0] += ss[i * 2 + 0];
    cc[j * 2 + 1]  = ZERO;

    ss += nn  * COMPSIZE;
    cc += ldc * COMPSIZE;
  }

#else
  for (j = 0; j < nn; j ++) {

    cc[j * 2 + 0] += ss[j * 2 + 0];
    cc[j * 2 + 1]  = ZERO;

    for (i = j + 1; i < nn; i ++) {
      cc[i * 2 + 0] += ss[i * 2 + 0];
      cc[i * 2 + 1] += ss[i * 2 + 1];
    }
    ss += nn  * COMPSIZE;
    cc += ldc * COMPSIZE;
  }
#endif

#ifdef LOWER
    GEMM_KERNEL(m - mm - nn, nn, k,
		alpha_r, ZERO,
		a + (mm + nn) * k * COMPSIZE, b + loop * k * COMPSIZE,
		c + (mm + nn + loop * ldc) * COMPSIZE, ldc);
#endif

  }

    return 0;
}
