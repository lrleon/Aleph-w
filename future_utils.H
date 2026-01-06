/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
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

#ifndef FUTURE_UTILS_H
#define FUTURE_UTILS_H

#include <future>
#include <tpl_dynList.H>

/** @file future_utils.H
 *  @brief Utility functions for working with std::future collections.
 *
 *  This header provides convenient functions for collecting results from
 *  a list of std::future objects. It simplifies the common pattern of
 *  waiting for multiple asynchronous operations to complete and gathering
 *  their results.
 *
 *  ## Example Usage
 *
 *  @code
 *  #include <future_utils.H>
 *  #include <async>
 *
 *  // Launch multiple async tasks
 *  DynList<std::future<int>> futures;
 *  for (int i = 0; i < 10; ++i)
 *    futures.append(std::async(std::launch::async, [i] { return i * i; }));
 *
 *  // Collect all results (blocks until all complete)
 *  DynList<int> results = get_futures(futures);
 *  // results now contains: 0, 1, 4, 9, 16, 25, 36, 49, 64, 81
 *  @endcode
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

namespace Aleph
{

/** @brief Collect results from a list of futures.
 *
 *  Waits for each future in the list to complete and collects all results
 *  into a new DynList. The futures are consumed (removed) from the input
 *  list in FIFO order.
 *
 *  @tparam T The result type of the futures.
 *
 *  @param[in,out] future_list List of futures to collect. Will be empty
 *                             after the call.
 *
 *  @return DynList containing all the results in the order the futures
 *          were stored in the list.
 *
 *  @throw Any exception thrown by any of the futures will propagate.
 *
 *  @note This function blocks until all futures have completed.
 *  @note The order of results matches the order of futures in the list,
 *        not the order in which the async operations completed.
 *
 *  @see std::future
 */
template <typename T>
[[nodiscard]] inline DynList<T>
get_futures(DynList<std::future<T>> & future_list)
{
  DynList<T> ret;
  while (!future_list.is_empty())
    ret.append(future_list.remove_first().get());

  return ret;
}

/** @brief Collect results from a list of futures (rvalue overload).
 *
 *  Overload for rvalue references, allowing temporary lists to be passed.
 *
 *  @tparam T The result type of the futures.
 *
 *  @param[in] future_list Temporary list of futures to collect.
 *
 *  @return DynList containing all the results.
 *
 *  @throw Any exception thrown by any of the futures will propagate.
 *
 *  @see get_futures(DynList<std::future<T>> &)
 */
template <typename T>
[[nodiscard]] inline DynList<T>
get_futures(DynList<std::future<T>> && future_list)
{
  return get_futures(future_list);
}

/** @brief Wait for all void futures to complete.
 *
 *  Waits for each void future in the list to complete. Since void futures
 *  don't return values, this function simply ensures all async operations
 *  have finished. The futures are consumed from the input list.
 *
 *  @param[in,out] future_list List of void futures to wait for. Will be
 *                             empty after the call.
 *
 *  @throw Any exception thrown by any of the futures will propagate.
 *
 *  @note This function blocks until all futures have completed.
 *
 *  @see std::future<void>
 */
inline void get_futures(DynList<std::future<void>> & future_list)
{
  while (!future_list.is_empty())
    future_list.remove_first().get();
}

/** @brief Wait for all void futures to complete (rvalue overload).
 *
 *  Overload for rvalue references, allowing temporary lists to be passed.
 *
 *  @param[in] future_list Temporary list of void futures to wait for.
 *
 *  @throw Any exception thrown by any of the futures will propagate.
 *
 *  @see get_futures(DynList<std::future<void>> &)
 */
inline void get_futures(DynList<std::future<void>> && future_list)
{
  get_futures(future_list);
}

/** @brief Check if all futures in a list are ready.
 *
 *  Non-blocking check to determine if all futures have completed.
 *  Does not consume the futures, but may modify their internal state
 *  (wait_for is not const in the standard).
 *
 *  @tparam T The result type of the futures.
 *
 *  @param[in,out] future_list List of futures to check.
 *
 *  @return true if all futures are ready, false otherwise.
 */
template <typename T>
[[nodiscard]] inline bool
all_ready(DynList<std::future<T>> & future_list)
{
  for (auto it = future_list.get_it(); it.has_curr(); it.next())
    if (it.get_curr().wait_for(std::chrono::seconds(0)) != std::future_status::ready)
      return false;
  return true;
}

/** @brief Count how many futures are ready.
 *
 *  Non-blocking count of futures that have completed.
 *
 *  @tparam T The result type of the futures.
 *
 *  @param[in,out] future_list List of futures to check.
 *
 *  @return Number of futures that are ready.
 */
template <typename T>
[[nodiscard]] inline size_t
count_ready(DynList<std::future<T>> & future_list)
{
  size_t count = 0;
  for (auto it = future_list.get_it(); it.has_curr(); it.next())
    if (it.get_curr().wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      ++count;
  return count;
}

} // end namespace Aleph

#endif // FUTURE_UTILS_H