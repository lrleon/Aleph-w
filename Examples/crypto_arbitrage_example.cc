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
 * @file crypto_arbitrage_example.cc
 * @brief Example: triangular arbitrage detection in a market of currency pairs.
 *
 * A market is a digraph whose nodes are assets and whose arcs are trades. An
 * arc `a -> b` carries the rate at which one unit of `a` turns into units of
 * `b`. Trading around a cycle multiplies the rates, so the cycle is an
 * arbitrage opportunity exactly when the product of its rates, after fees,
 * exceeds one. Taking logarithms turns the product into a sum:
 *
 *   weight(a -> b) = -log(rate * (1 - fee))
 *
 * and an opportunity is a cycle of negative total weight.
 *
 * The example shows how to use the negative-cycle tools of Aleph-w on it:
 *
 *  - `find_disjoint_negative_cycles()` lists several opportunities at once, not
 *    just the first one `Bellman_Ford` finds, and with
 *    `Negative_Cycle_Exclusion::All_Arcs` no two of them share a trade. That
 *    only diversifies the candidates: two cycles that share no trade can still
 *    need the same balance, so whether they can be executed together depends on
 *    capital, quantities and liquidity, which the caller has to check.
 *  - `most_negative_cycle_bounded()` finds the most profitable cycle of at most
 *    a given number of trades. It ranks by total profit, unlike
 *    `karp_minimum_mean_cycle()`, which ranks by profit per trade and is shown
 *    for comparison.
 *  - A minimum margin is demanded by adding it to the weight of every arc
 *    inside the distance functor (see `Log_Rate_Distance`). The library applies
 *    no threshold of its own: the margin that covers latency and slippage is a
 *    business decision, and the functor is where it belongs.
 *
 * ## Using it against a live price feed
 *
 * The searches take a `const` graph and keep their state in private arrays, so
 * several threads may search the same graph as long as nobody modifies it. A
 * price feed does modify it, so the search needs a consistent version of the
 * market: either copy the graph while holding the same lock the feed takes to
 * write it (copying alone does not synchronize anything), or have the feed
 * publish immutable versions. A result refers to the graph that was searched,
 * and its pointers are valid only while that graph is. To act on a result,
 * identify each trade by something stable stored in the arc (an instrument id,
 * venue and side) rather than by pointer: `GraphCopyWithMapping` (tpl_graph.H)
 * only maps original nodes to copied ones (`get_copy(original)`), not back, and
 * does not map arcs, which parallel trades between the same assets need. This
 * example does not implement a feed.
 *
 * Build and run:
 *
 *   cmake -S . -B build -DBUILD_EXAMPLES=ON
 *   cmake --build build --target crypto_arbitrage_example
 *   ./build/Examples/crypto_arbitrage_example
 */

# include <cmath>
# include <iomanip>
# include <iostream>
# include <string>
# include <utility>

# include <Min_Mean_Cycle.H>
# include <Negative_Cycles.H>
# include <tpl_array.H>
# include <tpl_dynMapTree.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  /// Nodes are assets; the information of an arc is its trade rate.
  using Market = List_Digraph<Graph_Node<string>, Graph_Arc<double>>;
  using Asset = Market::Node;
  using Trade = Market::Arc;

  /// Fee charged by the exchange on every trade (0.1 %).
  constexpr double trading_fee = 0.001;

  /**
   * @brief Arc weight `-log(rate * (1 - fee)) + margin`.
   *
   * This is where a minimum margin is demanded: every arc weighs `margin` more,
   * so a cycle of `k` trades is reported only if its profit exceeds `k * margin`
   * in log terms. Note that the threshold grows with the length of the cycle;
   * it is not a fixed margin per cycle.
   */
  struct Log_Rate_Distance
  {
    using Distance_Type = double;

    double fee = trading_fee;
    double margin = 0.0;

    /**
     * @brief Weight of a trade.
     * @param trade Arc whose information is the trade rate.
     * @return `-log(rate * (1 - fee)) + margin`.
     */
    double operator()(Trade * trade) const
    {
      return -std::log(trade->get_info() * (1.0 - fee)) + margin;
    }
  };

  /// A currency pair as quoted by an exchange: the price of one unit of `base`
  /// expressed in units of `quote`.
  struct Pair
  {
    const char * base;
    const char * quote;
    double distortion;   ///< Relative mispricing seeded on purpose (0 = fair).
  };

  /// Reference values in USDT, from which consistent prices are derived.
  double usdt_value(const string & asset)
  {
    if (asset == "BTC")
      return 60000.0;
    if (asset == "ETH")
      return 3000.0;
    if (asset == "BNB")
      return 600.0;
    if (asset == "SOL")
      return 150.0;
    if (asset == "ADA")
      return 0.5;
    return 1.0;   // USDT
  }

  /// The market: ten pairs, all consistent except two seeded mispricings. Each
  /// one makes every cycle that goes through the mispriced trade profitable:
  /// ETH/BTC gives about +0.7 % after fees (BTC -> USDT -> ETH -> BTC, and a
  /// longer one through BNB), ADA/BNB about +0.5 % (ADA -> BNB -> USDT -> ADA,
  /// and ADA -> BNB -> SOL -> ADA). The cycles of one mispricing share its
  /// trade and `All_Arcs` keeps only one of them, so one search reports one
  /// opportunity per mispricing.
  const Pair pairs[] =
    {
      {"BTC", "USDT", 0.0},
      {"ETH", "USDT", 0.0},
      {"ETH", "BTC", 0.010},    // ETH is 1.0 % too expensive in BTC
      {"BNB", "USDT", 0.0},
      {"SOL", "USDT", 0.0},
      {"SOL", "BNB", 0.0},
      {"ADA", "USDT", 0.0},
      {"ADA", "SOL", 0.0},
      {"ADA", "BNB", 0.008},    // ADA is 0.8 % too expensive in BNB
      {"BNB", "BTC", 0.0},
    };

  /// Builds the market digraph: each pair gives two arcs, buying the base with
  /// the quote (rate `1 / price`) and selling the base for the quote (rate
  /// `price`).
  void build_market(Market & market)
  {
    DynMapTree<string, Asset *> assets;
    auto asset_of = [&](const char * name) -> Asset *
    {
      const string key = name;
      if (not assets.has(key))
        assets.insert(key, market.insert_node(key));
      return assets.find(key);
    };

    for (const Pair & p : pairs)
      {
        const double price = usdt_value(p.base) / usdt_value(p.quote) * (1.0 + p.distortion);
        Asset * base = asset_of(p.base);
        Asset * quote = asset_of(p.quote);
        market.insert_arc(quote, base, 1.0 / price);   // buy the base
        market.insert_arc(base, quote, price);         // sell the base
      }
  }

  /// A cycle of trades in a form that does not depend on where the search
  /// happened to start it.
  struct Opportunity
  {
    string route;        ///< Assets in trading order, starting at the smallest name.
    size_t trades = 0;   ///< Number of trades (arcs).
    double gain = 0.0;   ///< Relative gain after fees, e.g. 0.007 = +0.7 %.
  };

  /**
   * @brief Describe a cycle given by its closed list of assets and its trades.
   *
   * The route is rotated to start at the asset with the smallest name, so the
   * output does not depend on which asset the search chose as a starting point.
   *
   * @param assets Closed walk: the first asset is repeated at the end.
   * @param trades Trades aligned with the assets.
   * @return The opportunity, with its gain recomputed from the rates and the fee.
   */
  Opportunity describe(const DynList<Asset *> & assets, const DynList<Trade *> & trades)
  {
    Array<string> names;
    assets.for_each([&names](Asset * a) { names.append(a->get_info()); });
    (void) names.remove_last();   // drop the repeated first asset

    size_t first = 0;
    for (size_t i = 1; i < names.size(); ++i)
      if (names(i) < names(first))
        first = i;

    Opportunity op;
    for (size_t i = 0; i < names.size(); ++i)
      op.route += names((first + i) % names.size()) + " -> ";
    op.route += names(first);
    op.trades = names.size();

    double growth = 1.0;
    trades.for_each([&growth](Trade * t) { growth *= t->get_info() * (1.0 - trading_fee); });
    op.gain = growth - 1.0;
    return op;
  }

  void print_opportunity(const Opportunity & op)
  {
    cout << "  " << op.route << "  (" << op.trades << " trades, gain "
         << showpos << fixed << setprecision(3) << op.gain * 100.0 << "%" << noshowpos << ")\n";
  }

  /// Lists the opportunities of `market` under a minimum per-arc `margin`,
  /// most profitable first.
  size_t list_opportunities(const Market & market, const double margin, const size_t max_cycles)
  {
    const auto cycles = find_disjoint_negative_cycles(
        market, max_cycles, Negative_Cycle_Exclusion::All_Arcs,
        Log_Rate_Distance{trading_fee, margin});

    Array<Opportunity> found;
    for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
      found.append(describe(it.get_curr().cycle.nodes(), it.get_curr().cycle.arcs()));

    // At most a handful of items: a plain insertion sort, best gain first.
    for (size_t i = 1; i < found.size(); ++i)
      for (size_t j = i; j > 0 and found(j).gain > found(j - 1).gain; --j)
        swap(found(j), found(j - 1));

    for (size_t i = 0; i < found.size(); ++i)
      print_opportunity(found(i));
    return found.size();
  }

  void show_bounded(const Market & market, const size_t max_trades)
  {
    const auto r = most_negative_cycle_bounded(market, max_trades,
                                               Log_Rate_Distance{trading_fee, 0.0});
    cout << "  at most " << max_trades << " trades: ";
    if (not r.is_negative())
      {
        cout << "no profitable cycle\n";
        return;
      }

    // With floating-point weights the search proves nothing (`is_exact` is
    // false); `matches_relaxed_bound` says that it found no better cycle in its
    // own rounded arithmetic, which is what a log-rate market can expect.
    const Opportunity op = describe(r.cycle_nodes, r.cycle_arcs);
    const char * quality = r.is_exact                ? "proven optimal"
                           : r.matches_relaxed_bound ? "optimal as computed in floating point, not a proof"
                                                     : "not certified";
    cout << op.route << "  (gain " << showpos << fixed << setprecision(3) << op.gain * 100.0
         << "%" << noshowpos << ", " << quality << ")\n";
  }
} // namespace


int main()
{
  Market market;
  build_market(market);

  cout << "Crypto arbitrage reference example\n";
  cout << "Market: " << market.get_num_nodes() << " assets, "
       << market.get_num_arcs() / 2 << " pairs, fee " << fixed << setprecision(2)
       << trading_fee * 100.0 << "% per trade\n";

  cout << "\nOpportunities, no minimum margin:\n";
  const size_t unconstrained = list_opportunities(market, 0.0, 5);
  cout << "Opportunities found: " << unconstrained << "\n";

  // Option A of the plan: the margin lives in the distance functor. 0.2 % per
  // trade asks 0.6 % from a triangle, so the thinner one disappears.
  cout << "\nOpportunities with a minimum margin of 0.2% per trade:\n";
  const size_t with_margin = list_opportunities(market, 0.002, 5);
  cout << "Opportunities found: " << with_margin << "\n";

  cout << "\nMost profitable cycle by total gain, bounded by its length:\n";
  show_bounded(market, 2);   // a round trip can only lose the fees
  show_bounded(market, 3);

  // Karp ranks by the mean weight of the arcs of the cycle, not by the total.
  const auto karp = karp_minimum_mean_cycle(market, Log_Rate_Distance{trading_fee, 0.0});
  cout << "\nKarp minimum mean cycle (profit per trade, not total profit):\n";
  const Opportunity mean_cycle = describe(karp.cycle_nodes, karp.cycle_arcs);
  cout << "  " << mean_cycle.route << "  (mean log-weight " << fixed << setprecision(5)
       << static_cast<double>(karp.minimum_mean) << " per trade)\n";

  return 0;
}
