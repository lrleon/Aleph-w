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

# ifdef  __cplusplus
extern "C" {
# endif



/* 
   do this before using ran_array 
*/
extern void ran_start(long seed);


/* the following routines are from exercise 3.6--15 */
/* after calling ran_start, get new randoms by, e.g., "x=ran_arr_next()" */

extern long *ran_arr_ptr; /* the next random number, or -1 */

extern long ran_arr_cycle(void);
       
#define ran_arr_next() (*ran_arr_ptr >=0 ? *ran_arr_ptr++ : ran_arr_cycle())

# ifdef  __cplusplus
}
# endif





