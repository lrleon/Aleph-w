/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file ah-compat.H
    @brief Cross-compiler compatibility shims, force-included on MSVC.

    The Aleph-w core relies on a handful of GCC/Clang builtins that plain
    MSVC (cl) does not provide. clang-cl keeps the real builtins, so every
    shim here is gated on cl only. This header is force-included into every
    translation unit on Windows (via the `/FI` flag in CMakeLists.txt), so the
    shims are visible regardless of the per-header include graph. It contains
    preprocessor definitions only, keeping it valid for both C and C++ units.

    @ingroup Utilities
*/

#ifndef AH_COMPAT_H
#define AH_COMPAT_H

#if defined(_MSC_VER) && !defined(__clang__)

// Advisory hints with no observable effect: __builtin_prefetch is a cache
// prefetch and __builtin_expect is a branch-prediction hint. Mapping the
// former to a no-op and the latter to the bare condition is functionally
// transparent on cl. The variadic forms swallow the extra builtin arguments.
#  define __builtin_prefetch(...) ((void) 0)
#  define __builtin_expect(expr, expected) (expr)

// __builtin_ctzll: GCC/Clang count-trailing-zeros for unsigned long long.
// MSVC equivalent is _BitScanForward64 from <intrin.h>.
#  ifdef __cplusplus
#    include <intrin.h>
inline int __builtin_ctzll(unsigned long long x) noexcept
{
  unsigned long idx;
  _BitScanForward64(&idx, x);
  return static_cast<int>(idx);
}
#  endif

#endif // _MSC_VER && !__clang__

// Windows uses _popen/_pclose; shim for both cl and clang-cl.
#ifdef _WIN32
#  ifndef popen
#    define popen  _popen
#  endif
#  ifndef pclose
#    define pclose _pclose
#  endif
#endif

#endif // AH_COMPAT_H