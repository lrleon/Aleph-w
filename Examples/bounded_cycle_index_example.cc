
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
 * @file bounded_cycle_index_example.cc
 * @brief Example: the cycles of a market that a quote can make profitable,
 *        with `Bounded_Cycle_Index`.
 *
 * A market is a digraph whose nodes are assets and whose arcs are trades:
 * the arc `a -> b` carries the rate at which one unit of `a` turns into
 * units of `b`, fees included. A cycle is profitable when the product of
 * its rates exceeds one. A quote changes the two arcs of one market, and
 * only the cycles through them can change their answer.
 *
 * `Bounded_Cycle_Index` enumerates once the cycles of the lengths wanted
 * and, for each change, looks only at the cycles of the arc that changed.
 * It works with one `double` for each arc, `-log(rate)`, so that a
 * profitable cycle has a negative sum, and it hands the cycles whose sum
 * is below a bound. That sum is rounded: the index does not decide. The
 * bound is given a margin, so that no profitable cycle is lost, and each
 * cycle that the index hands is decided here with the exact product of
 * its rates, which are rationals. The index spares the exact product of
 * the cycles that are far from being profitable, which are nearly all.
 *
 * The example shows:
 *
 *  - the catalog: the numbers of the arcs, the cycles by length and the
 *    cycles of an arc;
 *  - `set_weight()` for all the arcs and `for_each_cycle_below()` for the
 *    whole catalog, to start;
 *  - a quote: the two arcs of a market changed with `set_weight()`, and
 *    then `for_each_cycle_below()` with both, which sees the two changes
 *    at once and hands each cycle once;
 *  - `update()`, for the change of one arc;
 *  - an arc that cannot be used, with the weight plus infinity;
 *  - `stamp()` and `is_stale()`, to know whether what was computed of a
 *    cycle is still good.
 */

# include <cmath>
# include <iostream>
# include <limits>
# include <string>

# include <gmpxx.h>

# include <Bounded_Cycle_Index.H>
# include <tpl_array.H>
# include <tpl_const_graph_view.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  struct Trade
  {
    string name;    // the market and the side, which identify the trade
    mpq_class rate; // exact, fees included
  };

  using Market = List_Digraph<Graph_Node<string>, Graph_Arc<Trade>>;
  using View = Const_Graph_View<Market>;
  using Index = Bounded_Cycle_Index<View>;

  // A rate from its digits. GMP keeps the fraction as it is written, and
  // its arithmetic needs it in lowest terms
  mpq_class exact_rate(const char *fraction)
  {
    mpq_class rate(fraction);
    rate.canonicalize();
    return rate;
  }

  // The weight of a trade: a profitable cycle has a negative sum
  double weight_of(const mpq_class & rate) { return -log(rate.get_d()); }

  // The margin of the bound. The sum of at most 4 weights below 64 is
  // within sum_error_bound() of the sum of those weights, and each weight
  // is within a few units in its last place of the exact -log(rate): one
  // part in a thousand millions covers both with much room.
  constexpr double Margin = 1e-9;
  static_assert(Index::sum_error_bound(4, 64) < Margin);

  // The exact rule: the product of the rates of the cycle exceeds one
  bool is_profitable(const Index & index, const size_t position)
  {
    mpq_class product = 1;
    for (const auto number : index.cycle_arcs(position))
      product *= index.arc(number)->get_info().rate;
    return product > 1;
  }

  string path_of(const View & market, const Index & index, const size_t position)
  {
    string ret;
    for (const auto number : index.cycle_arcs(position))
      ret += market.get_src_node(index.arc(number))->get_info() + " -> ";
    return ret + market.get_src_node(index.arc(index.cycle_arcs(position).front()))->get_info();
  }

  // What to do with a cycle that the index hands: decide it exactly
  struct Decide
  {
    const View & market;
    const Index & index;
    size_t candidates = 0;
    size_t profitable = 0;

    void operator () (const size_t position, const double sum)
    {
      ++candidates;
      const bool yes = is_profitable(index, position);
      profitable += yes;
      cout << "    " << path_of(market, index, position) << ": sum " << sum
           << (yes ? ", profitable" : ", not profitable") << '\n';
    }
  };
}

int main()
{
  Market market;
  const auto usd = market.insert_node("USD");
  const auto eur = market.insert_node("EUR");
  const auto gbp = market.insert_node("GBP");
  const auto jpy = market.insert_node("JPY");

  // Each market has two trades; the rates are decimals, taken exactly
  const auto trade = [&market] (Market::Node *from, Market::Node *to, const string & name,
                                const char *rate)
    {
      return market.insert_arc(from, to, Trade{name, exact_rate(rate)});
    };
  const auto eurusd_sell_arc = trade(eur, usd, "EURUSD sell", "10850/10000");
  const auto eurusd_buy_arc = trade(usd, eur, "EURUSD buy", "10000/10852");
  trade(gbp, usd, "GBPUSD sell", "12700/10000");
  trade(usd, gbp, "GBPUSD buy", "10000/12703");
  const auto eurgbp_sell_arc = trade(eur, gbp, "EURGBP sell", "8540/10000");
  trade(gbp, eur, "EURGBP buy", "10000/8543");
  trade(usd, jpy, "USDJPY sell", "15000/100");
  trade(jpy, usd, "USDJPY buy", "100/15003");
  trade(eur, jpy, "EURJPY sell", "16270/100");
  trade(jpy, eur, "EURJPY buy", "100/16275");

  // The index reads the market through a view: it cannot change it. The
  // arcs take their numbers in the order of their names, whatever the
  // order in which they were inserted.
  const View view(market);
  Index index(view, {2, 3, 4}, [] (const Market::Arc *a, const Market::Arc *b)
    {
      return a->get_info().name < b->get_info().name;
    });

  cout << "Arcs: " << index.num_arcs() << "; cycles: " << index.num_cycles() << " ("
       << index.num_cycles(2) << " of 2 arcs, " << index.num_cycles(3) << " of 3, "
       << index.num_cycles(4) << " of 4)\n";
  const size_t eurusd_sell = index.arc_number(eurusd_sell_arc);
  const size_t eurusd_buy = index.arc_number(eurusd_buy_arc);
  cout << "Arc " << eurusd_sell << " is \"" << index.arc(eurusd_sell)->get_info().name
       << "\", in " << index.cycles_of(eurusd_sell).size() << " cycles\n\n";

  // To start: every weight, and then the whole catalog. One cycle has a
  // product of exactly one: its sum is rounded, not zero, the margin
  // hands it and the exact rule says that it is not profitable.
  for (size_t a = 0; a < index.num_arcs(); ++a)
    index.set_weight(a, weight_of(index.arc(a)->get_info().rate));
  Decide decide{view, index};
  cout << "With the first quotes:\n";
  const size_t all = index.for_each_cycle_below(Margin, 4, decide);
  cout << "  " << all << " cycles looked at, " << decide.candidates << " candidates, "
       << decide.profitable << " profitable\n\n";

  // A quote of EURUSD changes its two trades. Both are written before
  // any cycle is looked at, and each cycle is handed once.
  const auto quote = [&] (const char *bid, const char *ask_inverse)
    {
      eurusd_sell_arc->get_info().rate = exact_rate(bid); // the owner of the market writes it
      eurusd_buy_arc->get_info().rate = exact_rate(ask_inverse);
      index.set_weight(eurusd_sell, weight_of(eurusd_sell_arc->get_info().rate));
      index.set_weight(eurusd_buy, weight_of(eurusd_buy_arc->get_info().rate));
      decide.candidates = decide.profitable = 0;
      const size_t seen = index.for_each_cycle_below({eurusd_sell, eurusd_buy}, Margin, 4, decide);
      cout << "  " << seen << " cycles looked at, " << decide.candidates << " candidates, "
           << decide.profitable << " profitable\n\n";
    };
  cout << "EURUSD rises to 1.0880 / 1.0882:\n";
  quote("10880/10000", "10000/10882");

  cout << "EURUSD falls back to 1.0850 / 1.0852:\n";
  quote("10850/10000", "10000/10852");

  // The change of one arc: update() writes it and looks at its cycles
  const size_t eurgbp_sell = index.arc_number(eurgbp_sell_arc);
  eurgbp_sell_arc->get_info().rate = exact_rate("8600/10000");
  cout << "EURGBP sell at 0.8600:\n";
  decide.candidates = decide.profitable = 0;
  const uint64_t before = index.clock();
  const size_t seen = index.update(eurgbp_sell, weight_of(eurgbp_sell_arc->get_info().rate),
                                   Margin, 4, decide);
  cout << "  " << seen << " cycles looked at, " << decide.candidates << " candidates, "
       << decide.profitable << " profitable\n\n";

  // What was computed before that change is old for the cycles of the arc
  size_t stale = 0;
  for (size_t position = 0; position < index.num_cycles(); ++position)
    stale += index.is_stale(position, before);
  cout << stale << " cycles are stale since the clock was " << before << ": those of arc "
       << eurgbp_sell << ", which are " << index.cycles_of(eurgbp_sell).size() << "\n\n";

  // A market that stops: its trades cannot be used, and no cycle through
  // them is a candidate
  index.set_weight(eurgbp_sell, numeric_limits<double>::infinity());
  decide.candidates = decide.profitable = 0;
  index.for_each_cycle_below(Margin, 4, decide);
  cout << "Without EURGBP sell: " << decide.candidates << " candidates\n";

  return 0;
}
