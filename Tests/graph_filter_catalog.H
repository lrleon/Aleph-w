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

/**
 * @file graph_filter_catalog.H
 * @brief The kinds of arc filter and of distance that the graph
 *        algorithms must accept.
 *
 * ArcFilter only asks a filter to be callable with an arc. These filters
 * cover the ways in which a valid one can differ from Dft_Show_Arc, each
 * of which broke some algorithm at some time: state shared through a
 * pointer; no assignment (a lambda with captures, a reference or a const
 * member); a copy or a move that is not noexcept (a hand-written copy, a
 * std::deque member); no default constructor; a call that throws.
 *
 * All of them hide the arcs whose numbers, the infos of graph_harness.H,
 * are set in a mask, so that a test gives the same mask to the oracle.
 *
 * The distances read the weight of an arc by its number from a vector,
 * with state, without a default constructor, or throwing at a given call.
 */

#ifndef ALEPH_TESTS_GRAPH_FILTER_CATALOG_H
#define ALEPH_TESTS_GRAPH_FILTER_CATALOG_H

#include <cstddef>
#include <deque>
#include <limits>
#include <stdexcept>
#include <vector>

namespace graph_filters
{
  /// Whether the arc number i is shown by the mask hidden
  constexpr bool shown(unsigned long hidden, int i) noexcept
  {
    return i < 0 or i >= int(sizeof(unsigned long) * 8) or ((hidden >> i) & 1UL) == 0;
  }

  /// Hides the arcs of a mask; copyable, assignable, noexcept
  template <class GT>
  struct Hide_Mask
  {
    unsigned long hidden = 0;

    bool operator () (typename GT::Arc * a) const noexcept
    {
      return shown(hidden, a->get_info());
    }
  };

  /// As Hide_Mask, and counts its calls through a pointer
  template <class GT>
  struct Counting
  {
    unsigned long hidden = 0;
    size_t * calls = nullptr;

    bool operator () (typename GT::Arc * a) const noexcept
    {
      if (calls != nullptr)
        ++*calls;
      return shown(hidden, a->get_info());
    }
  };

  /// A lambda with captures: copyable, neither assignable nor default
  /// constructible
  template <class GT>
  auto make_capturing_lambda(unsigned long hidden)
  {
    return [hidden] (typename GT::Arc * a) { return shown(hidden, a->get_info()); };
  }

  template <class GT>
  using Capturing_Lambda = decltype(make_capturing_lambda<GT>(0));

  /// Keeps its mask by reference: neither assignable nor default
  /// constructible
  template <class GT>
  struct Reference_Member
  {
    const unsigned long & hidden;

    explicit Reference_Member(const unsigned long & mask) noexcept : hidden(mask) {}

    bool operator () (typename GT::Arc * a) const noexcept
    {
      return shown(hidden, a->get_info());
    }
  };

  /// A const member: default constructible, not assignable
  template <class GT>
  struct Const_Member
  {
    const unsigned long hidden = 0;

    Const_Member() noexcept = default;
    explicit Const_Member(unsigned long mask) noexcept : hidden(mask) {}

    bool operator () (typename GT::Arc * a) const noexcept
    {
      return shown(hidden, a->get_info());
    }
  };

  /// A hand-written copy that is not noexcept, which leaves the type
  /// without a move: std::move_if_noexcept copies it
  template <class GT>
  struct Throwing_Copy
  {
    unsigned long hidden = 0;

    Throwing_Copy() = default;
    explicit Throwing_Copy(unsigned long mask) : hidden(mask) {}
    Throwing_Copy(const Throwing_Copy & other) : hidden(other.hidden) {}
    Throwing_Copy & operator = (const Throwing_Copy & other)
    {
      hidden = other.hidden;
      return *this;
    }

    bool operator () (typename GT::Arc * a) const
    {
      return shown(hidden, a->get_info());
    }
  };

  /// A std::deque member, whose move is not noexcept in libstdc++
  template <class GT>
  struct Deque_Member
  {
    std::deque<int> state;
    unsigned long hidden = 0;

    Deque_Member() = default;
    explicit Deque_Member(unsigned long mask) : hidden(mask) {}

    bool operator () (typename GT::Arc * a) const
    {
      return shown(hidden, a->get_info());
    }
  };

  /// Throws std::runtime_error at the call whose number, counted from 1
  /// through a shared counter, is throw_at
  template <class GT>
  struct Throw_At
  {
    unsigned long hidden = 0;
    size_t * calls = nullptr;
    size_t throw_at = 0;

    bool operator () (typename GT::Arc * a) const
    {
      if (calls != nullptr and ++*calls == throw_at)
        throw std::runtime_error("graph_filters::Throw_At");
      return shown(hidden, a->get_info());
    }
  };

  /// The weight of an arc, read by its number from a vector
  template <class GT, typename T = long long>
  struct Weight_Of
  {
    using Distance_Type = T;

    static const Distance_Type Zero_Distance;
    static const Distance_Type Max_Distance;

    const std::vector<T> * weights = nullptr;

    Distance_Type operator () (typename GT::Arc * a) const
    {
      return (*weights)[size_t(a->get_info())];
    }

    Distance_Type operator () (typename GT::Arc * a, typename GT::Node *) const
    {
      return (*this)(a);
    }
  };

  template <class GT, typename T>
  const T Weight_Of<GT, T>::Zero_Distance = T();

  template <class GT, typename T>
  const T Weight_Of<GT, T>::Max_Distance = std::numeric_limits<T>::max();

  /// As Weight_Of, without a default constructor
  template <class GT, typename T = long long>
  struct Weight_Without_Default : Weight_Of<GT, T>
  {
    explicit Weight_Without_Default(const std::vector<T> & w) noexcept
    {
      this->weights = &w;
    }
  };

  /// As Weight_Of, and throws std::runtime_error at the call whose
  /// number, counted from 1 through a shared counter, is throw_at
  template <class GT, typename T = long long>
  struct Throwing_Weight : Weight_Of<GT, T>
  {
    size_t * calls = nullptr;
    size_t throw_at = 0;

    T operator () (typename GT::Arc * a) const
    {
      if (calls != nullptr and ++*calls == throw_at)
        throw std::runtime_error("graph_filters::Throwing_Weight");
      return Weight_Of<GT, T>::operator()(a);
    }

    T operator () (typename GT::Arc * a, typename GT::Node *) const
    {
      return (*this)(a);
    }
  };
} // namespace graph_filters

#endif // ALEPH_TESTS_GRAPH_FILTER_CATALOG_H
