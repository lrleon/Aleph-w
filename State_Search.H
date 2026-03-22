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

/** @file State_Search.H
 *  @brief Umbrella header for the implicit state-space search framework.
 *
 *  Include this header to obtain the current common infrastructure, the
 *  reusable depth-first backtracking engine, the branch-and-bound optimization
 *  layer, the shared move-ordering and memo/transposition support, and the
 *  adversarial-search engines (`Negamax`, `Alpha_Beta`) plus iterative
 *  deepening, aspiration-window control and tracing helpers for that layer.
 *
 *  It also exports namespace @ref Aleph::Search, a light user-facing facade
 *  with friendlier aliases and one-shot helper functions.
 *
 *  @author Leandro Rabindranath León
 */

# ifndef STATE_SEARCH_H
# define STATE_SEARCH_H

#include <search_move_ordering.H>
#include <state_search_common.H>
#include <Transposition_Table.H>
#include <Backtracking.H>
#include <Branch_And_Bound.H>
#include <Negamax.H>
#include <Alpha_Beta.H>
#include <State_Search_IDA_Star.H>

namespace Aleph {

/** @brief User-facing facade exported by the umbrella search header.
 *
 *  `State_Search.H` is the recommended entry point for users who want the
 *  current framework as one coherent module. This namespace keeps the
 *  underlying APIs untouched while exposing a smaller, easier-to-discover
 *  surface:
 *
 *  - shared policy, limit and status types,
 *  - friendlier engine aliases (`Backtracking`, `BranchAndBound`,
 *    `NegamaxEngine`, `AlphaBeta`),
 *  - iterative-deepening and tracing aliases for adversarial search,
 *  - one-shot helper functions (`backtracking_search()`,
 *    `branch_and_bound_search()`, `negamax_search()`, `alpha_beta_search()`,
 *    `iterative_deepening_negamax_search()`,
 *    `iterative_deepening_alpha_beta_search()`),
 *  - `Unlimited` as the common sentinel for unbounded limits.
 *
 *  Users who prefer narrower includes can still include the individual module
 *  headers directly and use the original names in namespace @ref Aleph.
 */
namespace Search {

inline constexpr size_t Unlimited = Search_Unlimited;  ///< Alias for @ref Search_Unlimited.

using Aleph::ExplorationPolicy;
using Aleph::MoveOrderingMode;
using Aleph::SearchLimits;
using Aleph::SearchPath;
using Aleph::SearchResult;
using Aleph::SearchSolution;
using Aleph::SearchSolutionCollector;
using Aleph::SearchStats;
using Aleph::SearchStatus;
using Aleph::SearchStorageMap;
using Aleph::SearchStorageSet;

using Aleph::AdversarialSearchResult;
using Aleph::AdversarialTraceEventKind;
using Aleph::AdversarialTranspositionTable;
using Aleph::AspirationWindow;
using Aleph::BestSolution;
using Aleph::Maximize_Objective;
using Aleph::Minimize_Objective;
using Aleph::OptimizationSolution;
using Aleph::SearchSolutionVisitor;

template <typename Domain>
using Backtracking = Depth_First_Backtracking<Domain>;

template <typename Domain, typename ObjectivePolicy = Maximize_Objective<typename Domain::Objective>>
using BranchAndBound = Branch_And_Bound<Domain, ObjectivePolicy>;

template <typename Domain>
using NegamaxEngine = Negamax<Domain>;

template <typename Domain>
using AlphaBeta = Alpha_Beta<Domain>;

template <IDAStarDomain Domain>
using IDAStar = IDA_Star_State_Search<Domain>;

template <AdversarialScore Score>
using IterativeDeepeningOptions = AdversarialIterativeDeepeningOptions<Score>;

template <SearchMove Move, AdversarialScore Score>
using IterativeDeepeningResult = AdversarialIterativeDeepeningResult<Move, Score>;

template <SearchMove Move, AdversarialScore Score>
using TraceEvent = AdversarialTraceEvent<Move, Score>;

template <SearchMove Move, AdversarialScore Score>
using TraceCollector = AdversarialTraceCollector<Move, Score>;

template <typename Solution>
using Collector = SearchSolutionCollector<Solution>;

template <typename StateKey,
          SearchMove Move,
          AdversarialScore Score,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<AdversarialTranspositionKey<StateKey>>>
using GameTT = AdversarialTranspositionTable<StateKey, Move, Score, HashMapTable, Cmp>;

using Aleph::alpha_beta_search;
using Aleph::backtracking_search;
using Aleph::branch_and_bound_search;
using Aleph::ida_star_search;
using Aleph::iterative_deepening_alpha_beta_search;
using Aleph::iterative_deepening_negamax_search;
using Aleph::negamax_search;

}  // end namespace Search

}  // end namespace Aleph

# endif  // STATE_SEARCH_H
