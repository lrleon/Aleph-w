
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \|  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// Aleph implementation of C++ standard algorithms
// Based on code from GNU ISO C++ Library, Hewlett-Packard and Silicon Graphics

#ifndef AHALGO_H
#define AHALGO_H

/** @file ahAlgo.H
    @brief Aleph-w implementation of STL-like algorithms.

    This file provides implementations of common STL algorithms adapted
    for use with Aleph-w containers and iterators.

    @ingroup Algos
*/

#include <ahAssert.H>
#include <ahUtils.H>

namespace Aleph
{

using size_type = size_t;

// ============================================================================
// Non-Modifying Algorithms
// ============================================================================

/** @brief Apply an operation to each element in a range.

    Iterates through all elements in the range [beg, end) and applies
    the given operation to each one.

    @tparam Itor Iterator type.
    @tparam Operation Callable type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Operation to apply to each element.
    @return A copy of the operation op.

    @ingroup Algos
*/
template <class Itor, class Operation>
inline Operation for_each(Itor beg, const Itor & end, Operation op)
{
  while (beg != end)
    op(*beg++);
	
  return op;
}

/** @brief Count elements satisfying a predicate.

    Counts the number of elements in the range [beg, end) for which
    the predicate op returns true.

    @tparam Itor Iterator type.
    @tparam Operation Predicate type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Unary predicate to test elements.
    @return The number of elements satisfying the predicate.

    @ingroup Algos
*/
template<class Itor, class Operation>
[[nodiscard]] inline typename Itor::difference_type
count_if(Itor beg, const Itor & end, Operation op)
{
  typename Itor::difference_type n = 0;

  while (beg != end)
    if (op(*beg++))
      ++n;
	
  return n;
}

/** @brief Count elements equal to a value.

    Counts the number of elements in the range [beg, end) that are
    equal to the specified value.

    @tparam Itor Iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to compare against.
    @return The number of elements equal to value.

    @ingroup Algos
*/
template <class Itor, class T>
[[nodiscard]] inline typename Itor::difference_type
count(const Itor & beg, const Itor & end, const T & value)
{
  return Aleph::count_if(beg, end, Aleph::bind2nd(equal_to<T>(), value));
}

/** @brief Find the minimum element in a range.

    Returns an iterator to the smallest element in the range [beg, end)
    according to the comparison function.

    @tparam Itor Iterator type.
    @tparam CompFunc Comparison functor type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Comparison function (defaults to less-than).
    @return Iterator to the minimum element.

    @ingroup Algos
*/
template<class Itor, 
         class CompFunc = Aleph::less<typename Itor::value_type>>
[[nodiscard]] inline Itor min_element(Itor beg, const Itor & end, 
                                       CompFunc op = CompFunc())
{
  if (beg == end)
    return end;

  Itor min = beg;
  ++beg;

  while (beg != end)
    {
      if (op(*beg, *min))
        min = beg;

      ++beg;
    }

  return min;
}

/** @brief Find the maximum element in a range.

    Returns an iterator to the largest element in the range [beg, end)
    according to the comparison function.

    @tparam Itor Iterator type.
    @tparam CompFunc Comparison functor type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Comparison function (defaults to greater-than).
    @return Iterator to the maximum element.

    @ingroup Algos
*/
template<class Itor, 
         class CompFunc = Aleph::greater<typename Itor::value_type>>
[[nodiscard]] inline Itor max_element(const Itor& beg, const Itor& end, 
                                       CompFunc op = CompFunc())
{
  return Aleph::min_element(beg, end, op);
}

/** @brief Find the first element satisfying a predicate.

    Searches the range [beg, end) for the first element for which
    the predicate op returns true.

    @tparam Itor Iterator type.
    @tparam UnaryPredicate Predicate type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Unary predicate to test elements.
    @return Iterator to the found element, or end if not found.

    @ingroup Algos
*/
template<class Itor, class UnaryPredicate>
[[nodiscard]] inline Itor find_if(Itor beg, const Itor & end, UnaryPredicate op)
{
  while (beg != end and not op(*beg))
    ++beg;

  assert(beg == end or op(*beg));

  return beg;
}	

/** @brief Find the first element equal to a value.

    Searches the range [beg, end) for the first element equal to
    the specified value.

    @tparam Itor Iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to search for.
    @return Iterator to the found element, or end if not found.

    @ingroup Algos
*/
template<class Itor, class T>
[[nodiscard]] inline Itor find(const Itor & beg, const Itor & end, const T & value)
{
  return Aleph::find_if(beg, end, Aleph::bind2nd(Aleph::equal_to<T>(), value));
}

/** @brief Find n consecutive elements equal to a value.

    Searches for the first occurrence of count consecutive elements
    equal to value in the range [beg, end).

    @tparam Itor Iterator type.
    @tparam Size Integer type for count.
    @tparam T Value type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param count Number of consecutive elements to find.
    @param value Value to compare against.
    @param op Binary predicate for comparison (defaults to equality).
    @return Iterator to the first of the consecutive elements, or end.

    @ingroup Algos
*/
template<class Itor, class Size, class T, 
         class BinaryPredicate = Aleph::equal_to<T>>
[[nodiscard]] inline Itor search_n(Itor            beg, 
                                    const Itor &    end, 
                                    Size            count, 
                                    const T &       value, 
                                    BinaryPredicate op = BinaryPredicate())
{
  if (count <= 0 or beg == end)
    return end;

  Size i = 0;
  Itor first;

  while (beg != end and i < count)
    {
      if (op(*beg, value))
        {
          if (i == 0)
            first = beg;
          ++i;
        }
      else
        i = 0;
		
      ++beg;
    }

  if (i == count)
    return first;

  return end;
}

/** @brief Search for a subrange within a range.

    Searches for the first occurrence of the subrange [searchBeg, searchEnd)
    within the range [beg, end).

    @tparam Itor1 Main range iterator type.
    @tparam Itor2 Subrange iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element of the main range.
    @param end Iterator past the last element of the main range.
    @param searchBeg Iterator to the first element of the subrange.
    @param searchEnd Iterator past the last element of the subrange.
    @param op Binary predicate for comparison (defaults to equality).
    @return Iterator to the first element of the found subrange, or end.

    @note The ranges can be of different container types as long as op
          can compare their elements.

    @ingroup Algos
*/
template<class Itor1, class Itor2, 
         class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>>
[[nodiscard]] inline Itor1 search(Itor1 beg, const Itor1 & end, 
                                   Itor2 searchBeg, const Itor2 & searchEnd, 
                                   BinaryPredicate op = BinaryPredicate())
{
  if (beg == end or searchBeg == searchEnd)
    return end;

  Itor1 first;
  Itor2 pivot = searchBeg;
  int count = 0;

  while (beg != end and pivot != searchEnd)
    {	
      if (op(*beg, *pivot))
        {
          if (count == 0)
            first = beg;
		
          ++pivot;
          ++count;
        }
      else
        {
          pivot = searchBeg;
          count = 0;
        }

      ++beg;
    }
	
  if (pivot == searchEnd)
    return first;

  return end;
}

/** @brief Find the last occurrence of a subrange.

    Searches for the last occurrence of the subrange [searchBeg, searchEnd)
    within the range [beg, end).

    @tparam Itor1 Main range iterator type.
    @tparam Itor2 Subrange iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element of the main range.
    @param end Iterator past the last element of the main range.
    @param searchBeg Iterator to the first element of the subrange.
    @param searchEnd Iterator past the last element of the subrange.
    @param op Binary predicate for comparison.
    @return Iterator to the first element of the last occurrence, or end.

    @ingroup Algos
*/
template<class Itor1, class Itor2, 
         class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>>
[[nodiscard]] inline Itor1 find_end(Itor1 beg, Itor1 end, 
                                     Itor2 searchBeg, Itor2 searchEnd, 
                                     BinaryPredicate op = BinaryPredicate())
{
  if (beg == end or searchBeg == searchEnd)
    return end;

  Itor1 ret_val = end;
  while (true)
    {
      Itor1 new_ret = search(beg, end, searchBeg, searchEnd, op);
      if (new_ret == end)
        return ret_val;
      else
        {
          ret_val = new_ret;
          beg = new_ret;
          ++beg;
        }
    }
}

/** @brief Find first element from a set.

    Searches the range [beg, end) for the first element that matches
    any element in [searchBeg, searchEnd).

    @tparam Itor1 Main range iterator type.
    @tparam Itor2 Search set iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element of the main range.
    @param end Iterator past the last element of the main range.
    @param searchBeg Iterator to the first element of the search set.
    @param searchEnd Iterator past the last element of the search set.
    @param op Binary predicate for comparison.
    @return Iterator to the first matching element, or end.

    @ingroup Algos
*/
template<class Itor1, class Itor2, 
         class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>>
[[nodiscard]] inline Itor1 find_first_of(const Itor1& beg, const Itor1& end, 
                                          Itor2 searchBeg, const Itor2& searchEnd, 
                                          BinaryPredicate op = BinaryPredicate())
{
  while (searchBeg != searchEnd)
    {
      Itor1 current = beg;

      while (current != end)
        {
          if (op(*current, *searchBeg))
            return current;

          ++current;
        }

      ++searchBeg;
    }
  
  return end;
}

/** @brief Find first pair of adjacent equal elements.

    Searches the range [beg, end) for two consecutive elements that
    are equal according to the predicate.

    @tparam Itor Iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Binary predicate for comparison.
    @return Iterator to the first of the equal pair, or end.

    @ingroup Algos
*/
template<class Itor, 
         class BinaryPredicate = Aleph::equal_to<typename Itor::value_type>>
[[nodiscard]] inline Itor adjacent_find(Itor beg, const Itor & end, 
                                         BinaryPredicate op = BinaryPredicate())
{
  if (beg == end)
    return end;

  Itor next(beg);
  ++next;

  while (next != end)
    {
      if (op(*beg, *next))
        return beg;

      ++beg;
      ++next;
    }

  return end;
}

/** @brief Test if two ranges are equal.

    Compares elements in the range [beg, end) with elements starting
    at cmpBeg.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element of the first range.
    @param end Iterator past the last element of the first range.
    @param cmpBeg Iterator to the first element of the second range.
    @param op Binary predicate for comparison.
    @return true if all corresponding elements are equal.

    @ingroup Algos
*/
template <class Itor1, class Itor2, 
          class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>>
[[nodiscard]] inline bool equal(Itor1 beg, const Itor1 & end,
                                 Itor2 cmpBeg, BinaryPredicate op = BinaryPredicate())
{
  while (beg != end)
    {
      if (not op(*beg, *cmpBeg))
        return false;

      ++beg;
      ++cmpBeg;
    }

  return true;
}

/** @brief Find the first mismatching elements in two ranges.

    Compares elements in the range [beg, end) with elements starting
    at cmpBeg and returns the first pair that differs.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element of the first range.
    @param end Iterator past the last element of the first range.
    @param cmpBeg Iterator to the first element of the second range.
    @param op Binary predicate for comparison.
    @return Pair of iterators to the first mismatching elements.

    @ingroup Algos
*/
template <class Itor1, class Itor2, 
          class BinaryPredicate = Aleph::equal_to<typename Itor1::value_type>>
[[nodiscard]] inline std::pair<Itor1, Itor2>
mismatch(Itor1 beg, const Itor1& end, 
         Itor2 cmpBeg, 
         BinaryPredicate op = BinaryPredicate())
{
  while (beg != end and op(*beg, *cmpBeg))
    {
      ++beg;
      ++cmpBeg;
    }
  
  return std::make_pair(beg, cmpBeg);
}

/** @brief Lexicographical comparison of two ranges.

    Compares two ranges lexicographically.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @tparam Comp Comparison functor.
    @param beg1 Iterator to the first element of the first range.
    @param end1 Iterator past the last element of the first range.
    @param beg2 Iterator to the first element of the second range.
    @param end2 Iterator past the last element of the second range.
    @param op Comparison function.
    @return true if the first range is lexicographically less than the second.

    @ingroup Algos
*/
template <class Itor1, class Itor2, 
          class Comp = Aleph::less<typename Itor1::value_type>>
[[nodiscard]] inline bool lexicographical_compare(Itor1 beg1, const Itor1 & end1, 
                                                   Itor2 beg2, const Itor2 & end2, 
                                                   Comp op = Comp())
{
  while (beg1 != end1 and beg2 != end2) 
    {
      if (op(*beg1, *beg2))
        return true;
      else if (op(*beg2, *beg1))
        return false;
	
      ++beg1;
      ++beg2;
    }

  return beg1 == end1 and beg2 != end2;
}


// ============================================================================
// Modifying Algorithms
// ============================================================================

/** @brief Copy elements from one range to another.

    Copies elements from [sourceBeg, sourceEnd) to the range starting at destBeg.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 copy(Itor1 sourceBeg, const Itor1& sourceEnd, Itor2 destBeg)
{
  while (sourceBeg != sourceEnd)
    *destBeg++ = *sourceBeg++;

  return destBeg;
}

/** @brief Copy elements backward from one range to another.

    Copies elements from [sourceBeg, sourceEnd) to the range ending at destEnd,
    copying from back to front.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destEnd Iterator past the last destination position.
    @return Iterator to the first copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 copy_backward(const Itor1 & sourceBeg, Itor1 sourceEnd, Itor2 destEnd)
{
  while (sourceBeg != sourceEnd)
    *--destEnd = *--sourceEnd;

  return destEnd;
}

/** @brief Transform elements using a unary operation.

    Applies op to each element in [sourceBeg, sourceEnd) and stores
    the result in the range starting at destBeg.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @tparam UnaryFunc Transformation function type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @param op Unary operation to apply.
    @return Iterator past the last transformed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class UnaryFunc>
inline Itor2 transform(Itor1 sourceBeg, Itor1 sourceEnd, 
                       Itor2 destBeg, UnaryFunc op)
{
  while (sourceBeg != sourceEnd)
    *destBeg++ = op(*sourceBeg++);
  
  return destBeg;
}

/** @brief Transform elements using a binary operation.

    Applies op to corresponding elements from two ranges and stores
    the result starting at destBeg.

    @tparam Itor1 First source iterator type.
    @tparam Itor2 Second source iterator type.
    @tparam Itor3 Destination iterator type.
    @tparam BinaryFunc Transformation function type.
    @param source1Beg Iterator to the first element of the first source.
    @param source1End Iterator past the last element of the first source.
    @param source2Beg Iterator to the first element of the second source.
    @param destBeg Iterator to the first destination position.
    @param op Binary operation to apply.
    @return Iterator past the last transformed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class Itor3, class BinaryFunc>
inline Itor3 transform(Itor1 source1Beg, Itor1 source1End, 
                       Itor2 source2Beg, 
                       Itor3 destBeg, 
                       BinaryFunc op)
{
  while (source1Beg != source1End)
    *destBeg++ = op(*source1Beg++, *source2Beg++);

  return destBeg;
}

/** @brief Swap elements between two ranges.

    Swaps elements in the range [beg1, end1) with corresponding
    elements starting at beg2.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @param beg1 Iterator to the first element of the first range.
    @param end1 Iterator past the last element of the first range.
    @param beg2 Iterator to the first element of the second range.
    @return Iterator past the last swapped element in the second range.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 swap_ranges(Itor1 beg1, const Itor1& end1, Itor2 beg2)
{
  while (beg1 != end1)
    std::swap(*beg1++, *beg2++);

  return beg2;
}

/** @brief Fill a range with a value.

    Assigns the specified value to all elements in [beg, end).

    @tparam Itor Iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to assign.

    @ingroup Algos
*/
template <class Itor, class T>
inline void fill(Itor beg, const Itor& end, const T & value)
{
  while (beg != end)
    *beg++ = value;
}

/** @brief Fill n elements with a value.

    Assigns the specified value to the first n elements starting at beg.

    @tparam Itor Iterator type.
    @tparam T Value type.
    @tparam Size Integer type.
    @param beg Iterator to the first element.
    @param num Number of elements to fill.
    @param value Value to assign.

    @ingroup Algos
*/
template <class Itor, class T, class Size>
inline void fill_n(Itor beg, Size num, const T & value)
{
  while (num-- > 0)
    *beg++ = value;
}	

/** @brief Generate values for a range.

    Assigns the result of successive calls to op to each element in [beg, end).

    @tparam Itor Iterator type.
    @tparam Func Generator function type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Generator function.

    @ingroup Algos
*/
template <class Itor, class Func>
inline void generate(Itor beg, const Itor& end, Func op)
{
  while (beg != end)
    *beg++ = op();
}

/** @brief Generate values for n elements.

    Assigns the result of successive calls to op to the first n elements.

    @tparam Itor Iterator type.
    @tparam Size Integer type.
    @tparam Func Generator function type.
    @param beg Iterator to the first element.
    @param num Number of elements to generate.
    @param op Generator function.

    @ingroup Algos
*/
template <class Itor, class Size, class Func>
inline void generate_n(Itor beg, Size num, Func op)
{
  while (num-- > 0)
    *beg++ = op();
}

/** @brief Replace elements satisfying a predicate.

    Replaces all elements in [beg, end) for which op returns true
    with the specified value.

    @tparam Itor Iterator type.
    @tparam UnaryPredicate Predicate type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param op Unary predicate to test elements.
    @param value Replacement value.

    @ingroup Algos
*/
template <class Itor, class UnaryPredicate, class T>
inline void replace_if(Itor beg, const Itor& end, 
                       UnaryPredicate op, 
                       const T & value)
{
  while (beg != end)
    {
      if (op(*beg))
        *beg = value;

      ++beg;
    }
}

/** @brief Replace elements equal to a value.

    Replaces all elements in [beg, end) equal to old_value with new_value.

    @tparam Itor Iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param old_value Value to be replaced.
    @param new_value Replacement value.

    @ingroup Algos
*/
template <class Itor, class T>
inline void replace(Itor beg, const Itor& end, 
                    const T & old_value, 
                    const T & new_value)
{
  Aleph::replace_if(beg, end, 
                    Aleph::bind2nd(equal_to<T>(), old_value), new_value);
}

/** @brief Copy and replace elements satisfying a predicate.

    Copies elements from source to destination, replacing those
    for which op returns true with the specified value.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @tparam UnaryPredicate Predicate type.
    @tparam T Value type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @param op Unary predicate to test elements.
    @param value Replacement value.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class UnaryPredicate, class T>
inline Itor2 replace_copy_if(Itor1 sourceBeg, const Itor1& sourceEnd, 
                             Itor2 destBeg, 
                             UnaryPredicate op, 
                             const T & value)
{
  while (sourceBeg != sourceEnd)
    {
      if (op(*sourceBeg))
        *destBeg++ = value;
      else
        *destBeg++ = *sourceBeg;

      ++sourceBeg;
    }

  return destBeg;
}

/** @brief Copy and replace elements equal to a value.

    Copies elements from source to destination, replacing those
    equal to old_value with new_value.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @tparam T Value type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @param old_value Value to be replaced.
    @param new_value Replacement value.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class T>
inline Itor2 replace_copy(const Itor1& sourceBeg, const Itor1& sourceEnd, 
                          Itor2 destBeg, 
                          const T & old_value, 
                          const T & new_value)
{
  return Aleph::replace_copy_if(sourceBeg, sourceEnd, destBeg, 
                                 Aleph::bind2nd(equal_to<T>(), old_value), 
                                 new_value);
}

/** @brief Copy elements not satisfying a predicate.

    Copies elements from [first, last) to result, excluding those
    for which pred returns true.

    @tparam In_Itor Input iterator type.
    @tparam Out_Itor Output iterator type.
    @tparam Predicate Predicate type.
    @param __first Iterator to the first source element.
    @param __last Iterator past the last source element.
    @param __result Iterator to the first destination position.
    @param __pred Unary predicate to test elements.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class In_Itor, class Out_Itor, class Predicate>
inline Out_Itor remove_copy_if(In_Itor __first, const In_Itor & __last,
                               Out_Itor __result, 
                               Predicate __pred)
{
  for ( ; __first != __last; ++__first)
    if (not __pred(*__first))
      {
        *__result = *__first;
        ++__result;
      }

  return __result;
}

/** @brief Remove elements satisfying a predicate.

    Removes all elements in [first, last) for which pred returns true
    by moving the remaining elements forward.

    @tparam Fw_Itor Forward iterator type.
    @tparam Predicate Predicate type.
    @param __first Iterator to the first element.
    @param __last Iterator past the last element.
    @param __pred Unary predicate to test elements.
    @return Iterator to the new logical end of the range.

    @ingroup Algos
*/
template<class Fw_Itor, class Predicate>
inline Fw_Itor remove_if(Fw_Itor __first, const Fw_Itor & __last,
                         Predicate __pred)
{
  __first = Aleph::find_if(__first, __last, __pred);

  Fw_Itor __i = __first;

  if (__first == __last) 
    return __first;
  else
    return Aleph::remove_copy_if(++__i, __last, __first, __pred);
}

/** @brief Remove elements equal to a value.

    Removes all elements in [first, last) equal to value by moving
    the remaining elements forward.

    @tparam Fw_Itor Forward iterator type.
    @tparam T Value type.
    @param __first Iterator to the first element.
    @param __last Iterator past the last element.
    @param __value Value to remove.
    @return Iterator to the new logical end of the range.

    @ingroup Algos
*/
template<class Fw_Itor, class T>
inline Fw_Itor remove(Fw_Itor __first, const Fw_Itor & __last, 
                      const T & __value)
{
  return Aleph::remove_if(__first, __last, 
                          Aleph::bind2nd(Aleph::equal_to<T>(), __value));
}

/// @cond INTERNAL
template<class In_Itor, class Out_Itor, 
         class BinaryPredicate = Aleph::equal_to<typename In_Itor::value_type>>
static inline Out_Itor __unique_copy(In_Itor __first, In_Itor __last,
                                     Out_Itor __result,
                                     BinaryPredicate __binary_pred = BinaryPredicate())
{
  typename In_Itor::value_type __value = *__first;

  *__result = __value;
  
  while (++__first != __last)
    if (not __binary_pred(__value, *__first))
      {
        __value = *__first;
        *++__result = __value;
      }

  return ++__result;
}
/// @endcond

/** @brief Copy unique elements.

    Copies elements from [first, last) to result, eliminating
    consecutive duplicates.

    @tparam In_Itor Input iterator type.
    @tparam Out_Itor Output iterator type.
    @param __first Iterator to the first source element.
    @param __last Iterator past the last source element.
    @param __result Iterator to the first destination position.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template<class In_Itor, class Out_Itor>
inline Out_Itor unique_copy(In_Itor __first, In_Itor __last,
                            Out_Itor __result)
{
  if (__first == __last) 
    return __result;
   
  return Aleph::__unique_copy(__first, __last, __result);
}

/** @brief Copy unique elements with custom comparison.

    Copies elements from [first, last) to result, eliminating
    consecutive elements that compare equal according to the predicate.

    @tparam In_Itor Input iterator type.
    @tparam Out_Itor Output iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param __first Iterator to the first source element.
    @param __last Iterator past the last source element.
    @param __result Iterator to the first destination position.
    @param __binary_pred Binary predicate for comparison.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class In_Itor, class Out_Itor, class BinaryPredicate>
inline Out_Itor unique_copy(In_Itor __first, In_Itor __last,
                            Out_Itor __result,
                            BinaryPredicate __binary_pred)
{
  if (__first == __last) 
    return __result;

  return Aleph::__unique_copy(__first, __last, __result, __binary_pred);
}

/** @brief Remove consecutive duplicates in place.

    Removes consecutive elements that compare equal according to
    the predicate by moving unique elements forward.

    @tparam Itor Iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param __first Iterator to the first element.
    @param __last Iterator past the last element.
    @param __binary_pred Binary predicate for comparison.
    @return Iterator to the new logical end of the range.

    @ingroup Algos
*/
template<class Itor,
         class BinaryPredicate = Aleph::equal_to<typename Itor::value_type>>
inline Itor unique(Itor __first, Itor __last, 
                   BinaryPredicate __binary_pred = BinaryPredicate())
{
  // Skip the beginning, if already unique.
  __first = Aleph::adjacent_find(__first, __last, __binary_pred);

  if (__first == __last)
    return __last;

  // Do the real copy work.
  Itor __dest = __first;
  ++__first;

  while (++__first != __last)
    if (not __binary_pred(*__dest, *__first))
      *++__dest = *__first;

  return ++__dest;
}


// ============================================================================
// Mutating Algorithms
// ============================================================================

/** @brief Reverse elements in a range.

    Reverses the order of elements in the range [beg, end).

    @tparam Itor Bidirectional iterator type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.

    @ingroup Algos
*/
template <class Itor>
inline void reverse(Itor beg, Itor end)
{
  while (beg != end and beg != --end)
    std::swap(*beg++, *end);
}

/** @brief Copy elements in reverse order.

    Copies elements from [sourceBeg, sourceEnd) to destBeg in reverse order.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 reverse_copy(Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  while (sourceBeg != sourceEnd)
    {
      --sourceEnd;
      *destBeg = *sourceEnd;
      ++destBeg;
    }

  return destBeg;
}

/** @brief Rotate elements in a range.

    Rotates the elements in [beg, end) such that pos becomes the first element.

    @tparam Itor Forward iterator type.
    @param beg Iterator to the first element.
    @param pos Iterator to the new first element.
    @param end Iterator past the last element.

    @ingroup Algos
*/
template <class Itor>
inline void rotate(Itor beg, Itor pos, Itor end)
{
  Aleph::reverse(beg, pos);
  Aleph::reverse(pos, end);
  Aleph::reverse(beg, end);
}

/** @brief Copy and rotate elements.

    Copies elements from [beg, end) to tgt_beg, rotating so that
    pos becomes the first element.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param beg Iterator to the first source element.
    @param pos Iterator to the new first element.
    @param end Iterator past the last source element.
    @param tgt_beg Iterator to the first destination position.
    @return Iterator past the last copied element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 rotate_copy(const Itor1& beg, const Itor1& pos, const Itor1& end, 
                         Itor2 tgt_beg)
{
  return Aleph::copy(beg, pos, Aleph::copy(pos, end, tgt_beg));
}


// ============================================================================
// Sorted Range Algorithms
// ============================================================================

/** @brief Find lower bound in a sorted range.

    Returns an iterator to the first element not less than value
    in the sorted range [beg, end).

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to compare against.
    @return Iterator to the lower bound.

    @note This is a linear implementation suitable for forward iterators.
          For random access iterators, consider using binary search.

    @ingroup Algos
*/
template <class Itor, class T>
[[nodiscard]] inline Itor lower_bound(Itor beg, Itor end, const T& value)
{
  while (beg != end and *beg < value)
    ++beg;

  return beg;
}

/** @brief Find lower bound with custom comparison.

    Returns an iterator to the first element for which op(*beg, value)
    is false in the sorted range [beg, end).

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to compare against.
    @param op Binary comparison predicate.
    @return Iterator to the lower bound.

    @ingroup Algos
*/
template <class Itor, class T, class BinaryPredicate>
[[nodiscard]] inline Itor lower_bound(Itor beg, Itor end, const T& value, 
                                       BinaryPredicate op)
{
  while (beg != end and op(*beg, value))
    ++beg;

  return beg;
}

/** @brief Find upper bound in a sorted range.

    Returns an iterator to the first element greater than value
    in the sorted range [beg, end).

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to compare against.
    @return Iterator to the upper bound.

    @ingroup Algos
*/
template <class Itor, class T>
[[nodiscard]] inline Itor upper_bound(Itor beg, Itor end, const T& value)
{
  while (beg != end and not (value < *beg))
    ++beg;

  return beg;
}

/** @brief Find upper bound with custom comparison.

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to compare against.
    @param op Binary comparison predicate.
    @return Iterator to the upper bound.

    @ingroup Algos
*/
template <class Itor, class T, class BinaryPredicate>
[[nodiscard]] inline Itor upper_bound(Itor beg, Itor end, const T& value, 
                                       BinaryPredicate op)
{
  while (beg != end and (op(*beg, value) or not op(value, *beg)))
    ++beg;

  return beg;
}

/** @brief Binary search for a value.

    Tests whether value exists in the sorted range [beg, end).

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to search for.
    @return true if value is found.

    @ingroup Algos
*/
template<class Itor, class T>
[[nodiscard]] inline bool binary_search(Itor beg, Itor end, const T& value)
{
  Itor i = Aleph::lower_bound(beg, end, value);
  return i != end and not (value < *i);
}

/** @brief Binary search with custom comparison.

    Tests whether value exists in the sorted range [beg, end)
    according to the comparison predicate.

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to search for.
    @param op Binary comparison predicate.
    @return true if value is found.

    @ingroup Algos
*/
template<class Itor, class T, class BinaryPredicate>
[[nodiscard]] inline bool binary_search(Itor beg, Itor end, const T& value, 
                                         BinaryPredicate op)
{
  Itor i = Aleph::lower_bound(beg, end, value, op);
  return i != end and not op(value, *i);
}

/** @brief Find equal range in a sorted sequence.

    Returns a pair of iterators defining the subrange of elements
    equal to value.

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to search for.
    @return Pair of iterators [lower_bound, upper_bound).

    @ingroup Algos
*/
template <class Itor, class T>
[[nodiscard]] inline std::pair<Itor, Itor> 
equal_range(Itor beg, Itor end, const T& value)
{
  return std::make_pair(Aleph::lower_bound(beg, end, value),
                        Aleph::upper_bound(beg, end, value));
}

/** @brief Find equal range with custom comparison.

    @tparam Itor Forward iterator type.
    @tparam T Value type.
    @tparam BinaryPredicate Comparison predicate.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param value Value to search for.
    @param op Binary comparison predicate.
    @return Pair of iterators [lower_bound, upper_bound).

    @ingroup Algos
*/
template <class Itor, class T, class BinaryPredicate>
[[nodiscard]] inline std::pair<Itor, Itor> 
equal_range(Itor beg, Itor end, const T& value, BinaryPredicate op)
{
  return std::make_pair(Aleph::lower_bound(beg, end, value, op),
                        Aleph::upper_bound(beg, end, value, op));
}

/** @brief Test if one sorted range includes another.

    Tests whether all elements in [searchBeg, searchEnd) are contained
    in the sorted range [beg, end).

    @tparam Itor1 Main range iterator type.
    @tparam Itor2 Search range iterator type.
    @param beg Iterator to the first element of the main range.
    @param end Iterator past the last element of the main range.
    @param searchBeg Iterator to the first element of the search range.
    @param searchEnd Iterator past the last element of the search range.
    @return true if all search elements are found.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
[[nodiscard]] inline bool includes(Itor1 beg, Itor1 end, 
                                    Itor2 searchBeg, Itor2 searchEnd)
{
  while (beg != end and searchBeg != searchEnd)
    {
      if (*searchBeg < *beg)
        return false;
      else if (*beg < *searchBeg)
        ++beg;
      else
        {
          ++beg;
          ++searchBeg;
        }
    }
  
  return searchBeg == searchEnd;
}

/** @brief Merge two sorted ranges.

    Merges elements from [source1Beg, source1End) and [source2Beg, source2End)
    into a single sorted sequence starting at destBeg.

    @tparam Itor1 First source iterator type.
    @tparam Itor2 Second source iterator type.
    @tparam Itor3 Destination iterator type.
    @param source1Beg Iterator to the first element of the first source.
    @param source1End Iterator past the last element of the first source.
    @param source2Beg Iterator to the first element of the second source.
    @param source2End Iterator past the last element of the second source.
    @param destBeg Iterator to the first destination position.
    @return Iterator past the last merged element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class Itor3>
inline Itor3 merge(Itor1 source1Beg, Itor1 source1End, 
                   Itor2 source2Beg, Itor2 source2End, 
                   Itor3 destBeg) 
{
  while (source1Beg != source1End and source2Beg != source2End) 
    {
      if (*source2Beg < *source1Beg) 
        {
          *destBeg = *source2Beg;
          ++source2Beg;
        }
      else 
        {
          *destBeg = *source1Beg;
          ++source1Beg;
        }
      ++destBeg;
    }
  return Aleph::copy(source2Beg, source2End, 
                     Aleph::copy(source1Beg, source1End, destBeg));
}

/** @brief Merge two sorted ranges with custom comparison.

    Merges elements from two sorted ranges into a single sorted sequence
    using the given comparison function.

    @tparam Itor1 First source iterator type.
    @tparam Itor2 Second source iterator type.
    @tparam Itor3 Destination iterator type.
    @tparam BinaryPredicate Comparison predicate.
    @param source1Beg Iterator to the first element of the first source.
    @param source1End Iterator past the last element of the first source.
    @param source2Beg Iterator to the first element of the second source.
    @param source2End Iterator past the last element of the second source.
    @param destBeg Iterator to the first destination position.
    @param op Binary comparison predicate.
    @return Iterator past the last merged element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class Itor3, class BinaryPredicate>
inline Itor3 merge(Itor1 source1Beg, Itor1 source1End, 
                   Itor2 source2Beg, Itor2 source2End, 
                   Itor3 destBeg, BinaryPredicate op) 
{
  while (source1Beg != source1End and source2Beg != source2End) 
    {
      if (op(*source2Beg, *source1Beg)) 
        {
          *destBeg = *source2Beg;
          ++source2Beg;
        }
      else 
        {
          *destBeg = *source1Beg;
          ++source1Beg;
        }
      ++destBeg;
    }
  return Aleph::copy(source2Beg, source2End, 
                     Aleph::copy(source1Beg, source1End, destBeg));
}


// ============================================================================
// Numeric Algorithms
// ============================================================================

/** @brief Accumulate values in a range.

    Computes the sum of initValue and all elements in [beg, end).

    @tparam Itor Iterator type.
    @tparam T Accumulator type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param initValue Initial value for the accumulation.
    @return The accumulated value.

    @ingroup Algos
*/
template <class Itor, class T>
[[nodiscard]] inline T accumulate(Itor beg, Itor end, T initValue)
{
  for ( ; beg != end; ++beg)
    initValue = initValue + *beg;

  return initValue;
}

/** @brief Accumulate values with custom operation.

    Computes the generalized sum of initValue and all elements in [beg, end)
    using the binary operation op.

    @tparam Itor Iterator type.
    @tparam T Accumulator type.
    @tparam BinaryFunc Binary operation type.
    @param beg Iterator to the first element.
    @param end Iterator past the last element.
    @param initValue Initial value for the accumulation.
    @param op Binary operation for combining values.
    @return The accumulated value.

    @ingroup Algos
*/
template <class Itor, class T, class BinaryFunc>
[[nodiscard]] inline T accumulate(Itor beg, Itor end, T initValue, BinaryFunc op)
{
  for ( ; beg != end; ++beg)
    initValue = op(initValue, *beg);

  return initValue;
}

/** @brief Compute inner product of two ranges.

    Computes the inner product of elements from [beg1, end1) with
    elements starting at beg2, initialized with initValue.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @tparam T Accumulator type.
    @param beg1 Iterator to the first element of the first range.
    @param end1 Iterator past the last element of the first range.
    @param beg2 Iterator to the first element of the second range.
    @param initValue Initial value for the accumulation.
    @return The inner product.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class T>
[[nodiscard]] inline T inner_product(Itor1 beg1, Itor1 end1, Itor2 beg2, T initValue)
{
  for ( ; beg1 != end1; ++beg1, ++beg2)
    initValue = initValue + (*beg1 * *beg2);

  return initValue;
}

/** @brief Compute inner product with custom operations.

    Computes a generalized inner product using op1 for accumulation
    and op2 for combining element pairs.

    @tparam Itor1 First range iterator type.
    @tparam Itor2 Second range iterator type.
    @tparam T Accumulator type.
    @tparam BinaryFunc1 Accumulation operation type.
    @tparam BinaryFunc2 Combination operation type.
    @param beg1 Iterator to the first element of the first range.
    @param end1 Iterator past the last element of the first range.
    @param beg2 Iterator to the first element of the second range.
    @param initValue Initial value for the accumulation.
    @param op1 Binary operation for accumulation.
    @param op2 Binary operation for combining element pairs.
    @return The generalized inner product.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class T, class BinaryFunc1, class BinaryFunc2>
[[nodiscard]] inline T inner_product(Itor1 beg1, Itor1 end1, Itor2 beg2, 
                                      T initValue, BinaryFunc1 op1, BinaryFunc2 op2)
{
  for ( ; beg1 != end1; ++beg1, ++beg2)
    initValue = op1(initValue, op2(*beg1, *beg2));

  return initValue;
}

/** @brief Compute partial sums of a range.

    Computes cumulative sums of elements in [sourceBeg, sourceEnd)
    and writes them to the range starting at destBeg.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @return Iterator past the last computed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 partial_sum(Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  if (sourceBeg == sourceEnd)
    return destBeg;

  typename Itor1::value_type sum = *sourceBeg;
  *destBeg = sum;
  ++sourceBeg;
  ++destBeg;

  for ( ; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      sum = sum + *sourceBeg;
      *destBeg = sum;
    }

  return destBeg;
}

/** @brief Compute partial sums with custom operation.

    Computes cumulative sums using the binary operation op.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @tparam BinaryFunc Binary operation type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @param op Binary operation for combining values.
    @return Iterator past the last computed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class BinaryFunc>
inline Itor2 partial_sum(Itor1 sourceBeg, Itor1 sourceEnd, 
                         Itor2 destBeg, BinaryFunc op)
{
  if (sourceBeg == sourceEnd)
    return destBeg;

  typename Itor1::value_type sum = *sourceBeg;
  *destBeg = sum;
  ++sourceBeg;
  ++destBeg;

  for ( ; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      sum = op(sum, *sourceBeg);
      *destBeg = sum;
    }

  return destBeg;
}

/** @brief Compute adjacent differences.

    Computes differences between consecutive elements in [sourceBeg, sourceEnd)
    and writes them to the range starting at destBeg. The first element is
    copied unchanged.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @return Iterator past the last computed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2>
inline Itor2 adjacent_difference(Itor1 sourceBeg, Itor1 sourceEnd, Itor2 destBeg)
{
  if (sourceBeg == sourceEnd)
    return destBeg;

  typename Itor1::value_type prev = *sourceBeg;
  *destBeg = prev;
  ++sourceBeg;
  ++destBeg;

  for ( ; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      typename Itor1::value_type curr = *sourceBeg;
      *destBeg = curr - prev;
      prev = curr;
    }

  return destBeg;
}

/** @brief Compute adjacent differences with custom operation.

    Computes differences using the binary operation op.

    @tparam Itor1 Source iterator type.
    @tparam Itor2 Destination iterator type.
    @tparam BinaryFunc Binary operation type.
    @param sourceBeg Iterator to the first source element.
    @param sourceEnd Iterator past the last source element.
    @param destBeg Iterator to the first destination position.
    @param op Binary operation for computing differences.
    @return Iterator past the last computed element in destination.

    @ingroup Algos
*/
template <class Itor1, class Itor2, class BinaryFunc>
inline Itor2 adjacent_difference(Itor1 sourceBeg, Itor1 sourceEnd, 
                                 Itor2 destBeg, BinaryFunc op)
{
  if (sourceBeg == sourceEnd)
    return destBeg;

  typename Itor1::value_type prev = *sourceBeg;
  *destBeg = prev;
  ++sourceBeg;
  ++destBeg;

  for ( ; sourceBeg != sourceEnd; ++sourceBeg, ++destBeg)
    {
      typename Itor1::value_type curr = *sourceBeg;
      *destBeg = op(curr, prev);
      prev = curr;
    }

  return destBeg;
}

} // end namespace Aleph

#endif // AHALGO_H
