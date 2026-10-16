/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file cxx_standard.cc
 * @brief Installed-package check: the C++ standard that Aleph::Aleph gives.
 *
 * Compiled once with no standard chosen by the consumer, which must get the
 * C++20 that Aleph::Aleph requires, and once with C++23 chosen by the
 * consumer (`EXPECT_NEWER_THAN_CXX20`), which must keep it. The header needs
 * C++20: before the requirement was exported, a consumer that chose no
 * standard could not compile it with the default standard of GCC.
 */

#include <cstdio>

#include <tpl_array.H>

// MSVC keeps __cplusplus at 199711L without /Zc:__cplusplus; _MSVC_LANG
// holds the standard in use.
#if defined(_MSVC_LANG)
# define CXX_STANDARD_IN_USE _MSVC_LANG
#else
# define CXX_STANDARD_IN_USE __cplusplus
#endif

static_assert(CXX_STANDARD_IN_USE >= 202002L,
              "Aleph::Aleph did not give its consumer C++20");
#if EXPECT_NEWER_THAN_CXX20
static_assert(CXX_STANDARD_IN_USE > 202002L,
              "the C++23 chosen by the consumer was not kept");
#endif

int main()
{
  Aleph::Array<int> a;
  a.append(1);
  a.append(2);
  std::printf("C++ standard in use: %ld\n", static_cast<long>(CXX_STANDARD_IN_USE));
  return a.size() == 2 ? 0 : 1;
}
