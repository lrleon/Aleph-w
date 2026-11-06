

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

/*    This program by D E Knuth is in the public domain and freely copyable
 *    AS LONG AS YOU MAKE ABSOLUTELY NO CHANGES!
 *    It is explained in Seminumerical Algorithms, 3rd edition, Section 3.6
 *    (or in the errata to the 2nd edition --- see
 *        http://www-cs-faculty.stanford.edu/~knuth/taocp.html
 *    in the changes to Volume 2 on pages 171 and following).              */

/*    N.B. The MODIFICATIONS introduced in the 9th printing (2002) are
      included here; there's no backwards compatibility with the original. */

/*    This version also adopts Brendan McKay's suggestion to
      accommodate naive users who forget to call ran_start(seed).          */

/*    If you find any bugs, please report them immediately to
 *                 taocp@cs.stanford.edu
 *    (and you will be rewarded if the bug is genuine). Thanks!            */

/************ see the book for explanations and caveats! *******************/
/************ in particular, you need two's complement arithmetic **********/

/* the old C calling conventions are used here, for reasons of portability */

/** @file ran_array.h
    @brief Knuth's random number generator, from his `rng.c` (The Art of
    Computer Programming, Volume 2, Section 3.6), which `ran_array.c`
    compiles into Aleph.

    After `ran_start(seed)`, `ran_arr_next()` gives the numbers, in
    [0, 2^30).

    @deprecated Nothing of Aleph uses it: `ran_array.h` and `ran_array.c`
    go in 8.0.0. Use the engines of `<random>` instead.
*/

# ifdef  __cplusplus
#   define ALEPH_RAN_ARRAY_DEPRECATED \
      [[deprecated("ran_array.h goes in 8.0.0; use the engines of <random> instead")]]
extern "C" {
# else
#   define ALEPH_RAN_ARRAY_DEPRECATED
# endif

/** Starts the generator with `seed`: do this before using it.

    @deprecated `ran_array.h` goes in 8.0.0. */
ALEPH_RAN_ARRAY_DEPRECATED extern void ran_start(long seed);

/* the following routines are from exercise 3.6--15 */
/* after calling ran_start, get new randoms by, e.g., "x=ran_arr_next()" */

/** Points to the next number of the batch, or to -1 when the batch has
    run out.

    @deprecated `ran_array.h` goes in 8.0.0. */
ALEPH_RAN_ARRAY_DEPRECATED extern long *ran_arr_ptr;

/** Makes a new batch of numbers and returns its first one:
    `ran_arr_next()` calls it when the batch runs out. Without
    `ran_start()`, it first starts the generator as `ran_start(314159)`.

    @deprecated `ran_array.h` goes in 8.0.0. */
ALEPH_RAN_ARRAY_DEPRECATED extern long ran_arr_cycle(void);

/** The next random number.

    @deprecated `ran_array.h` goes in 8.0.0. */
#define ran_arr_next() (*ran_arr_ptr >=0 ? *ran_arr_ptr++ : ran_arr_cycle())

# ifdef  __cplusplus
}
# endif

# undef ALEPH_RAN_ARRAY_DEPRECATED
