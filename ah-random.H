
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

/** @file ah-random.H
 *  @brief A random number generator that gives the numbers of the
 *  `gsl_rng_mt19937` of GSL, without GSL.
 *
 *  Gsl_Compatible_Rng is the Mersenne Twister of the standard library,
 *  `std::mt19937`, which is the algorithm of `gsl_rng_mt19937` and whose
 *  output the standard fixes, seeded as GSL seeds it. Its draws follow
 *  the algorithms of GSL. So, for each seed, its integers and the reals of
 *  uniform() and uniform_pos() are those of GSL, bit by bit, on every
 *  platform, and a structure or a test that took its numbers from GSL
 *  takes the same ones from it.
 *
 *  flat(), gaussian() and exponential() take the logarithm and the square
 *  root of the C library, as GSL does; flat() and gaussian() round each
 *  product, so that a fused multiply-add of the compiler cannot change
 *  their draws, which do not depend on the options with which a program is
 *  built. On a platform, they are those of GSL where GSL rounds them too,
 *  as in the builds of the distributions for x86-64; where GSL is built
 *  with fused multiply-adds, as for arm64, a draw of flat() or gaussian()
 *  may differ from that of GSL in its last bits.
 *
 *  @ingroup Utilities
 */

# ifndef AH_RANDOM_H
# define AH_RANDOM_H

# include <atomic>
# include <cmath>
# include <cstdint>
# include <cstdlib>
# include <memory>
# include <new>
# include <random>
# include <type_traits>
# include <utility>
# include <ah-errors.H>

/** @brief Whether the deprecated accessors to a `gsl_rng` of the
 *  randomized structures exist.
 *
 *  It is 1 if the headers of GSL can be included, unless it is defined
 *  before, as 0 to leave the accessors out. While it is 1, a structure
 *  whose `gsl_rng_object()` is called gives a `gsl_rng` whose numbers
 *  come from its Gsl_Compatible_Rng.
 */
# ifndef ALEPH_WITH_GSL
#   if __has_include(<gsl/gsl_rng.h>)
#     define ALEPH_WITH_GSL 1
#   else
#     define ALEPH_WITH_GSL 0
#   endif
# endif

# if ALEPH_WITH_GSL
#   include <gsl/gsl_rng.h>
# endif

namespace Aleph
{
  /** @brief A random number generator with the sequences of GSL's
   *  `gsl_rng_mt19937`.
   *
   *  For each seed, the numbers and the draws are those of the functions
   *  of GSL named on each member, bit by bit, but for what the
   *  documentation of the file says of flat() and gaussian():
   *
   *  | Member | GSL |
   *  |---|---|
   *  | Gsl_Compatible_Rng(), seed(s) | `gsl_rng_alloc(gsl_rng_mt19937)`, `gsl_rng_set(r, s)` |
   *  | operator()(), get() | `gsl_rng_get(r)` |
   *  | uniform_int(n) | `gsl_rng_uniform_int(r, n)` |
   *  | uniform() | `gsl_rng_uniform(r)` |
   *  | uniform_pos() | `gsl_rng_uniform_pos(r)` |
   *  | flat(a, b) | `gsl_ran_flat(r, a, b)` |
   *  | gaussian(sigma) | `gsl_ran_gaussian(r, sigma)` |
   *  | exponential(mu) | `gsl_ran_exponential(r, mu)` |
   *
   *  It satisfies `std::uniform_random_bit_generator`, so that it can feed
   *  the distributions and algorithms of the standard library too.
   *
   *  The numbers come from inline code, without the call through a
   *  pointer to a function that GSL makes for each one.
   *
   *  @note It holds the state of the Mersenne Twister, 624 words of
   *        `std::uint_fast32_t`: about 2500 or 5000 bytes, as the standard
   *        library goes. A structure that holds one may keep it in dynamic
   *        memory.
   *
   *  @ingroup Utilities
   */
  class Gsl_Compatible_Rng
  {
    std::mt19937 engine;

    // The seed of the Mersenne Twister for a seed of GSL: 4357 for 0, and
    // else its low 32 bits, which may be 0
    static std::uint32_t engine_seed(const unsigned long s) noexcept
    {
      return s == 0 ? default_seed : static_cast<std::uint32_t>(s & 0xffffffffUL);
    }

    // The error of uniform_int(), out of line, so that a draw does not pay
    // for the frame of its message
    [[noreturn, gnu::noinline, gnu::cold]] static void throw_bad_range(const unsigned long n)
    {
      ah_domain_error() << "Gsl_Compatible_Rng::uniform_int(): n is " << n
                        << ", outside [1, " << max() << "]";
    }

    // A product rounded to double: through a volatile, the compiler cannot
    // fuse it with an addition into a multiply-add, whose single rounding
    // would change a draw, and so how many numbers a rejection takes
    static double rounded(const double product) noexcept
    {
      volatile double v = product;
      return v;
    }

  public:
    /// The type of the numbers of operator()().
    using result_type = std::uint32_t;

    /// The seed of a new generator, as GSL takes 0.
    static constexpr std::uint32_t default_seed = 4357;

    /// The least number of operator()().
    [[nodiscard]] static constexpr result_type min() noexcept { return 0; }

    /// The greatest number of operator()(): 2^32 - 1.
    [[nodiscard]] static constexpr result_type max() noexcept { return 0xffffffffu; }

    /// A generator with the default seed, as `gsl_rng_alloc(gsl_rng_mt19937)`.
    Gsl_Compatible_Rng() noexcept : engine(default_seed) {}

    /** @brief A generator with a seed, as `gsl_rng_alloc()` followed by
     *  `gsl_rng_set(r, s)`.
     *
     *  @param[in] s The seed: 0 is taken as 4357, and else its low 32 bits
     *               count.
     */
    explicit Gsl_Compatible_Rng(const unsigned long s) noexcept : engine(engine_seed(s)) {}

    /** @brief Seeds again, as `gsl_rng_set(r, s)`.
     *
     *  @param[in] s The seed: 0 is taken as 4357, and else its low 32 bits
     *               count.
     */
    void seed(const unsigned long s) noexcept { engine.seed(engine_seed(s)); }

    /** @brief The next number, in [min(), max()], as `gsl_rng_get(r)`.
     *
     *  @return The number.
     */
    result_type operator () () noexcept { return engine(); }

    /** @brief The next number, as operator()().
     *
     *  @return The number, in [min(), max()].
     */
    unsigned long get() noexcept { return engine(); }

    /** @brief An integer in [0, n), each with the same probability, as
     *  `gsl_rng_uniform_int(r, n)`.
     *
     *  It divides a number by (2^32 - 1) / n, and takes another one while
     *  the quotient is n or more.
     *
     *  @param[in] n The number of values, in [1, max()].
     *  @return The integer.
     *  @throw std::domain_error If n is 0 or greater than max(). GSL calls
     *         its error handler, which ends the program by default.
     */
    unsigned long uniform_int(const unsigned long n)
    {
      if (n == 0 or n > max()) [[unlikely]]
        throw_bad_range(n);

      const unsigned long scale = static_cast<unsigned long>(max()) / n;
      unsigned long k;
      do
        k = static_cast<unsigned long>(engine()) / scale;
      while (k >= n);

      return k;
    }

    /** @brief A real number in [0, 1), as `gsl_rng_uniform(r)`.
     *
     *  @return The next number divided by 2^32.
     */
    double uniform() noexcept { return engine() / 4294967296.0; }

    /** @brief A real number in (0, 1), as `gsl_rng_uniform_pos(r)`.
     *
     *  @return The first number of uniform() that is not 0.
     */
    double uniform_pos() noexcept
    {
      double x;
      do
        x = uniform();
      while (x == 0);

      return x;
    }

    /** @brief A real number of the uniform distribution on [a, b), as
     *  `gsl_ran_flat(r, a, b)`.
     *
     *  @param[in] a The least value.
     *  @param[in] b The bound of the values.
     *  @return a (1 - u) + b u, for u of uniform(), with each product
     *          rounded.
     */
    double flat(const double a, const double b) noexcept
    {
      const double u = uniform();
      return rounded(a * (1 - u)) + rounded(b * u);
    }

    /** @brief A real number of the normal distribution of mean 0, as
     *  `gsl_ran_gaussian(r, sigma)`.
     *
     *  The polar method of Box and Muller: a point of the square
     *  (-1, 1)^2, of uniform_pos(), until it falls inside the unit circle
     *  and is not its center; the squares of its coordinates are rounded
     *  before their sum.
     *
     *  @param[in] sigma The standard deviation.
     *  @return The number.
     */
    double gaussian(const double sigma) noexcept
    {
      double x, y, r2;
      do
        {
          x = -1 + 2 * uniform_pos();
          y = -1 + 2 * uniform_pos();
          r2 = rounded(x * x) + rounded(y * y);
        }
      while (r2 > 1.0 or r2 == 0);

      return sigma * y * std::sqrt(-2.0 * std::log(r2) / r2);
    }

    /** @brief A real number of the exponential distribution, as
     *  `gsl_ran_exponential(r, mu)`.
     *
     *  @param[in] mu The mean.
     *  @return -mu log(1 - u), for u of uniform().
     */
    double exponential(const double mu) noexcept
    {
      return -mu * std::log1p(-uniform());
    }

    /** @brief Whether two generators give the same numbers from now on.
     *
     *  @param[in] a A generator.
     *  @param[in] b Another one.
     *  @return Whether their states are equal.
     */
    friend bool operator == (const Gsl_Compatible_Rng & a, const Gsl_Compatible_Rng & b)
    {
      return a.engine == b.engine;
    }
  };

  static_assert(std::uniform_random_bit_generator<Gsl_Compatible_Rng>);

# if ALEPH_WITH_GSL
  extern "C"
  {
    // The functions of the gsl_rng_type of a gsl_rng whose state is a
    // Gsl_Compatible_Rng

    inline void aleph_gsl_compatible_rng_set(void * state, unsigned long s)
    {
      static_cast<Gsl_Compatible_Rng *>(state)->seed(s);
    }

    inline unsigned long aleph_gsl_compatible_rng_get(void * state)
    {
      return static_cast<Gsl_Compatible_Rng *>(state)->get();
    }

    inline double aleph_gsl_compatible_rng_get_double(void * state)
    {
      return static_cast<Gsl_Compatible_Rng *>(state)->uniform();
    }
  }

  static_assert(std::is_trivially_copyable_v<Gsl_Compatible_Rng>
                and std::is_trivially_destructible_v<Gsl_Compatible_Rng>,
                "GSL copies the state of a gsl_rng by its bytes");

  namespace random_detail
  {
    // The type of the gsl_rng of gsl_rng_object(): that of
    // gsl_rng_mt19937, in its limits, but its state is a
    // Gsl_Compatible_Rng, which gsl_rng_clone(), gsl_rng_memcpy(),
    // gsl_rng_fwrite() and gsl_rng_fread() copy by its bytes
    inline const gsl_rng_type gsl_compatible_rng_type =
      {
        "aleph_gsl_compatible_rng",
        0xffffffffUL,
        0,
        sizeof(Gsl_Compatible_Rng),
        &aleph_gsl_compatible_rng_set,
        &aleph_gsl_compatible_rng_get,
        &aleph_gsl_compatible_rng_get_double
      };

    // A gsl_rng whose state is rng. It is made by hand, as gsl_rng_alloc()
    // would make another state and seed it, and so it is freed by hand too:
    // the free() of GSL may not be the one of this malloc(), as with a DLL
    // of GSL that has a CRT of its own on Windows.
    inline gsl_rng * make_gsl_adapter(Gsl_Compatible_Rng & rng)
    {
      auto * r = static_cast<gsl_rng *>(std::malloc(sizeof(gsl_rng)));
      if (r == nullptr)
        throw std::bad_alloc();
      r->type = &gsl_compatible_rng_type;
      r->state = &rng;
      return r;
    }

    inline void free_gsl_adapter(void * r) noexcept
    {
      std::free(r); // the state is the generator, which is not its own
    }
  } // namespace random_detail
# endif

  /** @brief The generator of a randomized structure: a Gsl_Compatible_Rng
   *  in dynamic memory, so that the structure grows only by a pointer, as
   *  with the `gsl_rng` of before.
   *
   *  It moves and swaps with the structure; it is not copied, since each
   *  structure makes its own generator. While `ALEPH_WITH_GSL` is 1, gsl()
   *  gives a `gsl_rng` whose state is the generator, for the deprecated
   *  `gsl_rng_object()` of the structures: the functions of GSL on it act
   *  on the generator of the structure.
   *
   *  @ingroup Utilities
   */
  class Gsl_Compatible_Rng_Owner
  {
    // The generator, and a gsl_rng whose state it is, made the first time
    // that gsl() is called, with the function that frees it; untyped, so
    // that the layout does not depend on ALEPH_WITH_GSL. Atomic, so that
    // calls to gsl() from several threads make one.
    struct Block
    {
      using Free = void (*)(void *) noexcept;

      Gsl_Compatible_Rng rng;
      std::atomic<void *> adapter{nullptr};
      std::atomic<Free> free_adapter{nullptr};

      explicit Block(const unsigned long s) noexcept : rng(s) {}

      ~Block()
      {
        if (void * a = adapter.load(std::memory_order_acquire); a != nullptr)
          free_adapter.load(std::memory_order_relaxed)(a);
      }

      Block(const Block &) = delete;
      Block & operator = (const Block &) = delete;
    };

    // The block is in memory of malloc(), as the gsl_rng of before was, so
    // that it does not count among the allocations of operator new
    struct Free_Block
    {
      void operator () (Block * b) const noexcept
      {
        b->~Block();
        std::free(b);
      }
    };

    std::unique_ptr<Block, Free_Block> block;

  public:
    /// An owner without a generator, as after a move; it can take one by a
    /// move or a swap.
    Gsl_Compatible_Rng_Owner() noexcept = default;

    /** @brief Makes a generator seeded with `s`, as Gsl_Compatible_Rng(s).
     *
     *  @param[in] s The seed.
     *  @throw std::bad_alloc If there is not enough memory: the generator
     *         is in memory of `malloc()`, as the `gsl_rng` of GSL was.
     */
    explicit Gsl_Compatible_Rng_Owner(const unsigned long s)
    {
      void * mem = std::malloc(sizeof(Block));
      if (mem == nullptr)
        throw std::bad_alloc();
      block.reset(::new (mem) Block(s));
    }

    Gsl_Compatible_Rng_Owner(const Gsl_Compatible_Rng_Owner &) = delete;

    Gsl_Compatible_Rng_Owner & operator = (const Gsl_Compatible_Rng_Owner &) = delete;

    /// Takes the generator of `o`, which keeps none.
    Gsl_Compatible_Rng_Owner(Gsl_Compatible_Rng_Owner && o) noexcept = default;

    /** @brief Frees the generator, and takes that of `o`, which keeps none.
     *
     *  @param[in] o The other one.
     *  @return `*this`.
     */
    Gsl_Compatible_Rng_Owner & operator = (Gsl_Compatible_Rng_Owner && o) noexcept = default;

    /** @brief Swaps the generators, with their `gsl_rng`.
     *
     *  @param[in] o The other one.
     */
    void swap(Gsl_Compatible_Rng_Owner & o) noexcept { block.swap(o.block); }

    /// The generator.
    [[nodiscard]] Gsl_Compatible_Rng & operator * () const noexcept { return block->rng; }

    /// The generator.
    [[nodiscard]] Gsl_Compatible_Rng * operator -> () const noexcept { return &block->rng; }

    /// The generator, or nullptr after a move.
    [[nodiscard]] Gsl_Compatible_Rng * get() const noexcept
    {
      return block == nullptr ? nullptr : &block->rng;
    }

# if ALEPH_WITH_GSL
    /** @brief A `gsl_rng` whose state is the generator.
     *
     *  It is made the first time, once even if several threads call it,
     *  and freed with the generator, so that it must not be passed to
     *  `gsl_rng_free()`. As its state is the generator, `gsl_rng_clone()`,
     *  `gsl_rng_memcpy()`, `gsl_rng_fwrite()` and `gsl_rng_fread()` copy,
     *  save and restore the generator, as they do the state of a `gsl_rng`
     *  of GSL; a file of `gsl_rng_fwrite()` is read back by a program built
     *  with the same standard library.
     *
     *  @return The `gsl_rng`.
     *  @throw std::bad_alloc If there is not enough memory.
     */
    [[nodiscard]] gsl_rng * gsl() const
    {
      void * a = block->adapter.load(std::memory_order_acquire);
      if (a == nullptr)
        {
          gsl_rng * made = random_detail::make_gsl_adapter(block->rng);
          block->free_adapter.store(&random_detail::free_gsl_adapter, std::memory_order_relaxed);
          if (block->adapter.compare_exchange_strong(a, made, std::memory_order_acq_rel,
                                                     std::memory_order_acquire))
            a = made;
          else // another thread made one first, which a is now
            random_detail::free_gsl_adapter(made);
        }
      return static_cast<gsl_rng *>(a);
    }
# endif
  };

  static_assert(sizeof(Gsl_Compatible_Rng_Owner) == sizeof(void *));
} // namespace Aleph

# endif // AH_RANDOM_H
