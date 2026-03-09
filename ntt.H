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

/** @file ntt.H
 *  @brief Industrial-grade Number Theoretic Transform core for modular
 *         polynomial multiplication.
 *
 *  This header provides a reusable NTT plan with Montgomery-based butterfly
 *  arithmetic, batch transforms, explicit ThreadPool-powered parallel
 *  variants, big-integer multiplication over base-B digits, and a three-prime
 *  CRT exact multiplier for coefficient ranges that exceed a single modulus.
 *
 *  @ingroup Algorithms
 */

# ifndef NTT_H
# define NTT_H

# if !defined(__SIZEOF_INT128__)
#   error "ntt.H requires compiler support for __uint128_t"
# endif

# include <algorithm>
# include <cstdlib>
# include <cstdint>
# include <future>
# include <limits>
# include <memory>
# include <string>
# include <string_view>
# include <type_traits>
# include <utility>

# if (defined(__GNUC__) or defined(__clang__)) \
      and (defined(__x86_64__) or defined(__i386__) \
           or defined(_M_X64) or defined(_M_IX86))
#   include <immintrin.h>
#   define ALEPH_NTT_HAS_X86_AVX2_DISPATCH 1
#   define ALEPH_NTT_AVX2_TARGET __attribute__((target("avx2")))
# else
#   define ALEPH_NTT_HAS_X86_AVX2_DISPATCH 0
# endif

# if (defined(__GNUC__) or defined(__clang__)) \
      and (defined(__aarch64__) or defined(_M_ARM64))
#   include <arm_neon.h>
#   define ALEPH_NTT_HAS_ARM_NEON_DISPATCH 1
# else
#   define ALEPH_NTT_HAS_ARM_NEON_DISPATCH 0
# endif

# if ALEPH_NTT_HAS_ARM_NEON_DISPATCH and defined(__linux__)
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
# endif

# include <ah-errors.H>
# include <modular_arithmetic.H>
# include <thread_pool.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Number Theoretic Transform over `Z / MOD Z`.
   *
   *  The implementation mirrors the iterative Cooley-Tukey structure used in
   *  Aleph's FFT support, but replaces floating-point arithmetic with exact
   *  modular operations.  Internally, multiplications are carried out in the
   *  Montgomery domain for a fixed odd modulus.
   *
 *  Current capabilities:
 *  - power-of-two transforms
 *  - Bluestein-based transforms for supported non-power-of-two sizes
 *  - AVX2/NEON runtime dispatch on the power-of-two butterfly path
 *  - reusable plans
 *  - batch transforms
 *  - explicit `ThreadPool` parallelism
 *  - fast polynomial multiplication modulo @p MOD
 *  - exact big-integer multiplication over base-B digits
 *  - negacyclic convolution modulo `x^N + 1`
   *
   *  @tparam MOD Prime modulus with a sufficiently large power-of-two factor in
   *          `MOD - 1`.
   *  @tparam ROOT Primitive root modulo @p MOD.
   */
  template <uint64_t MOD = 998244353ULL, uint64_t ROOT = 3ULL>
  class NTT
  {
    static_assert(MOD > 1, "NTT requires MOD > 1");
    static_assert((MOD & 1ULL) == 1ULL, "NTT requires an odd modulus");
    static_assert(ROOT > 0 and ROOT < MOD, "NTT root must lie in (0, MOD)");

  public:
    /** @brief SIMD backends available to the NTT butterfly core. */
    enum class NTTSimdBackend
    {
      scalar, ///< Portable scalar implementation.
      avx2, ///< x86-64 AVX2 grouped butterfly path.
      neon ///< AArch64 NEON grouped butterfly path.
    };

  private:
    enum class SimdPreference
    {
      automatic,
      scalar_only,
      avx2_only,
      neon_only
    };

    enum class Representation
    {
      standard,
      montgomery
    };

    static constexpr MontgomeryCtx mctx_ = montgomery_ctx_for_mod<MOD>();

    [[nodiscard]] static constexpr bool
    is_power_of_two(const size_t n) noexcept
    {
      return n != 0 and (n & (n - 1)) == 0;
    }

    [[nodiscard]] static constexpr uint64_t
    add_mod(const uint64_t lhs,
            const uint64_t rhs) noexcept
    {
      const __uint128_t sum = static_cast<__uint128_t>(lhs) + rhs;
      return static_cast<uint64_t>(sum >= MOD ? sum - MOD : sum);
    }

    [[nodiscard]] static constexpr uint64_t
    sub_mod(const uint64_t lhs,
            const uint64_t rhs) noexcept
    {
      return lhs >= rhs ? lhs - rhs : MOD - (rhs - lhs);
    }

    [[nodiscard]] static constexpr bool
    simd_mod_supported() noexcept
    {
      return MOD <= std::numeric_limits<uint64_t>::max() / 2;
    }

    [[nodiscard]] static constexpr uint64_t
    pow_mod_constexpr(uint64_t base,
                      uint64_t exp) noexcept
    {
      if (MOD == 1)
        return 0;

      uint64_t result = 1;
      base %= MOD;
      while (exp > 0)
        {
          if (exp & 1ULL)
            result = static_cast<uint64_t>(
                (static_cast<__uint128_t>(result) * base) % MOD);
          base = static_cast<uint64_t>(
              (static_cast<__uint128_t>(base) * base) % MOD);
          exp >>= 1;
        }

      return result;
    }

    [[nodiscard]] static constexpr uint64_t
    max_transform_size_impl() noexcept
    {
      uint64_t value = MOD - 1;
      uint64_t size = 1;
      while ((value & 1ULL) == 0)
        {
          size <<= 1;
          value >>= 1;
        }
      return size;
    }

    [[nodiscard]] static constexpr bool
    supports_power_of_two_size(const size_t n) noexcept
    {
      return is_power_of_two(n)
             and n <= static_cast<size_t>(max_transform_size_impl());
    }

    [[nodiscard]] static constexpr bool
    supports_root_order(const uint64_t order) noexcept
    {
      return order != 0 and (MOD - 1) % order == 0;
    }

    [[nodiscard]] static constexpr bool
    supports_bluestein_size(const size_t n) noexcept
    {
      if (n <= 1 or is_power_of_two(n))
        return false;

      if (n > static_cast<size_t>((MOD - 1) / 2))
        return false;

      const uint64_t order = static_cast<uint64_t>(n) * 2ULL;
      if (not supports_root_order(order))
        return false;

      if (n > std::numeric_limits<size_t>::max() / 2)
        return false;

      const size_t required = n * 2 - 1;
      size_t conv_size = 1;
      while (conv_size < required)
        {
          if (conv_size > std::numeric_limits<size_t>::max() / 2)
            return false;
          conv_size <<= 1;
        }

      return supports_power_of_two_size(conv_size);
    }

    static void
    validate_root_order(const uint64_t order,
                        const char * const ctx)
    {
      ah_invalid_argument_if(order == 0)
        << ctx << ": root order must be positive";
      ah_invalid_argument_if(not supports_root_order(order))
        << ctx << ": order " << order
        << " does not divide MOD - 1 (" << (MOD - 1) << ")";
    }

    [[nodiscard]] static constexpr uint64_t
    primitive_root_of_order(const uint64_t order)
    {
      return pow_mod_constexpr(ROOT, (MOD - 1) / order);
    }

    static void
    validate_supported_size(const size_t n,
                            const char * const ctx)
    {
      ah_invalid_argument_if(n == 0)
        << ctx << ": size must be positive";
      ah_invalid_argument_if(not supports_size(n))
        << ctx << ": size " << n
        << " is not supported by MOD " << MOD
        << " (power-of-two sizes require n | 2^k <= "
        << max_transform_size()
        << "; Bluestein sizes require 2*n | (MOD - 1) and an internal "
        << "power-of-two convolution size <= " << max_transform_size() << ")";
    }

    [[nodiscard]] static size_t
    next_power_of_two(size_t n,
                      const char * const ctx)
    {
      if (n <= 1)
        return 1;

      size_t value = 1;
      while (value < n)
        {
          ah_overflow_error_if(value > std::numeric_limits<size_t>::max() / 2)
            << ctx << ": next power of two overflows size_t for requested size "
            << n;
          value <<= 1;
        }

      return value;
    }

    [[nodiscard]] static Array<uint64_t>
    padded_copy(const Array<uint64_t> & input,
                const size_t n)
    {
      Array<uint64_t> output = Array<uint64_t>::create(n);
      for (size_t i = 0; i < n; ++i)
        output(i) = 0;

      for (size_t i = 0; i < input.size(); ++i)
        output(i) = input[i] % MOD;

      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    prefix_copy(const Array<uint64_t> & input,
                const size_t length)
    {
      ah_invalid_argument_if(length > input.size())
        << "NTT::prefix_copy: length " << length
        << " exceeds input size " << input.size();

      Array<uint64_t> output;
      output.reserve(length);
      for (size_t i = 0; i < length; ++i)
        output.append(input[i]);
      return output;
    }

  public:
    static constexpr uint64_t mod = MOD;
    static constexpr uint64_t root = ROOT;

    [[nodiscard]] static constexpr const char *
    simd_backend_name(const NTTSimdBackend backend) noexcept
    {
      switch (backend)
        {
        case NTTSimdBackend::avx2:
          return "avx2";
        case NTTSimdBackend::neon:
          return "neon";
        case NTTSimdBackend::scalar:
        default:
          return "scalar";
        }
    }

  private:
    [[nodiscard]] static constexpr const char *
    simd_preference_name(const SimdPreference preference) noexcept
    {
      switch (preference)
        {
        case SimdPreference::scalar_only:
          return "scalar";
        case SimdPreference::avx2_only:
          return "avx2";
        case SimdPreference::neon_only:
          return "neon";
        case SimdPreference::automatic:
        default:
          return "auto";
        }
    }

    [[nodiscard]] static SimdPreference
    simd_preference() noexcept
    {
      if (const char *mode = std::getenv("ALEPH_NTT_SIMD");
          mode != nullptr and mode[0] != '\0')
        {
          const std::string_view value(mode);
          if (value == "scalar")
            return SimdPreference::scalar_only;
          if (value == "avx2")
            return SimdPreference::avx2_only;
          if (value == "neon")
            return SimdPreference::neon_only;
        }

      return SimdPreference::automatic;
    }

    [[nodiscard]] static NTTSimdBackend
    detected_simd_backend() noexcept
    {
      if (avx2_dispatch_available())
        return NTTSimdBackend::avx2;
      if (neon_dispatch_available())
        return NTTSimdBackend::neon;
      return NTTSimdBackend::scalar;
    }

  public:
    /** @brief Returns whether AVX2 dispatch is available at runtime. */
    [[nodiscard]] static bool
    avx2_dispatch_available() noexcept
    {
# if ALEPH_NTT_HAS_X86_AVX2_DISPATCH
      static const bool available = []() noexcept
        {
          __builtin_cpu_init();
          return static_cast<bool>(__builtin_cpu_supports("avx2"));
        }();
      return available;
# else
      return false;
# endif
    }

    /** @brief Returns whether AArch64 NEON dispatch is available at runtime. */
    [[nodiscard]] static bool
    neon_dispatch_available() noexcept
    {
# if ALEPH_NTT_HAS_ARM_NEON_DISPATCH
#   if defined(__linux__) and defined(HWCAP_ASIMD)
      static const bool available = []() noexcept
        {
          return (getauxval(AT_HWCAP) & HWCAP_ASIMD) != 0;
        }();
      return available;
#   else
      return true;
#   endif
# else
      return false;
# endif
    }

    /** @brief Returns the SIMD backend selected under `ALEPH_NTT_SIMD`. */
    [[nodiscard]] static NTTSimdBackend
    simd_backend() noexcept
    {
      const NTTSimdBackend detected = detected_simd_backend();
      switch (simd_preference())
        {
        case SimdPreference::scalar_only:
          return NTTSimdBackend::scalar;
        case SimdPreference::avx2_only:
          return detected == NTTSimdBackend::avx2 ?
                   NTTSimdBackend::avx2 :
                   NTTSimdBackend::scalar;
        case SimdPreference::neon_only:
          return detected == NTTSimdBackend::neon ?
                   NTTSimdBackend::neon :
                   NTTSimdBackend::scalar;
        case SimdPreference::automatic:
        default:
          return detected;
        }
    }

    /** @brief Returns the active SIMD backend name. */
    [[nodiscard]] static const char *
    simd_backend_name() noexcept
    {
      return simd_backend_name(simd_backend());
    }

    class Plan
    {
      enum class Strategy
      {
        power_of_two,
        bluestein
      };

      Strategy strategy_ = Strategy::power_of_two;
      size_t n_ = 0;
      size_t log_n_ = 0;
      Array<size_t> bit_rev_;
      Array<uint64_t> twiddles_fwd_;
      Array<uint64_t> twiddles_inv_;
      uint64_t inv_n_ = 0;
      uint64_t inv_n_std_ = 1;
      size_t bluestein_size_ = 0;
      Array<uint64_t> bluestein_chirp_fwd_;
      Array<uint64_t> bluestein_chirp_inv_;
      Array<uint64_t> bluestein_kernel_forward_;
      Array<uint64_t> bluestein_kernel_inverse_;
      std::shared_ptr<const Plan> bluestein_plan_;

      template <typename F>
      static void
      for_each_index(ThreadPool * const pool,
                     const size_t count,
                     F && fn,
                     const size_t chunk_size)
      {
        if (count == 0)
          return;

        if (pool != nullptr and pool->num_threads() > 1 and count > 1)
          {
            parallel_for_index(*pool, 0, count, std::forward<F>(fn),
                               chunk_size);
            return;
          }

        for (size_t i = 0; i < count; ++i)
          fn(i);
      }

      void
      apply_scalar_butterfly_range(Array<uint64_t> & a,
                                   const Array<uint64_t> & twiddles,
                                   const size_t base,
                                   const size_t half,
                                   const size_t offset,
                                   const size_t begin,
                                   const size_t end) const
      {
        for (size_t j = begin; j < end; ++j)
          {
            const uint64_t u = a[base + j];
            const uint64_t v =
                mont_mul(a[base + j + half], twiddles[offset + j], mctx_);
            a(base + j) = add_mod(u, v);
            a(base + j + half) = sub_mod(u, v);
          }
      }

      [[nodiscard]] bool
      should_use_avx2(ThreadPool * const pool) const noexcept
      {
# if !ALEPH_NTT_HAS_X86_AVX2_DISPATCH
        (void) pool;
        return false;
# else
        if (strategy_ != Strategy::power_of_two or not simd_mod_supported())
          return false;

        if (pool != nullptr and pool->num_threads() > 1)
          return false;

        return NTT::simd_backend() == NTTSimdBackend::avx2;
# endif
      }

      [[nodiscard]] bool
      should_use_neon(ThreadPool * const pool) const noexcept
      {
# if !ALEPH_NTT_HAS_ARM_NEON_DISPATCH
        (void) pool;
        return false;
# else
        if (strategy_ != Strategy::power_of_two or not simd_mod_supported())
          return false;

        if (pool != nullptr and pool->num_threads() > 1)
          return false;

        return NTT::simd_backend() == NTTSimdBackend::neon;
# endif
      }

# if ALEPH_NTT_HAS_X86_AVX2_DISPATCH
      [[nodiscard]] static __m256i
      avx2_cmp_ge_u64(const __m256i lhs,
                      const __m256i rhs) noexcept ALEPH_NTT_AVX2_TARGET
      {
        alignas(32) static const uint64_t sign_bits[4] = {
          0x8000000000000000ULL,
          0x8000000000000000ULL,
          0x8000000000000000ULL,
          0x8000000000000000ULL
        };
        const __m256i sign =
            _mm256_load_si256(reinterpret_cast<const __m256i *>(sign_bits));
        const __m256i lhs_signed = _mm256_xor_si256(lhs, sign);
        const __m256i rhs_signed = _mm256_xor_si256(rhs, sign);
        const __m256i gt = _mm256_cmpgt_epi64(lhs_signed, rhs_signed);
        const __m256i eq = _mm256_cmpeq_epi64(lhs, rhs);
        return _mm256_or_si256(gt, eq);
      }

      static void
      avx2_apply_chunk(uint64_t * const low,
                       uint64_t * const high,
                       const uint64_t * const twiddle_ptr) ALEPH_NTT_AVX2_TARGET
      {
        alignas(32) uint64_t products[4];
        for (size_t lane = 0; lane < 4; ++lane)
          products[lane] = mont_mul(high[lane], twiddle_ptr[lane], mctx_);

        alignas(32) static const uint64_t mod_lanes[4] = {MOD, MOD, MOD, MOD};
        const __m256i modv =
            _mm256_load_si256(reinterpret_cast<const __m256i *>(mod_lanes));
        const __m256i u =
            _mm256_loadu_si256(reinterpret_cast<const __m256i *>(low));
        const __m256i v =
            _mm256_load_si256(reinterpret_cast<const __m256i *>(products));

        const __m256i sum = _mm256_add_epi64(u, v);
        const __m256i sum_minus_mod = _mm256_sub_epi64(sum, modv);
        const __m256i sum_mask = avx2_cmp_ge_u64(sum, modv);
        const __m256i sum_result = _mm256_blendv_epi8(sum, sum_minus_mod, sum_mask);

        const __m256i diff = _mm256_sub_epi64(u, v);
        const __m256i vu = _mm256_sub_epi64(v, u);
        const __m256i diff_alt = _mm256_sub_epi64(modv, vu);
        const __m256i diff_mask = avx2_cmp_ge_u64(u, v);
        const __m256i diff_result =
            _mm256_blendv_epi8(diff_alt, diff, diff_mask);

        _mm256_storeu_si256(reinterpret_cast<__m256i *>(low), sum_result);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(high), diff_result);
      }

      void
      apply_butterflies_avx2(Array<uint64_t> & a,
                             const Array<uint64_t> & twiddles) const ALEPH_NTT_AVX2_TARGET
      {
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = static_cast<size_t>(1) << stage;
            const size_t len = half << 1;
            const size_t blocks = n_ / len;
            const size_t offset = half - 1;

            for (size_t block = 0; block < blocks; ++block)
              {
                const size_t base = block * len;
                size_t j = 0;
                for (; j + 4 <= half; j += 4)
                  avx2_apply_chunk(&a(base + j),
                                   &a(base + j + half),
                                   &twiddles[offset + j]);
                apply_scalar_butterfly_range(a, twiddles, base, half, offset,
                                             j, half);
              }
          }
      }
# endif

# if ALEPH_NTT_HAS_ARM_NEON_DISPATCH
      static void
      neon_apply_chunk(uint64_t * const low,
                       uint64_t * const high,
                       const uint64_t * const twiddle_ptr)
      {
        alignas(16) uint64_t products[2];
        for (size_t lane = 0; lane < 2; ++lane)
          products[lane] = mont_mul(high[lane], twiddle_ptr[lane], mctx_);

        const uint64x2_t modv = vdupq_n_u64(MOD);
        const uint64x2_t u = vld1q_u64(low);
        const uint64x2_t v = vld1q_u64(products);
        const uint64x2_t sum = vaddq_u64(u, v);
        const uint64x2_t sum_minus_mod = vsubq_u64(sum, modv);
        const uint64x2_t sum_mask = vcgeq_u64(sum, modv);
        const uint64x2_t sum_result = vbslq_u64(sum_mask, sum_minus_mod, sum);

        const uint64x2_t diff = vsubq_u64(u, v);
        const uint64x2_t vu = vsubq_u64(v, u);
        const uint64x2_t diff_alt = vsubq_u64(modv, vu);
        const uint64x2_t diff_mask = vcgeq_u64(u, v);
        const uint64x2_t diff_result = vbslq_u64(diff_mask, diff, diff_alt);

        vst1q_u64(low, sum_result);
        vst1q_u64(high, diff_result);
      }

      void
      apply_butterflies_neon(Array<uint64_t> & a,
                             const Array<uint64_t> & twiddles) const
      {
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = static_cast<size_t>(1) << stage;
            const size_t len = half << 1;
            const size_t blocks = n_ / len;
            const size_t offset = half - 1;

            for (size_t block = 0; block < blocks; ++block)
              {
                const size_t base = block * len;
                size_t j = 0;
                for (; j + 2 <= half; j += 2)
                  neon_apply_chunk(&a(base + j),
                                   &a(base + j + half),
                                   &twiddles[offset + j]);
                apply_scalar_butterfly_range(a, twiddles, base, half, offset,
                                             j, half);
              }
          }
      }
# endif

      void
      initialize_bit_reversal()
      {
        bit_rev_ = Array<size_t>::create(n_);
        bit_rev_(0) = 0;
        for (size_t i = 1, j = 0; i < n_; ++i)
          {
            size_t bit = n_ >> 1;
            for (; j & bit; bit >>= 1)
              j ^= bit;
            j ^= bit;
            bit_rev_(i) = j;
          }
      }

      void
      initialize_twiddles()
      {
        if (n_ <= 1)
          return;

        twiddles_fwd_ = Array<uint64_t>::create(n_ - 1);
        twiddles_inv_ = Array<uint64_t>::create(n_ - 1);

        const uint64_t mont_one = to_mont(1, mctx_);
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = static_cast<size_t>(1) << stage;
            const size_t len = half << 1;
            const size_t offset = half - 1;

            const uint64_t root_len = NTT::primitive_root_of_unity(len);
            const uint64_t root_len_inv = mod_inv(root_len, MOD);

            uint64_t w_fwd = mont_one;
            uint64_t w_inv = mont_one;
            const uint64_t w_step_fwd = to_mont(root_len, mctx_);
            const uint64_t w_step_inv = to_mont(root_len_inv, mctx_);

            for (size_t j = 0; j < half; ++j)
              {
                twiddles_fwd_(offset + j) = w_fwd;
                twiddles_inv_(offset + j) = w_inv;
                w_fwd = mont_mul(w_fwd, w_step_fwd, mctx_);
                w_inv = mont_mul(w_inv, w_step_inv, mctx_);
              }
          }
      }

      void
      initialize_power_of_two_plan()
      {
        strategy_ = Strategy::power_of_two;

        for (size_t value = n_; value > 1; value >>= 1)
          ++log_n_;

        initialize_bit_reversal();
        initialize_twiddles();
        inv_n_ = to_mont(mod_inv(static_cast<uint64_t>(n_), MOD), mctx_);
      }

      void
      initialize_bluestein_plan()
      {
        strategy_ = Strategy::bluestein;

        ah_invalid_argument_if(n_ > std::numeric_limits<size_t>::max() / 2)
          << "NTT::Plan: size " << n_
          << " is too large for Bluestein convolution sizing";

        const size_t required = n_ * 2 - 1;
        bluestein_size_ = NTT::next_power_of_two(required, "NTT::Plan");
        ah_invalid_argument_if(not NTT::supports_power_of_two_size(bluestein_size_))
          << "NTT::Plan: Bluestein internal size " << bluestein_size_
          << " exceeds the power-of-two capacity of MOD " << MOD;

        const uint64_t order = static_cast<uint64_t>(n_) * 2ULL;
        NTT::validate_root_order(order, "NTT::Plan");

        bluestein_plan_ = std::make_shared<Plan>(bluestein_size_);
        bluestein_chirp_fwd_ = Array<uint64_t>::create(n_);
        bluestein_chirp_inv_ = Array<uint64_t>::create(n_);

        const uint64_t z = NTT::primitive_root_of_order(order);
        const uint64_t z_inv = mod_inv(z, MOD);
        for (size_t i = 0; i < n_; ++i)
          {
            const uint64_t exponent =
                static_cast<uint64_t>((static_cast<__uint128_t>(i) * i) % order);
            bluestein_chirp_fwd_(i) = mod_exp(z, exponent, MOD);
            bluestein_chirp_inv_(i) = mod_exp(z_inv, exponent, MOD);
          }

        bluestein_kernel_forward_ = Array<uint64_t>::create(bluestein_size_);
        bluestein_kernel_inverse_ = Array<uint64_t>::create(bluestein_size_);
        for (size_t i = 0; i < bluestein_size_; ++i)
          {
            bluestein_kernel_forward_(i) = 0;
            bluestein_kernel_inverse_(i) = 0;
          }

        bluestein_kernel_forward_(0) = 1;
        bluestein_kernel_inverse_(0) = 1;
        for (size_t i = 1; i < n_; ++i)
          {
            bluestein_kernel_forward_(i) = bluestein_chirp_inv_[i];
            bluestein_kernel_forward_(bluestein_size_ - i) =
                bluestein_chirp_inv_[i];

            bluestein_kernel_inverse_(i) = bluestein_chirp_fwd_[i];
            bluestein_kernel_inverse_(bluestein_size_ - i) =
                bluestein_chirp_fwd_[i];
          }

        bluestein_plan_->transform(bluestein_kernel_forward_, false);
        bluestein_plan_->transform(bluestein_kernel_inverse_, false);
      }

      void
      apply_bit_reversal(Array<uint64_t> & a) const noexcept
      {
        for (size_t i = 0; i < n_; ++i)
          if (i < bit_rev_(i))
            std::swap(a(i), a(bit_rev_(i)));
      }

      void
      lift_input(Array<uint64_t> & a,
                 ThreadPool * const pool,
                 const size_t chunk_size) const
      {
        auto lift_one = [&a](const size_t i)
          {
            a(i) = to_mont(a[i] % MOD, mctx_);
          };
        for_each_index(pool, n_, lift_one, chunk_size);
      }

      void
      scale_inverse(Array<uint64_t> & a,
                    ThreadPool * const pool,
                    const size_t chunk_size) const
      {
        auto scale_one = [this, &a](const size_t i)
          {
            a(i) = mont_mul(a[i], inv_n_, mctx_);
          };
        for_each_index(pool, n_, scale_one, chunk_size);
      }

      void
      lower_output(Array<uint64_t> & a,
                   ThreadPool * const pool,
                   const size_t chunk_size) const
      {
        auto lower_one = [&a](const size_t i)
          {
            a(i) = from_mont(a[i], mctx_);
          };
        for_each_index(pool, n_, lower_one, chunk_size);
      }

      void
      apply_butterflies_scalar(Array<uint64_t> & a,
                               const Array<uint64_t> & twiddles,
                               ThreadPool * const pool,
                               const size_t chunk_size) const
      {
        for (size_t stage = 0; stage < log_n_; ++stage)
          {
            const size_t half = static_cast<size_t>(1) << stage;
            const size_t len = half << 1;
            const size_t blocks = n_ / len;
            const size_t offset = half - 1;

            auto butterfly_block = [this, &a, &twiddles, half, len, offset]
            (const size_t block)
              {
                const size_t base = block * len;
                apply_scalar_butterfly_range(a, twiddles, base, half, offset,
                                             0, half);
              };

            for_each_index(pool, blocks, butterfly_block, chunk_size);
          }
      }

      void
      apply_butterflies(Array<uint64_t> & a,
                        const bool invert,
                        ThreadPool * const pool,
                        const size_t chunk_size) const
      {
        const Array<uint64_t> & twiddles =
            invert ? twiddles_inv_ : twiddles_fwd_;

# if ALEPH_NTT_HAS_X86_AVX2_DISPATCH
        if (should_use_avx2(pool))
          {
            apply_butterflies_avx2(a, twiddles);
            return;
          }
# endif

# if ALEPH_NTT_HAS_ARM_NEON_DISPATCH
        if (should_use_neon(pool))
          {
            apply_butterflies_neon(a, twiddles);
            return;
          }
# endif

        apply_butterflies_scalar(a, twiddles, pool, chunk_size);
      }

      void
      apply_bluestein_transform(Array<uint64_t> & a,
                                const bool invert,
                                ThreadPool * const pool,
                                const size_t chunk_size) const
      {
        ah_runtime_error_unless(bluestein_plan_ != nullptr)
          << "NTT::Plan::apply_bluestein_transform: missing internal plan";

        Array<uint64_t> work = Array<uint64_t>::create(bluestein_size_);
        for (size_t i = 0; i < bluestein_size_; ++i)
          work(i) = 0;

        const Array<uint64_t> & input_chirp =
            invert ? bluestein_chirp_inv_ : bluestein_chirp_fwd_;
        const Array<uint64_t> & output_chirp =
            invert ? bluestein_chirp_inv_ : bluestein_chirp_fwd_;
        const Array<uint64_t> & kernel =
            invert ? bluestein_kernel_inverse_ : bluestein_kernel_forward_;

        auto initialize = [&a, &work, &input_chirp](const size_t i)
          {
            if (i < a.size())
              work(i) = mod_mul(a[i] % MOD, input_chirp[i], MOD);
          };
        for_each_index(pool, n_, initialize, chunk_size);

        if (pool != nullptr and pool->num_threads() > 1)
          bluestein_plan_->ptransform(*pool, work, false, chunk_size);
        else
          bluestein_plan_->transform(work, false);

        auto pointwise = [&work, &kernel](const size_t i)
          {
            work(i) = mod_mul(work[i], kernel[i], MOD);
          };
        for_each_index(pool, bluestein_size_, pointwise, chunk_size);

        if (pool != nullptr and pool->num_threads() > 1)
          bluestein_plan_->ptransform(*pool, work, true, chunk_size);
        else
          bluestein_plan_->transform(work, true);

        auto finalize = [this, &a, &work, &output_chirp, invert](const size_t i)
          {
            uint64_t value = mod_mul(work[i], output_chirp[i], MOD);
            if (invert)
              value = mod_mul(value, inv_n_std_, MOD);
            a(i) = value;
          };
        for_each_index(pool, n_, finalize, chunk_size);
      }

      void
      apply_transform(Array<uint64_t> & a,
                      const bool invert,
                      const Representation input_repr,
                      const Representation output_repr,
                      ThreadPool * const pool,
                      const size_t chunk_size) const
      {
        ah_invalid_argument_if(a.size() != n_)
          << "NTT::Plan::transform: input size " << a.size()
          << " does not match plan size " << n_;

        switch (strategy_)
          {
          case Strategy::power_of_two:
            if (input_repr == Representation::standard)
              lift_input(a, pool, chunk_size);

            if (n_ > 1)
              {
                apply_bit_reversal(a);
                apply_butterflies(a, invert, pool, chunk_size);
              }

            if (invert)
              scale_inverse(a, pool, chunk_size);

            if (output_repr == Representation::standard)
              lower_output(a, pool, chunk_size);
            break;

          case Strategy::bluestein:
            ah_invalid_argument_if(input_repr != Representation::standard)
              << "NTT::Plan::apply_transform: Bluestein path expects standard "
              << "input representation";
            ah_invalid_argument_if(output_repr != Representation::standard)
              << "NTT::Plan::apply_transform: Bluestein path returns standard "
              << "output representation";
            apply_bluestein_transform(a, invert, pool, chunk_size);
            break;
          }
      }

      [[nodiscard]] Array<uint64_t>
      multiply_impl(const Array<uint64_t> & a,
                    const Array<uint64_t> & b,
                    ThreadPool * const pool,
                    const size_t chunk_size) const
      {
        if (a.is_empty() or b.is_empty())
          return {};

        ah_invalid_argument_if(a.size() >
                               std::numeric_limits<size_t>::max()
                               - b.size() + 1)
          << "NTT::Plan::multiply: product size exceeds size_t capacity";

        const size_t required = a.size() + b.size() - 1;
        ah_invalid_argument_if(required > n_)
          << "NTT::Plan::multiply: product size " << required
          << " exceeds plan size " << n_;

        Array<uint64_t> fa = NTT::padded_copy(a, n_);
        Array<uint64_t> fb = NTT::padded_copy(b, n_);

        if (strategy_ == Strategy::power_of_two)
          {
            apply_transform(fa, false,
                            Representation::standard,
                            Representation::montgomery,
                            pool, chunk_size);
            apply_transform(fb, false,
                            Representation::standard,
                            Representation::montgomery,
                            pool, chunk_size);

            auto pointwise_product = [&fa, &fb](const size_t i)
              {
                fa(i) = mont_mul(fa[i], fb[i], mctx_);
              };
            for_each_index(pool, n_, pointwise_product, chunk_size);

            apply_transform(fa, true,
                            Representation::montgomery,
                            Representation::standard,
                            pool, chunk_size);
          }
        else
          {
            apply_transform(fa, false,
                            Representation::standard,
                            Representation::standard,
                            pool, chunk_size);
            apply_transform(fb, false,
                            Representation::standard,
                            Representation::standard,
                            pool, chunk_size);

            auto pointwise_product = [&fa, &fb](const size_t i)
              {
                fa(i) = mod_mul(fa[i], fb[i], MOD);
              };
            for_each_index(pool, n_, pointwise_product, chunk_size);

            apply_transform(fa, true,
                            Representation::standard,
                            Representation::standard,
                            pool, chunk_size);
          }

        return NTT::prefix_copy(fa, required);
      }

    public:
      /** @brief Construct a reusable plan for a fixed transform size.
       *
       *  @param n Transform size. It must be a power of two supported by
       *         `MOD - 1`.
       *  @throws std::invalid_argument if @p n is zero, not a power of two, or
       *          exceeds the modulus capability.
       */
      explicit Plan(const size_t n) : n_(n)
      {
        NTT::validate_supported_size(n_, "NTT::Plan");
        inv_n_std_ = mod_inv(static_cast<uint64_t>(n_), MOD);

        if (NTT::supports_power_of_two_size(n_))
          initialize_power_of_two_plan();
        else
          initialize_bluestein_plan();
      }

      /** @brief Return the transform size bound to the plan. */
      [[nodiscard]] size_t size() const noexcept
      {
        return n_;
      }

      /** @brief In-place forward or inverse transform.
       *
       *  @param a Input/output array. Its size must equal the plan size.
       *  @param invert If true, computes the inverse NTT.
       */
      void
      transform(Array<uint64_t> & a,
                const bool invert) const
      {
        apply_transform(a, invert,
                        Representation::standard,
                        Representation::standard,
                        nullptr, 0);
      }

      /** @brief Transform an input array and return the result.
       *
       *  @param input Input array. Its size must equal the plan size.
       *  @param invert If true, computes the inverse NTT.
       *  @return Transformed copy of @p input.
       */
      [[nodiscard]] Array<uint64_t>
      transformed(const Array<uint64_t> & input,
                  const bool invert = false) const
      {
        Array<uint64_t> output = input;
        transform(output, invert);
        return output;
      }

      /** @brief Multiply two polynomials using this plan size.
       *
       *  @param a First polynomial.
       *  @param b Second polynomial.
       *  @return Convolution modulo @p MOD.
       *  @throws std::invalid_argument if the result length exceeds the plan
       *          size.
       */
      [[nodiscard]] Array<uint64_t>
      multiply(const Array<uint64_t> & a,
               const Array<uint64_t> & b) const
      {
        return multiply_impl(a, b, nullptr, 0);
      }

      /** @brief Parallel in-place transform using a ThreadPool.
       *
       *  @param pool Thread pool used for stage-level parallelism.
       *  @param a Input/output array. Its size must equal the plan size.
       *  @param invert If true, computes the inverse NTT.
       *  @param chunk_size Iterations per scheduled chunk (0 selects an
       *         automatic value).
       */
      void
      ptransform(ThreadPool & pool,
                 Array<uint64_t> & a,
                 const bool invert,
                 const size_t chunk_size = 0) const
      {
        apply_transform(a, invert,
                        Representation::standard,
                        Representation::standard,
                        &pool, chunk_size);
      }

      /** @brief Parallel transform returning a new array.
       *
       *  @param pool Thread pool used for stage-level parallelism.
       *  @param input Input array. Its size must equal the plan size.
       *  @param invert If true, computes the inverse NTT.
       *  @param chunk_size Iterations per scheduled chunk (0 selects an
       *         automatic value).
       *  @return Transformed copy of @p input.
       */
      [[nodiscard]] Array<uint64_t>
      ptransformed(ThreadPool & pool,
                   const Array<uint64_t> & input,
                   const bool invert = false,
                   const size_t chunk_size = 0) const
      {
        Array<uint64_t> output = input;
        ptransform(pool, output, invert, chunk_size);
        return output;
      }

      /** @brief Parallel polynomial multiplication using this plan size.
       *
       *  @param pool Thread pool used for transform stages and pointwise
       *         products.
       *  @param a First polynomial.
       *  @param b Second polynomial.
       *  @param chunk_size Iterations per scheduled chunk (0 selects an
       *         automatic value).
       *  @return Convolution modulo @p MOD.
       */
      [[nodiscard]] Array<uint64_t>
      pmultiply(ThreadPool & pool,
                const Array<uint64_t> & a,
                const Array<uint64_t> & b,
                const size_t chunk_size = 0) const
      {
        return multiply_impl(a, b, &pool, chunk_size);
      }

      /** @brief Sequential batch transform for equal-sized inputs.
       *
       *  @param batch Batch of arrays; every item must match the plan size.
       *  @param invert If true, computes inverse transforms.
       */
      void
      transform_batch(Array<Array<uint64_t>> & batch,
                      const bool invert) const
      {
        for (size_t i = 0; i < batch.size(); ++i)
          {
            ah_invalid_argument_if(batch[i].size() != n_)
              << "NTT::Plan::transform_batch: batch item " << i
              << " has size " << batch[i].size()
              << " but plan size is " << n_;
            apply_transform(batch(i), invert,
                            Representation::standard,
                            Representation::standard,
                            nullptr, 0);
          }
      }

      /** @brief Parallel batch transform for equal-sized inputs.
       *
       *  Parallelism is applied across batch items when possible.  For a single
       *  item, the internal transform itself uses the supplied thread pool.
       *
       *  @param pool Thread pool used for execution.
       *  @param batch Batch of arrays; every item must match the plan size.
       *  @param invert If true, computes inverse transforms.
       *  @param chunk_size Iterations per scheduled chunk (0 selects an
       *         automatic value).
       */
      void
      ptransform_batch(ThreadPool & pool,
                       Array<Array<uint64_t>> & batch,
                       const bool invert,
                       const size_t chunk_size = 0) const
      {
        for (size_t i = 0; i < batch.size(); ++i)
          ah_invalid_argument_if(batch[i].size() != n_)
            << "NTT::Plan::ptransform_batch: batch item " << i
            << " has size " << batch[i].size()
            << " but plan size is " << n_;

        if (batch.is_empty())
          return;

        if (batch.size() == 1 or pool.num_threads() <= 1)
          {
            for (size_t i = 0; i < batch.size(); ++i)
              apply_transform(batch(i), invert,
                              Representation::standard,
                              Representation::standard,
                              &pool, chunk_size);
            return;
          }

        auto transform_one = [this, &batch, invert](const size_t i)
          {
            apply_transform(batch(i), invert,
                            Representation::standard,
                            Representation::standard,
                            nullptr, 0);
          };
        parallel_for_index(pool, 0, batch.size(), transform_one, chunk_size);
      }

      /** @brief Return a transformed copy of an entire batch.
       *
       *  @param input Batch of arrays; every item must match the plan size.
       *  @param invert If true, computes inverse transforms.
       *  @return Transformed batch.
       */
      [[nodiscard]] Array<Array<uint64_t>>
      transformed_batch(const Array<Array<uint64_t>> & input,
                        const bool invert = false) const
      {
        Array<Array<uint64_t>> output = input;
        transform_batch(output, invert);
        return output;
      }

      /** @brief Return a parallel-transformed copy of an entire batch.
       *
       *  @param pool Thread pool used for execution.
       *  @param input Batch of arrays; every item must match the plan size.
       *  @param invert If true, computes inverse transforms.
       *  @param chunk_size Iterations per scheduled chunk (0 selects an
       *         automatic value).
       *  @return Transformed batch.
       */
      [[nodiscard]] Array<Array<uint64_t>>
      ptransformed_batch(ThreadPool & pool,
                         const Array<Array<uint64_t>> & input,
                         const bool invert = false,
                         const size_t chunk_size = 0) const
      {
        Array<Array<uint64_t>> output = input;
        ptransform_batch(pool, output, invert, chunk_size);
        return output;
      }
    };

  private:
    static void
    trim_trailing_zeros(Array<uint64_t> & poly)
    {
      while (not poly.is_empty() and poly.get_last() % MOD == 0)
        static_cast<void>(poly.remove_last());
    }

    [[nodiscard]] static Array<uint64_t>
    normalize_poly(const Array<uint64_t> & input)
    {
      Array<uint64_t> output;
      output.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
        output.append(input[i] % MOD);
      trim_trailing_zeros(output);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    zero_series(const size_t n)
    {
      Array<uint64_t> output = Array<uint64_t>::create(n);
      for (size_t i = 0; i < n; ++i)
        output(i) = 0;
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    series_prefix(const Array<uint64_t> & input,
                  const size_t n)
    {
      Array<uint64_t> output = zero_series(n);
      const size_t limit = std::min(input.size(), n);
      for (size_t i = 0; i < limit; ++i)
        output(i) = input[i] % MOD;
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    truncate_poly(const Array<uint64_t> & input,
                  const size_t n)
    {
      Array<uint64_t> output;
      output.reserve(std::min(input.size(), n));
      for (size_t i = 0; i < input.size() and i < n; ++i)
        output.append(input[i] % MOD);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    reverse_poly(const Array<uint64_t> & input)
    {
      Array<uint64_t> output;
      output.reserve(input.size());
      for (size_t i = input.size(); i > 0; --i)
        output.append(input[i - 1] % MOD);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_add_series(const Array<uint64_t> & lhs,
                    const Array<uint64_t> & rhs,
                    const size_t n)
    {
      Array<uint64_t> output = zero_series(n);
      for (size_t i = 0; i < n; ++i)
        {
          const uint64_t a = i < lhs.size() ? lhs[i] % MOD : 0;
          const uint64_t b = i < rhs.size() ? rhs[i] % MOD : 0;
          output(i) = add_mod(a, b);
        }
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_sub_series(const Array<uint64_t> & lhs,
                    const Array<uint64_t> & rhs,
                    const size_t n)
    {
      Array<uint64_t> output = zero_series(n);
      for (size_t i = 0; i < n; ++i)
        {
          const uint64_t a = i < lhs.size() ? lhs[i] % MOD : 0;
          const uint64_t b = i < rhs.size() ? rhs[i] % MOD : 0;
          output(i) = sub_mod(a, b);
        }
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_add_normalized(const Array<uint64_t> & lhs,
                        const Array<uint64_t> & rhs)
    {
      const size_t n = std::max(lhs.size(), rhs.size());
      Array<uint64_t> output = poly_add_series(lhs, rhs, n);
      trim_trailing_zeros(output);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_sub_normalized(const Array<uint64_t> & lhs,
                        const Array<uint64_t> & rhs)
    {
      const size_t n = std::max(lhs.size(), rhs.size());
      Array<uint64_t> output = poly_sub_series(lhs, rhs, n);
      trim_trailing_zeros(output);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_scalar_mul_series(const Array<uint64_t> & input,
                           const uint64_t scalar,
                           const size_t n)
    {
      Array<uint64_t> output = zero_series(n);
      const uint64_t factor = scalar % MOD;
      for (size_t i = 0; i < input.size() and i < n; ++i)
        output(i) = mod_mul(input[i] % MOD, factor, MOD);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_mul_trunc(const Array<uint64_t> & lhs,
                   const Array<uint64_t> & rhs,
                   const size_t n)
    {
      if (n == 0)
        return {};

      const Array<uint64_t> left = truncate_poly(lhs, n);
      const Array<uint64_t> right = truncate_poly(rhs, n);
      if (left.is_empty() or right.is_empty())
        return zero_series(n);

      return series_prefix(multiply(left, right), n);
    }

    [[nodiscard]] static Array<uint64_t>
    poly_derivative(const Array<uint64_t> & coeffs)
    {
      if (coeffs.size() <= 1)
        return {};

      Array<uint64_t> output = Array<uint64_t>::create(coeffs.size() - 1);
      for (size_t i = 1; i < coeffs.size(); ++i)
        output(i - 1) = mod_mul(coeffs[i] % MOD,
                                static_cast<uint64_t>(i) % MOD,
                                MOD);
      return output;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_integral(const Array<uint64_t> & coeffs)
    {
      Array<uint64_t> output = Array<uint64_t>::create(coeffs.size() + 1);
      output(0) = 0;
      for (size_t i = 0; i < coeffs.size(); ++i)
        {
          const uint64_t inv = mod_inv(static_cast<uint64_t>(i + 1), MOD);
          output(i + 1) = mod_mul(coeffs[i] % MOD, inv, MOD);
        }
      return output;
    }

    [[nodiscard]] static uint64_t
    tonelli_shanks(const uint64_t value,
                   const char * const ctx)
    {
      const uint64_t a = value % MOD;
      if (a == 0)
        return 0;

      const uint64_t legendre = mod_exp(a, (MOD - 1) / 2, MOD);
      ah_invalid_argument_if(legendre != 1)
        << ctx << ": constant term " << a
        << " is not a quadratic residue modulo " << MOD;

      if (MOD % 4 == 3)
        {
          const uint64_t root = mod_exp(a, (MOD + 1) / 4, MOD);
          return std::min(root, root == 0 ? 0 : MOD - root);
        }

      uint64_t q = MOD - 1;
      size_t s = 0;
      while ((q & 1ULL) == 0)
        {
          q >>= 1;
          ++s;
        }

      uint64_t z = 2;
      while (mod_exp(z, (MOD - 1) / 2, MOD) != MOD - 1)
        ++z;

      uint64_t c = mod_exp(z, q, MOD);
      uint64_t r = mod_exp(a, (q + 1) / 2, MOD);
      uint64_t t = mod_exp(a, q, MOD);
      size_t m = s;

      while (t != 1)
        {
          size_t i = 1;
          uint64_t t2i = mod_mul(t, t, MOD);
          while (i < m and t2i != 1)
            {
              t2i = mod_mul(t2i, t2i, MOD);
              ++i;
            }

          ah_runtime_error_if(i == m)
            << ctx << ": Tonelli-Shanks failed to converge";

          uint64_t b = c;
          for (size_t j = 0; j + i + 1 < m; ++j)
            b = mod_mul(b, b, MOD);

          r = mod_mul(r, b, MOD);
          const uint64_t bb = mod_mul(b, b, MOD);
          t = mod_mul(t, bb, MOD);
          c = bb;
          m = i;
        }

      return std::min(r, r == 0 ? 0 : MOD - r);
    }

    [[nodiscard]] static Array<Array<uint64_t>>
    make_product_tree_storage(const size_t count)
    {
      Array<Array<uint64_t>> tree;
      const size_t capacity = count == 0 ? 0 : count * 4 + 4;
      tree.reserve(capacity);
      for (size_t i = 0; i < capacity; ++i)
        tree.append(Array<uint64_t>());
      return tree;
    }

    static void
    build_product_tree(Array<Array<uint64_t>> & tree,
                       const Array<uint64_t> & points,
                       const size_t node,
                       const size_t left,
                       const size_t right)
    {
      if (left + 1 == right)
        {
          tree(node) = {
            sub_mod(0, points[left] % MOD),
            1
          };
          return;
        }

      const size_t mid = left + (right - left) / 2;
      build_product_tree(tree, points, node << 1, left, mid);
      build_product_tree(tree, points, (node << 1) | 1, mid, right);
      tree(node) = multiply(tree[node << 1], tree[(node << 1) | 1]);
    }

    static void
    validate_distinct_points(const Array<uint64_t> & points,
                             const char * const ctx)
    {
      for (size_t i = 0; i < points.size(); ++i)
        for (size_t j = i + 1; j < points.size(); ++j)
          ah_invalid_argument_if(points[i] % MOD == points[j] % MOD)
            << ctx << ": points[" << i << "] and points[" << j
            << "] collide modulo " << MOD;
    }

    [[nodiscard]] static Array<uint64_t>
    poly_mod(const Array<uint64_t> & dividend,
             const Array<uint64_t> & divisor)
    {
      if (dividend.is_empty())
        return {};

      if (divisor.is_empty())
        return dividend;

      if (dividend.size() < divisor.size())
        return dividend;

      return poly_divmod(dividend, divisor).second;
    }

    static void
    multipoint_eval_recursive(const Array<Array<uint64_t>> & tree,
                              const Array<uint64_t> & poly,
                              Array<uint64_t> & output,
                              const size_t node,
                              const size_t left,
                              const size_t right)
    {
      if (left + 1 == right)
        {
          output(left) = poly.is_empty() ? 0 : poly[0] % MOD;
          return;
        }

      const size_t mid = left + (right - left) / 2;
      const Array<uint64_t> left_remainder =
          poly.size() < tree[node << 1].size() ?
            poly :
            poly_mod(poly, tree[node << 1]);
      const Array<uint64_t> right_remainder =
          poly.size() < tree[(node << 1) | 1].size() ?
            poly :
            poly_mod(poly, tree[(node << 1) | 1]);

      multipoint_eval_recursive(tree, left_remainder, output,
                                node << 1, left, mid);
      multipoint_eval_recursive(tree, right_remainder, output,
                                (node << 1) | 1, mid, right);
    }

    [[nodiscard]] static Array<uint64_t>
    interpolate_recursive(const Array<Array<uint64_t>> & tree,
                          const Array<uint64_t> & scaled_values,
                          const size_t node,
                          const size_t left,
                          const size_t right)
    {
      if (left + 1 == right)
        return Array<uint64_t>({scaled_values[left] % MOD});

      const size_t mid = left + (right - left) / 2;
      const Array<uint64_t> left_poly =
          interpolate_recursive(tree, scaled_values, node << 1, left, mid);
      const Array<uint64_t> right_poly =
          interpolate_recursive(tree, scaled_values, (node << 1) | 1, mid, right);

      return poly_add_normalized(
          multiply(left_poly, tree[(node << 1) | 1]),
          multiply(right_poly, tree[node << 1]));
    }

  public:

    /** @brief Maximum supported power-of-two transform size.
     *
     *  @return Largest `2^k` such that `2^k` divides `MOD - 1`.
     */
    [[nodiscard]] static constexpr uint64_t
    max_transform_size() noexcept
    {
      return max_transform_size_impl();
    }

    /** @brief Check whether a transform size is supported.
     *
     *  @param n Candidate transform size.
     *  @return `true` iff @p n is a positive power of two within the modulus
     *          limit.
     */
    [[nodiscard]] static constexpr bool
    supports_size(const size_t n) noexcept
    {
      return n > 0
             and (supports_power_of_two_size(n)
                  or supports_bluestein_size(n));
    }

    /** @brief Return an `n`-th primitive root of unity modulo @p MOD.
     *
     *  @param n Transform length. It must be a supported power of two.
     *  @return Primitive `n`-th root of unity in standard representation.
     *  @throws std::invalid_argument if @p n is not supported.
     */
    [[nodiscard]] static constexpr uint64_t
    primitive_root_of_unity(const size_t n)
    {
      if (not std::is_constant_evaluated())
        validate_supported_size(n, "NTT::primitive_root_of_unity");

      if (n <= 1)
        return 1;

      return primitive_root_of_order(static_cast<uint64_t>(n));
    }

    /** @brief In-place forward or inverse transform.
     *
     *  @param a Input/output array.
     *  @param invert If true, computes the inverse NTT.
     */
    static void
    transform(Array<uint64_t> & a,
              const bool invert)
    {
      Plan(a.size()).transform(a, invert);
    }

    /** @brief Transform an input array and return the result.
     *
     *  @param input Input array.
     *  @param invert If true, computes the inverse NTT.
     *  @return Transformed copy of @p input.
     */
    [[nodiscard]] static Array<uint64_t>
    transformed(const Array<uint64_t> & input,
                const bool invert = false)
    {
      Array<uint64_t> output = input;
      transform(output, invert);
      return output;
    }

    /** @brief Multiply two polynomials modulo @p MOD.
     *
     *  @param a First polynomial.
     *  @param b Second polynomial.
     *  @return Discrete convolution of @p a and @p b modulo @p MOD.
     *  @throws std::invalid_argument if the required transform size is not
     *          supported by the modulus.
     */
    [[nodiscard]] static Array<uint64_t>
    multiply(const Array<uint64_t> & a,
             const Array<uint64_t> & b)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      ah_invalid_argument_if(a.size() >
                             std::numeric_limits<size_t>::max()
                             - b.size() + 1)
        << "NTT::multiply: product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      const size_t n =
          supports_size(required) ?
            required :
            next_power_of_two(required, "NTT::multiply");
      validate_supported_size(n, "NTT::multiply");

      return Plan(n).multiply(a, b);
    }

    /** @brief Replace @p a by the product `a * b`.
     *
     *  @param a Left polynomial, overwritten with the product.
     *  @param b Right polynomial.
     */
    static void
    multiply_inplace(Array<uint64_t> & a,
                     const Array<uint64_t> & b)
    {
      a = multiply(a, b);
    }

    /** @brief Negacyclic convolution modulo `x^N + 1`.
     *
     *  This computes polynomial multiplication in the quotient ring
     *  `Z / MOD Z [x] / (x^N + 1)` using the classic twist/untwist reduction to
     *  an `N`-point NTT.  A primitive `2N`-th root of unity must therefore
     *  exist under the current modulus.
     *
     *  @param a First polynomial coefficients, size `N`.
     *  @param b Second polynomial coefficients, size `N`.
     *  @return Negacyclic product of size `N`.
     *  @throws std::invalid_argument if the inputs have different sizes, if
     *          `N` is not a positive power of two, or if `2N` is unsupported by
     *          the modulus.
     */
    [[nodiscard]] static Array<uint64_t>
    negacyclic_multiply(const Array<uint64_t> & a,
                        const Array<uint64_t> & b)
    {
      ah_invalid_argument_if(a.is_empty() or b.is_empty())
        << "NTT::negacyclic_multiply: inputs must have positive size";
      ah_invalid_argument_if(a.size() != b.size())
        << "NTT::negacyclic_multiply: lhs size " << a.size()
        << " does not match rhs size " << b.size();

      const size_t n = a.size();
      ah_invalid_argument_if(not is_power_of_two(n))
        << "NTT::negacyclic_multiply: size " << n
        << " is not a power of two";
      ah_invalid_argument_if(n > static_cast<size_t>(max_transform_size() / 2))
        << "NTT::negacyclic_multiply: size " << n
        << " requires a primitive root of order " << (n << 1)
        << ", but the largest supported power-of-two order is "
        << max_transform_size();

      const uint64_t twist = primitive_root_of_unity(n << 1);
      const uint64_t inv_twist = mod_inv(twist, MOD);
      Plan plan(n);

      Array<uint64_t> lhs = Array<uint64_t>::create(n);
      Array<uint64_t> rhs = Array<uint64_t>::create(n);
      uint64_t power = 1;
      for (size_t i = 0; i < n; ++i)
        {
          lhs(i) = mod_mul(a[i] % MOD, power, MOD);
          rhs(i) = mod_mul(b[i] % MOD, power, MOD);
          power = mod_mul(power, twist, MOD);
        }

      plan.transform(lhs, false);
      plan.transform(rhs, false);
      for (size_t i = 0; i < n; ++i)
        lhs(i) = mod_mul(lhs[i], rhs[i], MOD);
      plan.transform(lhs, true);

      power = 1;
      for (size_t i = 0; i < n; ++i)
        {
          lhs(i) = mod_mul(lhs[i], power, MOD);
          power = mod_mul(power, inv_twist, MOD);
        }

      return lhs;
    }

    /** @brief Sequential batch transform for equal-sized inputs.
     *
     *  @param batch Batch of arrays. All items must have the same supported
     *         size.
     *  @param invert If true, computes inverse transforms.
     */
    static void
    transform_batch(Array<Array<uint64_t>> & batch,
                    const bool invert)
    {
      if (batch.is_empty())
        return;

      Plan(batch[0].size()).transform_batch(batch, invert);
    }

    /** @brief Return a transformed copy of an entire batch.
     *
     *  @param batch Batch of arrays. All items must have the same supported
     *         size.
     *  @param invert If true, computes inverse transforms.
     *  @return Transformed batch.
     */
    [[nodiscard]] static Array<Array<uint64_t>>
    transformed_batch(const Array<Array<uint64_t>> & batch,
                      const bool invert = false)
    {
      if (batch.is_empty())
        return {};

      return Plan(batch[0].size()).transformed_batch(batch, invert);
    }

    /** @brief Parallel in-place transform using a ThreadPool.
     *
     *  @param pool Thread pool used for stage-level parallelism.
     *  @param a Input/output array.
     *  @param invert If true, computes the inverse NTT.
     *  @param chunk_size Iterations per scheduled chunk (0 selects an
     *         automatic value).
     */
    static void
    ptransform(ThreadPool & pool,
               Array<uint64_t> & a,
               const bool invert,
               const size_t chunk_size = 0)
    {
      Plan(a.size()).ptransform(pool, a, invert, chunk_size);
    }

    /** @brief Parallel transform returning a new array.
     *
     *  @param pool Thread pool used for stage-level parallelism.
     *  @param input Input array.
     *  @param invert If true, computes the inverse NTT.
     *  @param chunk_size Iterations per scheduled chunk (0 selects an
     *         automatic value).
     *  @return Transformed copy of @p input.
     */
    [[nodiscard]] static Array<uint64_t>
    ptransformed(ThreadPool & pool,
                 const Array<uint64_t> & input,
                 const bool invert = false,
                 const size_t chunk_size = 0)
    {
      Array<uint64_t> output = input;
      ptransform(pool, output, invert, chunk_size);
      return output;
    }

    /** @brief Parallel polynomial multiplication modulo @p MOD.
     *
     *  @param pool Thread pool used for transform stages and pointwise
     *         products.
     *  @param a First polynomial.
     *  @param b Second polynomial.
     *  @param chunk_size Iterations per scheduled chunk (0 selects an
     *         automatic value).
     *  @return Discrete convolution of @p a and @p b modulo @p MOD.
     */
    [[nodiscard]] static Array<uint64_t>
    pmultiply(ThreadPool & pool,
              const Array<uint64_t> & a,
              const Array<uint64_t> & b,
              const size_t chunk_size = 0)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      ah_invalid_argument_if(a.size() >
                             std::numeric_limits<size_t>::max()
                             - b.size() + 1)
        << "NTT::pmultiply: product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      const size_t n =
          supports_size(required) ?
            required :
            next_power_of_two(required, "NTT::pmultiply");
      validate_supported_size(n, "NTT::pmultiply");

      return Plan(n).pmultiply(pool, a, b, chunk_size);
    }

    /** @brief Parallel batch transform for equal-sized inputs.
     *
     *  @param pool Thread pool used for execution.
     *  @param batch Batch of arrays. All items must have the same supported
     *         size.
     *  @param invert If true, computes inverse transforms.
     *  @param chunk_size Iterations per scheduled chunk (0 selects an
     *         automatic value).
     */
    static void
    ptransform_batch(ThreadPool & pool,
                     Array<Array<uint64_t>> & batch,
                     const bool invert,
                     const size_t chunk_size = 0)
    {
      if (batch.is_empty())
        return;

      Plan(batch[0].size()).ptransform_batch(pool, batch, invert, chunk_size);
    }

    /** @brief Evaluate a polynomial at a single point modulo @p MOD.
     *
     *  @param coeffs Polynomial coefficients in ascending degree order.
     *  @param x Evaluation point.
     *  @return `sum coeffs[i] * x^i mod MOD`.
     */
    [[nodiscard]] static uint64_t
    poly_eval(const Array<uint64_t> & coeffs,
              const uint64_t x)
    {
      uint64_t value = 0;
      const uint64_t x_mod = x % MOD;
      for (size_t i = coeffs.size(); i > 0; --i)
        {
          value = mod_mul(value, x_mod, MOD);
          value = add_mod(value, coeffs[i - 1] % MOD);
        }
      return value;
    }

    /** @brief Formal polynomial inverse modulo `x^n`.
     *
     *  @param coeffs Input polynomial.
     *  @param n Number of coefficients to compute.
     *  @return `g` such that `coeffs * g ≡ 1 (mod x^n, MOD)`.
     *  @throws std::invalid_argument if the constant term is not invertible.
     */
    [[nodiscard]] static Array<uint64_t>
    poly_inverse(const Array<uint64_t> & coeffs,
                 const size_t n)
    {
      if (n == 0)
        return {};

      ah_invalid_argument_if(coeffs.is_empty())
        << "NTT::poly_inverse: input polynomial must be non-empty";

      const uint64_t c0 = coeffs[0] % MOD;
      ah_invalid_argument_if(c0 == 0)
        << "NTT::poly_inverse: constant term must be invertible modulo " << MOD;

      Array<uint64_t> inverse = Array<uint64_t>::create(1);
      inverse(0) = mod_inv(c0, MOD);

      size_t m = 1;
      while (m < n)
        {
          const size_t m2 = std::min(n, m << 1);
          const Array<uint64_t> fg =
              poly_mul_trunc(truncate_poly(coeffs, m2), inverse, m2);
          Array<uint64_t> correction = zero_series(m2);
          correction(0) = sub_mod(2 % MOD, fg[0]);
          for (size_t i = 1; i < m2; ++i)
            correction(i) = fg[i] == 0 ? 0 : MOD - fg[i];
          inverse = poly_mul_trunc(inverse, correction, m2);
          m = m2;
        }

      return series_prefix(inverse, n);
    }

    /** @brief Polynomial division with remainder modulo @p MOD.
     *
     *  @param dividend Dividend coefficients in ascending degree order.
     *  @param divisor Divisor coefficients in ascending degree order.
     *  @return `{quotient, remainder}` with `remainder.degree < divisor.degree`.
     *  @throws std::invalid_argument if @p divisor is the zero polynomial.
     */
    [[nodiscard]] static std::pair<Array<uint64_t>, Array<uint64_t>>
    poly_divmod(const Array<uint64_t> & dividend,
                const Array<uint64_t> & divisor)
    {
      const Array<uint64_t> a = normalize_poly(dividend);
      const Array<uint64_t> b = normalize_poly(divisor);

      ah_invalid_argument_if(b.is_empty())
        << "NTT::poly_divmod: divisor cannot be the zero polynomial";

      if (a.is_empty() or a.size() < b.size())
        return {{}, a};

      const size_t quotient_size = a.size() - b.size() + 1;
      const Array<uint64_t> q_rev = poly_mul_trunc(
          reverse_poly(a),
          poly_inverse(reverse_poly(b), quotient_size),
          quotient_size);
      Array<uint64_t> quotient = reverse_poly(q_rev);
      trim_trailing_zeros(quotient);

      Array<uint64_t> remainder =
          poly_sub_normalized(a, multiply(b, quotient));
      if (remainder.size() >= b.size())
        remainder = truncate_poly(remainder, b.size() - 1);
      trim_trailing_zeros(remainder);
      return {quotient, remainder};
    }

    /** @brief Formal polynomial logarithm modulo `x^n`.
     *
     *  @param coeffs Input polynomial with constant term 1.
     *  @param n Number of coefficients to compute.
     *  @return `ln(coeffs) mod x^n`.
     *  @throws std::invalid_argument if `coeffs[0] != 1 (mod MOD)`.
     */
    [[nodiscard]] static Array<uint64_t>
    poly_log(const Array<uint64_t> & coeffs,
             const size_t n)
    {
      if (n == 0)
        return {};

      const uint64_t c0 = coeffs.is_empty() ? 0 : coeffs[0] % MOD;
      ah_invalid_argument_if(c0 != 1)
        << "NTT::poly_log: constant term must be 1 modulo " << MOD;

      if (n == 1)
        return zero_series(1);

      const Array<uint64_t> derivative =
          poly_derivative(truncate_poly(coeffs, n));
      const Array<uint64_t> inverse = poly_inverse(coeffs, n - 1);
      return series_prefix(poly_integral(
          poly_mul_trunc(derivative, inverse, n - 1)), n);
    }

    /** @brief Formal polynomial exponential modulo `x^n`.
     *
     *  @param coeffs Input polynomial with zero constant term.
     *  @param n Number of coefficients to compute.
     *  @return `exp(coeffs) mod x^n`.
     *  @throws std::invalid_argument if `coeffs[0] != 0 (mod MOD)`.
     */
    [[nodiscard]] static Array<uint64_t>
    poly_exp(const Array<uint64_t> & coeffs,
             const size_t n)
    {
      if (n == 0)
        return {};

      const uint64_t c0 = coeffs.is_empty() ? 0 : coeffs[0] % MOD;
      ah_invalid_argument_if(c0 != 0)
        << "NTT::poly_exp: constant term must be 0 modulo " << MOD;

      Array<uint64_t> result = Array<uint64_t>::create(1);
      result(0) = 1;

      size_t m = 1;
      while (m < n)
        {
          const size_t m2 = std::min(n, m << 1);
          Array<uint64_t> delta =
              poly_sub_series(series_prefix(coeffs, m2),
                              poly_log(result, m2), m2);
          delta(0) = add_mod(delta[0], 1);
          result = poly_mul_trunc(result, delta, m2);
          m = m2;
        }

      return series_prefix(result, n);
    }

    /** @brief Formal polynomial square root modulo `x^n`.
     *
     *  @param coeffs Input polynomial.
     *  @param n Number of coefficients to compute.
     *  @return `g` such that `g^2 ≡ coeffs (mod x^n, MOD)`.
     *  @throws std::invalid_argument if no square root exists under the stated
     *          preconditions.
     */
    [[nodiscard]] static Array<uint64_t>
    poly_sqrt(const Array<uint64_t> & coeffs,
              const size_t n)
    {
      if (n == 0)
        return {};

      const Array<uint64_t> input = truncate_poly(coeffs, n);
      size_t lead = 0;
      while (lead < input.size() and input[lead] % MOD == 0)
        ++lead;

      if (lead == input.size())
        return zero_series(n);

      ah_invalid_argument_if((lead & 1U) != 0)
        << "NTT::poly_sqrt: first non-zero term appears at odd degree "
        << lead;

      if (lead > 0)
        {
          const size_t shift = lead / 2;
          Array<uint64_t> tail;
          tail.reserve(input.size() - lead);
          for (size_t i = lead; i < input.size(); ++i)
            tail.append(input[i] % MOD);

          const Array<uint64_t> rooted = poly_sqrt(tail, n - shift);
          Array<uint64_t> output = zero_series(n);
          for (size_t i = 0; i < rooted.size() and i + shift < n; ++i)
            output(i + shift) = rooted[i];
          return output;
        }

      Array<uint64_t> result = Array<uint64_t>::create(1);
      result(0) = tonelli_shanks(input[0], "NTT::poly_sqrt");

      const uint64_t inv_two = mod_inv(2, MOD);
      size_t m = 1;
      while (m < n)
        {
          const size_t m2 = std::min(n, m << 1);
          const Array<uint64_t> quotient =
              poly_mul_trunc(series_prefix(input, m2),
                             poly_inverse(result, m2), m2);
          result = poly_scalar_mul_series(
              poly_add_series(result, quotient, m2), inv_two, m2);
          m = m2;
        }

      return series_prefix(result, n);
    }

    /** @brief Formal polynomial power modulo `x^n`.
     *
     *  @param coeffs Base polynomial.
     *  @param k Non-negative exponent.
     *  @param n Number of coefficients to compute.
     *  @return `coeffs^k mod x^n`.
     */
    [[nodiscard]] static Array<uint64_t>
    poly_power(const Array<uint64_t> & coeffs,
               const uint64_t k,
               const size_t n)
    {
      if (n == 0)
        return {};

      if (k == 0)
        {
          Array<uint64_t> output = zero_series(n);
          output(0) = 1;
          return output;
        }

      const Array<uint64_t> input = truncate_poly(coeffs, n);
      size_t lead = 0;
      while (lead < input.size() and input[lead] % MOD == 0)
        ++lead;

      if (lead == input.size())
        return zero_series(n);

      const __uint128_t shift128 =
          static_cast<__uint128_t>(lead) * static_cast<__uint128_t>(k);
      if (shift128 >= n)
        return zero_series(n);

      const size_t shift = static_cast<size_t>(shift128);
      const size_t target = n - shift;
      const uint64_t lead_coeff = input[lead] % MOD;
      const uint64_t inv_lead = mod_inv(lead_coeff, MOD);

      Array<uint64_t> normalized;
      normalized.reserve(input.size() - lead);
      for (size_t i = lead; i < input.size(); ++i)
        normalized.append(mod_mul(input[i] % MOD, inv_lead, MOD));

      Array<uint64_t> scaled_log = poly_log(normalized, target);
      for (size_t i = 0; i < scaled_log.size(); ++i)
        scaled_log(i) = mod_mul(scaled_log[i],
                                static_cast<uint64_t>(k % MOD), MOD);

      Array<uint64_t> powered = poly_exp(scaled_log, target);
      powered = poly_scalar_mul_series(powered, mod_exp(lead_coeff, k, MOD),
                                       target);

      Array<uint64_t> output = zero_series(n);
      for (size_t i = 0; i < powered.size() and i + shift < n; ++i)
        output(i + shift) = powered[i];
      return output;
    }

    /** @brief Evaluate a polynomial on multiple points modulo @p MOD.
     *
     *  @param coeffs Polynomial coefficients.
     *  @param points Evaluation points.
     *  @return Values `coeffs(points[i]) mod MOD`.
     */
    [[nodiscard]] static Array<uint64_t>
    multipoint_eval(const Array<uint64_t> & coeffs,
                    const Array<uint64_t> & points)
    {
      if (points.is_empty())
        return {};

      Array<uint64_t> reduced_points = Array<uint64_t>::create(points.size());
      for (size_t i = 0; i < points.size(); ++i)
        reduced_points(i) = points[i] % MOD;

      Array<Array<uint64_t>> tree = make_product_tree_storage(points.size());
      build_product_tree(tree, reduced_points, 1, 0, reduced_points.size());

      Array<uint64_t> output = Array<uint64_t>::create(points.size());
      for (size_t i = 0; i < output.size(); ++i)
        output(i) = 0;

      multipoint_eval_recursive(tree, normalize_poly(coeffs), output,
                                1, 0, reduced_points.size());
      return output;
    }

    /** @brief Interpolate a polynomial from point-value samples modulo @p MOD.
     *
     *  @param points Sample points.
     *  @param values Sample values.
     *  @return Unique polynomial of degree `< points.size()` passing through
     *          all pairs.
     *  @throws std::invalid_argument if the points collide modulo @p MOD or
     *          the arrays have different sizes.
     */
    [[nodiscard]] static Array<uint64_t>
    interpolate(const Array<uint64_t> & points,
                const Array<uint64_t> & values)
    {
      ah_invalid_argument_if(points.size() != values.size())
        << "NTT::interpolate: points size " << points.size()
        << " does not match values size " << values.size();

      if (points.is_empty())
        return {};

      Array<uint64_t> reduced_points = Array<uint64_t>::create(points.size());
      Array<uint64_t> reduced_values = Array<uint64_t>::create(values.size());
      for (size_t i = 0; i < points.size(); ++i)
        {
          reduced_points(i) = points[i] % MOD;
          reduced_values(i) = values[i] % MOD;
        }

      validate_distinct_points(reduced_points, "NTT::interpolate");

      Array<Array<uint64_t>> tree = make_product_tree_storage(points.size());
      build_product_tree(tree, reduced_points, 1, 0, reduced_points.size());

      const Array<uint64_t> total_derivative = poly_derivative(tree[1]);
      const Array<uint64_t> weights =
          multipoint_eval(total_derivative, reduced_points);

      Array<uint64_t> scaled_values = Array<uint64_t>::create(values.size());
      for (size_t i = 0; i < values.size(); ++i)
        {
          ah_invalid_argument_if(weights[i] == 0)
            << "NTT::interpolate: derivative vanished at point index " << i;
          scaled_values(i) = mod_mul(reduced_values[i],
                                     mod_inv(weights[i], MOD), MOD);
        }

      return normalize_poly(interpolate_recursive(tree, scaled_values,
                                                  1, 0, reduced_points.size()));
    }

    /** @brief Multiply two non-negative integers represented as base-@p Base digits.
     *
     *  Digits are stored in little-endian order: `a[0]` is the least
     *  significant digit.  The result is normalized by trimming high zero
     *  digits, except that zero is returned as a single digit `{0}`.
     *
     *  The implementation selects an exact convolution path automatically:
     *
     *  - if `min(a.size(), b.size()) * (Base - 1)^2 < MOD` and the transform
     *    size is supported by this `NTT<MOD, ROOT>`, a single-prime exact
     *    multiplication is used
     *  - otherwise it falls back to `NTTExact`, then propagates carries
     *
     *  @tparam Base Digit base, must be at least 2.
     *  @param a First integer digits.
     *  @param b Second integer digits.
     *  @return Exact product digits in base @p Base.
     *  @throws std::invalid_argument if some input digit is outside
     *          `[0, Base)` or if no exact backend can support the requested
     *          convolution.
     */
    template <uint64_t Base = (1ULL << 15)>
    [[nodiscard]] static Array<uint64_t>
    bigint_multiply(const Array<uint64_t> & a,
                    const Array<uint64_t> & b);

    /** @brief Parallel big-integer multiplication in base @p Base.
     *
     *  The expensive convolution runs in parallel using the best exact backend
     *  selected by @ref bigint_multiply; carry propagation itself remains a
     *  linear pass.
     *
     *  @tparam Base Digit base, must be at least 2.
     *  @param pool Thread pool used for the convolution backend.
     *  @param a First integer digits.
     *  @param b Second integer digits.
     *  @param chunk_size Parallel chunk size forwarded to the backend.
     *  @return Exact product digits in base @p Base.
     *  @throws std::invalid_argument if some input digit is outside
     *          `[0, Base)` or if no exact backend can support the requested
     *          convolution.
     */
    template <uint64_t Base = (1ULL << 15)>
    [[nodiscard]] static Array<uint64_t>
    pbigint_multiply(ThreadPool & pool,
                     const Array<uint64_t> & a,
                     const Array<uint64_t> & b,
                     const size_t chunk_size = 0);
  };

  /** @brief Metadata for an NTT-friendly prime used in CRT reconstruction.
   *
   *  The field @ref max_power_of_two stores the largest exponent `k` such that
   *  `2^k` divides `mod - 1`.
   */
  struct NTTPrime
  {
    uint64_t mod = 0;
    uint64_t root = 0;
    uint64_t max_power_of_two = 0;
  };

  /** @brief Exact polynomial multiplication via three-prime NTT + CRT.
   *
   *  This helper reconstructs coefficients in `__uint128_t` using three
   *  standard NTT-friendly primes:
   *
   *  - `998244353 = 119 * 2^23 + 1`
   *  - `469762049 = 7 * 2^26 + 1`
   *  - `1004535809 = 479 * 2^21 + 1`
   *
   *  The result is exact as long as every coefficient of the true convolution
   *  lies in `[0, P)`, where `P` is the product of the three moduli.  Before
   *  executing the transforms, the implementation applies conservative
   *  sufficient bounds and rejects inputs that cannot be proven to fit inside
   *  that CRT range.
   */
  class NTTExact
  {
  public:
    using coeff_type = __uint128_t;

  private:
    using Prime0NTT = NTT<998244353ULL, 3ULL>;
    using Prime1NTT = NTT<469762049ULL, 3ULL>;
    using Prime2NTT = NTT<1004535809ULL, 3ULL>;

    struct CoefficientStats
    {
      uint64_t max_value = 0;
      size_t non_zero = 0;
      coeff_type sum = 0;
    };

    static constexpr NTTPrime primes_[] = {
      {998244353ULL, 3ULL, 23ULL},
      {469762049ULL, 3ULL, 26ULL},
      {1004535809ULL, 3ULL, 21ULL}
    };

    [[nodiscard]] static constexpr coeff_type
    exact_modulus_product_impl() noexcept
    {
      return static_cast<coeff_type>(primes_[0].mod)
             * static_cast<coeff_type>(primes_[1].mod)
             * static_cast<coeff_type>(primes_[2].mod);
    }

    [[nodiscard]] static constexpr uint64_t
    sub_mod(const uint64_t lhs,
            const uint64_t rhs,
            const uint64_t mod) noexcept
    {
      return lhs >= rhs ? lhs - rhs : mod - (rhs - lhs);
    }

    [[nodiscard]] static constexpr coeff_type
    add_capped(const coeff_type lhs,
               const coeff_type rhs,
               const coeff_type cap) noexcept
    {
      return lhs >= cap - rhs ? cap : lhs + rhs;
    }

    [[nodiscard]] static constexpr coeff_type
    mul_capped(const coeff_type lhs,
               const coeff_type rhs,
               const coeff_type cap) noexcept
    {
      if (lhs == 0 or rhs == 0)
        return 0;
      return lhs > cap / rhs ? cap : lhs * rhs;
    }

    [[nodiscard]] static constexpr size_t
    next_power_of_two(const size_t n) noexcept
    {
      if (n <= 1)
        return 1;

      size_t value = 1;
      while (value < n)
        {
          if (value > std::numeric_limits<size_t>::max() / 2)
            return 0;
          value <<= 1;
        }

      return value;
    }

    template <typename PrimeNTT>
    [[nodiscard]] static constexpr bool
    prime_supports_product_size(const size_t required) noexcept
    {
      if (required == 0)
        return false;

      if (PrimeNTT::supports_size(required))
        return true;

      const size_t n = next_power_of_two(required);
      return n != 0 and PrimeNTT::supports_size(n);
    }

    [[nodiscard]] static std::string
    coeff_to_string(coeff_type value)
    {
      if (value == 0)
        return "0";

      std::string digits;
      while (value > 0)
        {
          const auto digit = static_cast<unsigned>(value % 10);
          digits.push_back(static_cast<char>('0' + digit));
          value /= 10;
        }

      std::reverse(digits.begin(), digits.end());
      return digits;
    }

    [[nodiscard]] static CoefficientStats
    analyze_coefficients(const Array<uint64_t> & input)
    {
      CoefficientStats stats;
      const coeff_type cap = exact_modulus_product();
      for (size_t i = 0; i < input.size(); ++i)
        {
          const uint64_t value = input[i];
          if (value == 0)
            continue;

          ++stats.non_zero;
          if (value > stats.max_value)
            stats.max_value = value;
          stats.sum = add_capped(stats.sum, static_cast<coeff_type>(value), cap);
        }

      return stats;
    }

    [[nodiscard]] static coeff_type
    conservative_bound(const Array<uint64_t> & a,
                       const Array<uint64_t> & b)
    {
      if (a.is_empty() or b.is_empty())
        return 0;

      const coeff_type cap = exact_modulus_product();
      const CoefficientStats lhs = analyze_coefficients(a);
      const CoefficientStats rhs = analyze_coefficients(b);

      if (lhs.non_zero == 0 or rhs.non_zero == 0)
        return 0;

      coeff_type bound = cap;

      const coeff_type max_product =
          mul_capped(static_cast<coeff_type>(lhs.max_value),
                     static_cast<coeff_type>(rhs.max_value), cap);

      const coeff_type overlap_bound =
          mul_capped(static_cast<coeff_type>(std::min(lhs.non_zero, rhs.non_zero)),
                     max_product, cap);
      if (overlap_bound < bound)
        bound = overlap_bound;

      const coeff_type lhs_sum_bound =
          mul_capped(lhs.sum, static_cast<coeff_type>(rhs.max_value), cap);
      if (lhs_sum_bound < bound)
        bound = lhs_sum_bound;

      const coeff_type rhs_sum_bound =
          mul_capped(rhs.sum, static_cast<coeff_type>(lhs.max_value), cap);
      if (rhs_sum_bound < bound)
        bound = rhs_sum_bound;

      return bound;
    }

    static void
    validate_inputs(const Array<uint64_t> & a,
                    const Array<uint64_t> & b,
                    const char * const ctx)
    {
      ah_invalid_argument_if(a.size() >
                             std::numeric_limits<size_t>::max()
                             - b.size() + 1)
        << ctx << ": product size exceeds size_t capacity";

      const size_t required = a.size() + b.size() - 1;
      ah_invalid_argument_if(not supports_product_size(required))
        << ctx << ": required product size " << required
        << " is not supported by the three-prime CRT pack";

      const coeff_type bound = conservative_bound(a, b);
      ah_invalid_argument_if(bound >= exact_modulus_product())
        << ctx << ": cannot guarantee exact reconstruction inside CRT range "
        << coeff_to_string(exact_modulus_product())
        << " with conservative coefficient bound "
        << coeff_to_string(bound);
    }

    [[nodiscard]] static coeff_type
    reconstruct_coefficient(const uint64_t r0,
                            const uint64_t r1,
                            const uint64_t r2)
    {
      static const uint64_t m0 = primes_[0].mod;
      static const uint64_t m1 = primes_[1].mod;
      static const uint64_t m2 = primes_[2].mod;
      static const coeff_type m0m1 =
          static_cast<coeff_type>(m0) * static_cast<coeff_type>(m1);
      static const uint64_t m0_inv_mod_m1 = mod_inv(m0 % m1, m1);
      static const uint64_t m0m1_inv_mod_m2 =
          mod_inv(static_cast<uint64_t>(m0m1 % m2), m2);

      const uint64_t t1 =
          mod_mul(sub_mod(r1, r0 % m1, m1), m0_inv_mod_m1, m1);
      const coeff_type x01 =
          static_cast<coeff_type>(r0)
          + static_cast<coeff_type>(t1) * static_cast<coeff_type>(m0);

      const uint64_t x01_mod_m2 = static_cast<uint64_t>(x01 % m2);
      const uint64_t t2 =
          mod_mul(sub_mod(r2, x01_mod_m2, m2), m0m1_inv_mod_m2, m2);

      return x01 + static_cast<coeff_type>(t2) * m0m1;
    }

    [[nodiscard]] static Array<coeff_type>
    reconstruct_product(const Array<uint64_t> & c0,
                        const Array<uint64_t> & c1,
                        const Array<uint64_t> & c2,
                        ThreadPool * const pool,
                        const size_t chunk_size)
    {
      ah_runtime_error_unless(c0.size() == c1.size() and c1.size() == c2.size())
        << "NTTExact::reconstruct_product: inconsistent residue sizes";

      Array<coeff_type> output = Array<coeff_type>::create(c0.size());

      auto reconstruct_one = [&output, &c0, &c1, &c2](const size_t i)
        {
          output(i) = reconstruct_coefficient(c0[i], c1[i], c2[i]);
        };

      if (pool != nullptr and pool->num_threads() > 1 and c0.size() > 1)
        parallel_for_index(*pool, 0, c0.size(), reconstruct_one, chunk_size);
      else
        for (size_t i = 0; i < c0.size(); ++i)
          reconstruct_one(i);

      return output;
    }

  public:
    /** @brief Number of CRT primes in the exact multiplier. */
    [[nodiscard]] static constexpr size_t
    prime_count() noexcept
    {
      return sizeof(primes_) / sizeof(primes_[0]);
    }

    /** @brief Product of the three CRT moduli.
     *
     *  Every exact result coefficient must be strictly smaller than this
     *  value for the reconstruction to be guaranteed.
     */
    [[nodiscard]] static constexpr coeff_type
    exact_modulus_product() noexcept
    {
      return exact_modulus_product_impl();
    }

    /** @brief Check whether a target product length is supported.
     *
     *  The length refers to the final convolution length `a.size() + b.size()
     *  - 1`.  Each internal prime transform may still use Bluestein or a
     *  larger power-of-two staging length.
     */
    [[nodiscard]] static constexpr bool
    supports_product_size(const size_t required) noexcept
    {
      return prime_supports_product_size<Prime0NTT>(required)
             and prime_supports_product_size<Prime1NTT>(required)
             and prime_supports_product_size<Prime2NTT>(required);
    }

    /** @brief Exact sequential polynomial multiplication.
     *
     *  @param a First polynomial with non-negative `uint64_t` coefficients.
     *  @param b Second polynomial with non-negative `uint64_t` coefficients.
     *  @return Exact convolution reconstructed in `__uint128_t`.
     *  @throws std::invalid_argument if the transform size is unsupported or
     *          if the inputs cannot be proven to fit in the CRT reconstruction
     *          range.
     */
    [[nodiscard]] static Array<coeff_type>
    multiply(const Array<uint64_t> & a,
             const Array<uint64_t> & b)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      validate_inputs(a, b, "NTTExact::multiply");

      const Array<uint64_t> c0 = Prime0NTT::multiply(a, b);
      const Array<uint64_t> c1 = Prime1NTT::multiply(a, b);
      const Array<uint64_t> c2 = Prime2NTT::multiply(a, b);
      return reconstruct_product(c0, c1, c2, nullptr, 0);
    }

    /** @brief Exact parallel polynomial multiplication.
     *
     *  Parallelism is applied across the independent prime transforms and then
     *  across CRT reconstruction when the supplied pool has more than one
     *  worker.
     *
     *  @param pool Thread pool used for execution.
     *  @param a First polynomial with non-negative `uint64_t` coefficients.
     *  @param b Second polynomial with non-negative `uint64_t` coefficients.
     *  @param chunk_size Reconstruction chunk size (0 selects automatic
     *         chunking).
     *  @return Exact convolution reconstructed in `__uint128_t`.
     *  @throws std::invalid_argument if the transform size is unsupported or
     *          if the inputs cannot be proven to fit in the CRT reconstruction
     *          range.
     */
    [[nodiscard]] static Array<coeff_type>
    pmultiply(ThreadPool & pool,
              const Array<uint64_t> & a,
              const Array<uint64_t> & b,
              const size_t chunk_size = 0)
    {
      if (a.is_empty() or b.is_empty())
        return {};

      validate_inputs(a, b, "NTTExact::pmultiply");

      if (pool.num_threads() <= 1)
        return multiply(a, b);

      auto f0 = pool.enqueue([&a, &b]()
                             {
                               return Prime0NTT::multiply(a, b);
                             });
      auto f1 = pool.enqueue([&a, &b]()
                             {
                               return Prime1NTT::multiply(a, b);
                             });
      auto f2 = pool.enqueue([&a, &b]()
                             {
                               return Prime2NTT::multiply(a, b);
                             });

      const Array<uint64_t> c0 = f0.get();
      const Array<uint64_t> c1 = f1.get();
      const Array<uint64_t> c2 = f2.get();
      return reconstruct_product(c0, c1, c2, &pool, chunk_size);
    }
  };

  template <uint64_t MOD, uint64_t ROOT>
  template <uint64_t Base>
  Array<uint64_t>
  NTT<MOD, ROOT>::bigint_multiply(const Array<uint64_t> & a,
                                  const Array<uint64_t> & b)
  {
    static_assert(Base > 1, "NTT::bigint_multiply requires Base >= 2");
    using ExactCoeff = NTTExact::coeff_type;

    auto zero_digits = []()
      {
        Array<uint64_t> output = Array<uint64_t>::create(1);
        output(0) = 0;
        return output;
      };

    auto validate_digits = [](const Array<uint64_t> & digits,
                              const char * const name,
                              const char * const ctx)
      {
        for (size_t i = 0; i < digits.size(); ++i)
          ah_invalid_argument_if(digits[i] >= Base)
            << ctx << ": " << name << "[" << i << "] = " << digits[i]
            << " is not in [0, " << Base << ")";
      };

    auto normalize_digits = [](const Array<uint64_t> & input)
      {
        Array<uint64_t> output;
        output.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i)
          output.append(input[i]);

        while (not output.is_empty() and output.get_last() == 0)
          static_cast<void>(output.remove_last());
        return output;
      };

    auto propagate_carries = [&zero_digits](const Array<ExactCoeff> & coeffs)
      {
        if (coeffs.is_empty())
          return zero_digits();

        Array<uint64_t> output;
        output.reserve(coeffs.size() + 2);

        ExactCoeff carry = 0;
        for (size_t i = 0; i < coeffs.size(); ++i)
          {
            const ExactCoeff total = coeffs[i] + carry;
            output.append(static_cast<uint64_t>(total % Base));
            carry = total / Base;
          }

        while (carry > 0)
          {
            output.append(static_cast<uint64_t>(carry % Base));
            carry /= Base;
          }

        while (output.size() > 1 and output.get_last() == 0)
          static_cast<void>(output.remove_last());

        return output.is_empty() ? zero_digits() : output;
      };

    validate_digits(a, "a", "NTT::bigint_multiply");
    validate_digits(b, "b", "NTT::bigint_multiply");

    const Array<uint64_t> lhs = normalize_digits(a);
    const Array<uint64_t> rhs = normalize_digits(b);
    if (lhs.is_empty() or rhs.is_empty())
      return zero_digits();

    ah_invalid_argument_if(lhs.size() >
                           std::numeric_limits<size_t>::max()
                           - rhs.size() + 1)
      << "NTT::bigint_multiply: product size exceeds size_t capacity";

    const size_t required = lhs.size() + rhs.size() - 1;
    const size_t internal_size =
        supports_size(required) ?
          required :
          next_power_of_two(required, "NTT::bigint_multiply");

    const ExactCoeff digit_max = static_cast<ExactCoeff>(Base - 1);
    const ExactCoeff single_prime_bound =
        static_cast<ExactCoeff>(std::min(lhs.size(), rhs.size()))
        * digit_max * digit_max;

    if (single_prime_bound < static_cast<ExactCoeff>(MOD)
        and supports_size(internal_size))
      {
        const Array<uint64_t> coeffs = multiply(lhs, rhs);
        Array<ExactCoeff> exact = Array<ExactCoeff>::create(coeffs.size());
        for (size_t i = 0; i < coeffs.size(); ++i)
          exact(i) = static_cast<ExactCoeff>(coeffs[i]);
        return propagate_carries(exact);
      }

    return propagate_carries(NTTExact::multiply(lhs, rhs));
  }

  template <uint64_t MOD, uint64_t ROOT>
  template <uint64_t Base>
  Array<uint64_t>
  NTT<MOD, ROOT>::pbigint_multiply(ThreadPool & pool,
                                   const Array<uint64_t> & a,
                                   const Array<uint64_t> & b,
                                   const size_t chunk_size)
  {
    static_assert(Base > 1, "NTT::pbigint_multiply requires Base >= 2");
    using ExactCoeff = NTTExact::coeff_type;

    auto zero_digits = []()
      {
        Array<uint64_t> output = Array<uint64_t>::create(1);
        output(0) = 0;
        return output;
      };

    auto validate_digits = [](const Array<uint64_t> & digits,
                              const char * const name,
                              const char * const ctx)
      {
        for (size_t i = 0; i < digits.size(); ++i)
          ah_invalid_argument_if(digits[i] >= Base)
            << ctx << ": " << name << "[" << i << "] = " << digits[i]
            << " is not in [0, " << Base << ")";
      };

    auto normalize_digits = [](const Array<uint64_t> & input)
      {
        Array<uint64_t> output;
        output.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i)
          output.append(input[i]);

        while (not output.is_empty() and output.get_last() == 0)
          static_cast<void>(output.remove_last());
        return output;
      };

    auto propagate_carries = [&zero_digits](const Array<ExactCoeff> & coeffs)
      {
        if (coeffs.is_empty())
          return zero_digits();

        Array<uint64_t> output;
        output.reserve(coeffs.size() + 2);

        ExactCoeff carry = 0;
        for (size_t i = 0; i < coeffs.size(); ++i)
          {
            const ExactCoeff total = coeffs[i] + carry;
            output.append(static_cast<uint64_t>(total % Base));
            carry = total / Base;
          }

        while (carry > 0)
          {
            output.append(static_cast<uint64_t>(carry % Base));
            carry /= Base;
          }

        while (output.size() > 1 and output.get_last() == 0)
          static_cast<void>(output.remove_last());

        return output.is_empty() ? zero_digits() : output;
      };

    validate_digits(a, "a", "NTT::pbigint_multiply");
    validate_digits(b, "b", "NTT::pbigint_multiply");

    const Array<uint64_t> lhs = normalize_digits(a);
    const Array<uint64_t> rhs = normalize_digits(b);
    if (lhs.is_empty() or rhs.is_empty())
      return zero_digits();

    ah_invalid_argument_if(lhs.size() >
                           std::numeric_limits<size_t>::max()
                           - rhs.size() + 1)
      << "NTT::pbigint_multiply: product size exceeds size_t capacity";

    const size_t required = lhs.size() + rhs.size() - 1;
    const size_t internal_size =
        supports_size(required) ?
          required :
          next_power_of_two(required, "NTT::pbigint_multiply");

    const ExactCoeff digit_max = static_cast<ExactCoeff>(Base - 1);
    const ExactCoeff single_prime_bound =
        static_cast<ExactCoeff>(std::min(lhs.size(), rhs.size()))
        * digit_max * digit_max;

    if (single_prime_bound < static_cast<ExactCoeff>(MOD)
        and supports_size(internal_size))
      {
        const Array<uint64_t> coeffs = pmultiply(pool, lhs, rhs, chunk_size);
        Array<ExactCoeff> exact = Array<ExactCoeff>::create(coeffs.size());
        for (size_t i = 0; i < coeffs.size(); ++i)
          exact(i) = static_cast<ExactCoeff>(coeffs[i]);
        return propagate_carries(exact);
      }

    return propagate_carries(NTTExact::pmultiply(pool, lhs, rhs, chunk_size));
  }
} // namespace Aleph

# endif // NTT_H
