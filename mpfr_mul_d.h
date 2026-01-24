

/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// a few handy routines that aren't in the mpfr distribution

#include "mpfr.h"

#ifdef __cplusplus
extern "C" {
#endif

int mpfr_mul_d(mpfr_ptr z, mpfr_srcptr x, double a, mpfr_rnd_t);
int mpfr_div_d(mpfr_ptr z, mpfr_srcptr x, double a, mpfr_rnd_t);
int mpfr_d_div(mpfr_ptr z, double a, mpfr_srcptr x, mpfr_rnd_t);
int mpfr_add_d(mpfr_ptr z, mpfr_srcptr x, double a, mpfr_rnd_t);
int mpfr_sub_d(mpfr_ptr z, mpfr_srcptr x, double a, mpfr_rnd_t);
int mpfr_d_sub(mpfr_ptr z, double a, mpfr_srcptr x, mpfr_rnd_t);
void mpfr_mul_d_clear();

#ifdef __cplusplus
} // end extern "C" block
#endif
