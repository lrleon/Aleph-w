
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

/** @file Bounded_Cycle_Index.H
 *  @brief Index of the simple cycles of bounded length of a digraph, with
 *         a weight for each arc and a filter by the weight of a cycle.
 *
 *  `Bounded_Cycle_Index` enumerates once the simple directed cycles of a
 *  digraph whose lengths are in a set, keeps for each arc the cycles that
 *  go through it, and, when the weight of an arc changes, hands to a
 *  function the cycles of that arc whose weight is below a bound. It is
 *  made for graphs whose shape does not change while the weights of their
 *  arcs do, as a graph of markets, where a quote changes two arcs and
 *  only the cycles through them need to be looked at again.
 *
 *  ## The catalog
 *
 *  The arcs take the numbers 0, 1, ... in the order of a comparison given
 *  by the caller; the arcs that it does not tell apart keep the order of
 *  the arc iterator of the graph. A cycle is the sequence of the numbers
 *  of its arcs, rotated so that it starts with the least of them, which is
 *  its canonical rotation: each cycle is in the catalog once, parallel
 *  arcs give different cycles, and a loop is a cycle of length 1. The
 *  cycles are ordered by length and, with the same length,
 *  lexicographically by the numbers of their arcs; the position of a
 *  cycle in that order identifies it. So the cycles whose length is at
 *  most `m` are a prefix of the catalog, with the positions that they
 *  have in the catalog of that smaller set of lengths.
 *
 *  ## The weights and the filter
 *
 *  Each arc has a weight, a `double`, which the index keeps apart from
 *  the graph. The weight of a cycle is the sum of the weights of its
 *  arcs, added in the order of its canonical rotation from the weights of
 *  the moment: no sum is kept from one change to the next, so that no
 *  error is carried, and the sum of a cycle is the same number whichever
 *  of its arcs changed. An arc that cannot be used has the weight
 *  \f$+\infty\f$, which is the weight of every arc until it is given
 *  one: no cycle through it is below a bound.
 *
 *  `update()` changes the weight of an arc and hands the cycles of that
 *  arc whose sum is below a bound. To change several arcs before looking
 *  at any cycle, as the two arcs of a market, change them with
 *  `set_weight()` and then call `for_each_cycle_below()` with those arcs,
 *  which hands each cycle once, also when several of them are in it.
 *
 *  ## The bound, with a margin
 *
 *  The filter is exact for the weights that it is given: it compares the
 *  sum, in `double`, with the bound. When the weights approximate exact
 *  quantities, as \f$w = -\log r\f$ for a rate \f$r\f$, a cycle whose
 *  exact product \f$P\f$ exceeds a threshold \f$q\f$ has an exact sum
 *  \f$S = -\log P\f$ below \f$t = -\log q\f$. If the sum that the index
 *  computes is within \f$\varepsilon_S\f$ of \f$S\f$ and the bound that
 *  the caller computes is within \f$\varepsilon_t\f$ of \f$t\f$, no such
 *  cycle is lost with the bound
 *
 *  \f[ t_{approx} + \varepsilon_S + \varepsilon_t \f]
 *
 *  The margin widens what is accepted: it is added to the bound, not
 *  subtracted from it. Those additions are rounded as well, and a bound
 *  that is rounded down loses the cycle whose sum equals it: the bound
 *  given must not be less than that expression. Round it up, with
 *  `std::nextafter()` towards \f$+\infty\f$ after each addition, or
 *  take a margin that is wider than the two errors. The cycles that the
 *  margin lets through are candidates, to be decided with the exact
 *  rule. `sum_error_bound()` gives the part
 *  of \f$\varepsilon_S\f$ that is of the index, the rounding of the sum;
 *  the error of each weight, that of the conversion of the rate and that
 *  of its logarithm, is of who computes the weights. With several
 *  thresholds, give the widest bound and classify the cycle with the sum
 *  that the function receives.
 *
 *  ## What does not change
 *
 *  The index keeps pointers to the arcs: the graph must outlive it, at
 *  the same address, with the nodes and the arcs that it had when the
 *  index was built. It does not use the cookies, the counters or the bits
 *  of the nodes and of the arcs, so that it works on a `const` graph and
 *  on a `Const_Graph_View`. It is not thread safe.
 *
 *  @code
 *  using Market = List_Digraph<Graph_Node<std::string>, Graph_Arc<double>>;
 *
 *  Bounded_Cycle_Index index(const_view(market), {3, 4});
 *  const size_t a = index.arc_number(arc);
 *  index.update(a, -std::log(rate), 1e-9, 4, [&] (size_t position, double sum)
 *    {
 *      for (const auto number : index.cycle_arcs(position))
 *        use(index.arc(number));  // a candidate: decide it exactly
 *    });
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef BOUNDED_CYCLE_INDEX_H
# define BOUNDED_CYCLE_INDEX_H

# include <cmath>
# include <concepts>
# include <cstddef>
# include <cstdint>
# include <initializer_list>
# include <limits>
# include <span>
# include <utility>

# include <ah-concepts.H>
# include <ah-errors.H>
# include <ah-graph-concepts.H>
# include <tpl_array.H>
# include <tpl_graph.H>
# include <tpl_sort_utils.H>

namespace Aleph
{
  /** @brief The order of the arc iterator of the graph, as a comparison
   *         for `Bounded_Cycle_Index`: it tells no arc apart from another.
   */
  struct Arc_List_Order
  {
    /// Return false: no arc goes before another.
    [[nodiscard]] constexpr bool operator () (const void *, const void *) const noexcept
    {
      return false;
    }
  };

  /** @brief Index of the simple cycles of bounded length of a digraph.
   *
   *  See the description of the file for the catalog, the weights, the
   *  filter and its margin.
   *
   *  The index is built in \f$O(V + E \log E + C k^2)\f$ time, with the
   *  sort of the arcs at its expected cost, besides the search, which
   *  walks each simple path of fewer arcs than the longest length whose
   *  first arc is its least one; `C` is the number of cycles and `k`
   *  their length. It takes \f$O(E + C k^2)\f$ memory: for each arc, the
   *  arcs of each of its cycles are kept contiguous, so that a change of
   *  the arc reads them in a row. `update()` takes \f$O(c\,k)\f$ for an
   *  arc that is in `c` cycles.
   *
   *  An index can be copied, assigned, moved and exchanged with
   *  `swap()`. A copy is another index of the same graph, with the
   *  weights, the stamps and the clock of the first. An index that is
   *  moved from is left without arcs and without cycles.
   *
   *  @tparam GT the type of the digraph: a graph class of Aleph or a
   *          `Const_Graph_View` of one.
   *  @ingroup Graphs
   */
  template <AlephGraph GT>
  class Bounded_Cycle_Index
  {
  public:
    using Arc = typename GT::Arc; ///< The type of the arcs, `const` on a view.

    /// The longest length of a cycle that an index can hold.
    static constexpr size_t Max_Length = 8;

    /// The number of cycles that an index holds, when no other is given.
    static constexpr size_t Default_Max_Cycles = size_t(1) << 24;

  private:
    // The counts of the build are checked against 2^32 in a size_t
    static_assert(sizeof(size_t) >= 8, "Bounded_Cycle_Index needs a size_t of 64 bits");

    static constexpr uint32_t Not_Found = std::numeric_limits<uint32_t>::max();
    static constexpr size_t Stride = Max_Length + 2; // entries of an arc, by length

    // Numbers by address: an open table with linear probing, as that of
    // In_Arcs_Index, with 2^bits places, at least twice the number of keys.
    // A table that was moved from has no place: its bits are 0.
    struct Number_Table
    {
      unsigned bits = 1;
      Array<const void *> keys = Array<const void *>(2, nullptr);
      Array<uint32_t> numbers = Array<uint32_t>(2, uint32_t(0));

      Number_Table() = default;

      explicit Number_Table(const size_t n)
      {
        while ((size_t(1) << bits) < 2 * n)
          ++bits;
        keys = Array<const void *>(size_t(1) << bits, nullptr);
        numbers = Array<uint32_t>(size_t(1) << bits, uint32_t(0));
      }

      Number_Table(const Number_Table &) = default;

      Number_Table(Number_Table && other) noexcept
        : bits(std::exchange(other.bits, 0u)), keys(std::move(other.keys)),
          numbers(std::move(other.numbers)) {}

      // By value: the table is copied or moved apart, and then exchanged
      Number_Table & operator = (Number_Table other) noexcept
      {
        swap(other);
        return *this;
      }

      [[nodiscard]] size_t place_of(const void *p) const noexcept
      {
        const auto key = std::uint64_t(reinterpret_cast<std::uintptr_t>(p));
        return size_t((key * 0x9E3779B97F4A7C15ULL) >> (64 - bits));
      }

      void put(const void *p, const uint32_t number) noexcept
      {
        const size_t mask = (size_t(1) << bits) - 1;
        size_t i = place_of(p);
        while (keys(i) != nullptr)
          i = (i + 1) & mask;
        keys(i) = p;
        numbers(i) = number;
      }

      [[nodiscard]] uint32_t find(const void *p) const noexcept
      {
        if (bits == 0)
          return Not_Found;
        const size_t mask = (size_t(1) << bits) - 1;
        for (size_t i = place_of(p); keys(i) != nullptr; i = (i + 1) & mask)
          if (keys(i) == p)
            return numbers(i);
        return Not_Found;
      }

      void swap(Number_Table & other) noexcept
      {
        std::swap(bits, other.bits);
        keys.swap(other.keys);
        numbers.swap(other.numbers);
      }
    };

    // The graph while the index is built: by the number of an arc, its
    // ends, and by the number of a node, the arcs that leave it, in
    // ascending order of their numbers
    struct Shape
    {
      Array<uint32_t> source;
      Array<uint32_t> target;
      Array<uint32_t> out_first; // by node, in out_arcs
      Array<uint32_t> out_arcs;
      uint32_t lengths = 0;
      size_t longest = 0;
    };

    // A simple path from the source of its first arc
    struct Path
    {
      uint32_t arcs[Max_Length];
      uint32_t nodes[Max_Length]; // nodes[i]: the source of arcs[i]
      size_t n = 0;
    };

    Number_Table arc_numbers_;         // by the address of an arc
    Array<Arc *> arcs_;                // by number
    uint32_t lengths_ = 0;             // bit k: the length k is in the set
    size_t longest_ = 0;
    size_t length_first_[Stride] = {}; // by length, its first position
    size_t arcs_before_[Stride] = {};  // by length, where its cycles start in cycle_arcs_
    Array<uint32_t> cycle_arcs_;       // the numbers of the arcs of each cycle
    Array<uint32_t> member_first_;     // by arc, in members_
    Array<uint32_t> members_;          // the positions of the cycles of each arc, ascending

    // For each arc and each length, the cycles of the arc with that
    // length: where their positions start in members_, and where their
    // arcs start in the blocks. The cycles of an arc are contiguous in
    // the blocks, each with all its arcs in its canonical rotation.
    Array<uint32_t> group_member_;
    Array<size_t> group_first_;
    Array<uint16_t> narrow_; // the blocks, if the numbers of the arcs fit in 16 bits
    Array<uint32_t> wide_;   // and if they do not

    Array<double> weight_;
    Array<uint64_t> stamp_; // by arc, the value of clock_ when it was last written
    uint64_t clock_ = 0;
    Array<unsigned char> done_; // by arc: for_each_cycle_below() has gone through it

    // Extend the path from `node`, its end, by the arcs after its first
    // one, and hand each cycle that closes with a length of the set
    template <class On_Cycle>
    static void extend(const Shape & shape, Path & path, const uint32_t node,
                       On_Cycle & on_cycle)
    {
      const uint32_t head = path.arcs[0];
      const uint32_t origin = path.nodes[0];
      for (uint32_t i = shape.out_first(node); i < shape.out_first(node + 1); ++i)
        {
          const uint32_t arc = shape.out_arcs(i);
          if (arc <= head)
            continue;
          const uint32_t target = shape.target(arc);
          if (target == origin)
            {
              if ((shape.lengths >> (path.n + 1) & 1u) != 0)
                {
                  path.arcs[path.n] = arc;
                  on_cycle(path.arcs, path.n + 1);
                }
              continue;
            }
          if (path.n + 2 > shape.longest)
            continue;
          bool revisits = target == node; // a loop
          for (size_t j = 1; j < path.n and not revisits; ++j)
            revisits = path.nodes[j] == target;
          if (revisits)
            continue;
          path.arcs[path.n] = arc;
          path.nodes[path.n] = node;
          ++path.n;
          extend(shape, path, target, on_cycle);
          --path.n;
        }
    }

    // Hand each cycle once, in its canonical rotation: with each arc as
    // the first one, in order, the cycles come out in lexicographic order
    template <class On_Cycle>
    static void enumerate(const Shape & shape, On_Cycle && on_cycle)
    {
      Path path;
      const auto num_arcs = uint32_t(shape.source.size());
      for (uint32_t head = 0; head < num_arcs; ++head)
        {
          path.arcs[0] = head;
          if (shape.source(head) == shape.target(head))
            {
              if ((shape.lengths >> 1 & 1u) != 0)
                on_cycle(path.arcs, size_t(1));
              continue;
            }
          if (shape.longest == 1)
            continue; // only loops are asked for: there is no path to extend
          path.nodes[0] = shape.source(head);
          path.n = 1;
          extend(shape, path, shape.target(head), on_cycle);
        }
    }

    template <class It>
    static uint32_t length_bits(It first, const It last)
    {
      uint32_t bits = 0;
      for (; first != last; ++first)
        {
          const size_t k = *first;
          ah_domain_error_if(k < 1 or k > Max_Length)
            << "Bounded_Cycle_Index: the length " << k << " is not between 1 and "
            << Max_Length;
          bits |= 1u << k;
        }
      ah_domain_error_if(bits == 0) << "Bounded_Cycle_Index: no length of cycle";
      return bits;
    }

    template <class Arc_Less>
    void build(const GT & g, const uint32_t lengths, Arc_Less & less,
               const size_t max_cycles)
    {
      ah_domain_error_if(not g.is_digraph())
        << "Bounded_Cycle_Index: the graph must be directed";
      const size_t V = g.get_num_nodes();
      const size_t E = g.get_num_arcs();
      ah_length_error_if(V >= Not_Found or E >= Not_Found)
        << "Bounded_Cycle_Index: the graph has too many nodes or arcs";

      Shape shape;
      shape.lengths = lengths_ = lengths;
      for (size_t k = 1; k <= Max_Length; ++k)
        if ((lengths >> k & 1u) != 0)
          shape.longest = longest_ = k;

      // The arcs with their ends, read from the iterator, in the order
      // of `less` and, where it does not decide, in that of the iterator
      struct Record
      {
        Arc *arc = nullptr;
        uint32_t source = 0;
        uint32_t target = 0;
        uint32_t turn = 0;
      };
      Array<Record> records = Array<Record>::create_reserved(E);
      {
        Number_Table nodes(V);
        uint32_t v = 0;
        for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
          nodes.put(it.get_curr_ne(), v++);
        uint32_t turn = 0;
        for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
          {
            Arc *a = it.get_curr_ne();
            records.append(Record{a, nodes.find(g.get_src_node(a)),
                                  nodes.find(g.get_tgt_node(a)), turn++});
          }
      }
      quicksort_op(records, [&less] (const Record & a, const Record & b)
        {
          if (less(a.arc, b.arc))
            return true;
          return not less(b.arc, a.arc) and a.turn < b.turn;
        });

      arc_numbers_ = Number_Table(E);
      arcs_ = Array<Arc *>::create_reserved(E);
      shape.source = Array<uint32_t>(E, uint32_t(0));
      shape.target = Array<uint32_t>(E, uint32_t(0));
      shape.out_first = Array<uint32_t>(V + 1, uint32_t(0));
      shape.out_arcs = Array<uint32_t>(E, uint32_t(0));
      for (uint32_t a = 0; a < E; ++a)
        {
          arcs_.append(records(a).arc);
          arc_numbers_.put(records(a).arc, a);
          shape.source(a) = records(a).source;
          shape.target(a) = records(a).target;
          ++shape.out_first(shape.source(a) + 1);
        }
      for (size_t v = 1; v <= V; ++v)
        shape.out_first(v) += shape.out_first(v - 1);
      {
        Array<uint32_t> next(V, uint32_t(0));
        for (uint32_t a = 0; a < E; ++a) // in ascending order of number
          shape.out_arcs(shape.out_first(shape.source(a)) + next(shape.source(a))++) = a;
      }

      // First the cycles are counted, by length and by arc, so that the
      // index takes the memory that it needs and no more, and so that a
      // graph with more cycles than the limit takes none
      group_member_ = Array<uint32_t>(E * Stride, uint32_t(0));
      size_t count[Stride] = {};
      size_t num_cycles = 0;
      size_t num_members = 0;
      enumerate(shape, [&] (const uint32_t *arcs, const size_t k)
        {
          ++num_cycles;
          ah_length_error_if(num_cycles > max_cycles)
            << "Bounded_Cycle_Index: the graph has more than " << max_cycles
            << " cycles of the lengths given";
          num_members += k;
          ah_length_error_if(num_members >= Not_Found)
            << "Bounded_Cycle_Index: the cycles have too many arcs";
          ++count[k];
          for (size_t j = 0; j < k; ++j)
            ++group_member_(arcs[j] * Stride + k);
        });

      size_t next_position[Stride] = {};
      for (size_t k = 1, position = 0, arc = 0; k < Stride; ++k)
        {
          length_first_[k] = next_position[k] = position;
          arcs_before_[k] = arc;
          position += count[k];
          arc += count[k] * k;
        }

      // by arc and length, the number of cycles becomes where they start
      member_first_ = Array<uint32_t>(E + 1, uint32_t(0));
      group_first_ = Array<size_t>(E * Stride, size_t(0));
      size_t block_size = 0;
      uint32_t member = 0;
      for (size_t a = 0; a < E; ++a)
        {
          member_first_(a) = member;
          for (size_t k = 0; k < Stride; ++k)
            {
              const uint32_t n = group_member_(a * Stride + k);
              group_member_(a * Stride + k) = member;
              group_first_(a * Stride + k) = block_size;
              member += n;
              block_size += size_t(n) * k;
            }
        }
      if (E > 0)
        member_first_(E) = member;

      // Then they are written, each in its position: those of a length
      // come out in order
      cycle_arcs_ = Array<uint32_t>(num_members, uint32_t(0));
      enumerate(shape, [&] (const uint32_t *arcs, const size_t k)
        {
          const size_t first = arcs_before_[k] + (next_position[k]++ - length_first_[k]) * k;
          for (size_t j = 0; j < k; ++j)
            cycle_arcs_(first + j) = arcs[j];
        });

      // and, by ascending position, each one goes to its arcs
      members_ = Array<uint32_t>(num_members, uint32_t(0));
      const bool is_narrow = E <= size_t(1) << 16;
      narrow_ = Array<uint16_t>(is_narrow ? block_size : 0, uint16_t(0));
      wide_ = Array<uint32_t>(is_narrow ? 0 : block_size, uint32_t(0));
      {
        Array<uint32_t> next_member = Array<uint32_t>::create_reserved(E);
        Array<size_t> next_in_block = Array<size_t>::create_reserved(E);
        for (size_t a = 0; a < E; ++a)
          {
            next_member.append(member_first_(a));
            next_in_block.append(group_first_(a * Stride));
          }
        for (size_t k = 1; k <= Max_Length; ++k)
          for (size_t position = length_first_[k]; position < length_first_[k + 1]; ++position)
            {
              const uint32_t *cycle = &cycle_arcs_(first_arc_of(position, k));
              for (size_t i = 0; i < k; ++i)
                {
                  const uint32_t a = cycle[i];
                  members_(next_member(a)++) = uint32_t(position);
                  size_t & at = next_in_block(a);
                  for (size_t j = 0; j < k; ++j, ++at)
                    if (is_narrow)
                      narrow_(at) = uint16_t(cycle[j]);
                    else
                      wide_(at) = cycle[j];
                }
            }
      }

      weight_ = Array<double>(E, std::numeric_limits<double>::infinity());
      stamp_ = Array<uint64_t>(E, uint64_t(0));
      done_ = Array<unsigned char>(E, static_cast<unsigned char>(0));
    }

    [[nodiscard]] size_t length_of(const size_t position) const noexcept
    {
      size_t k = 1;
      while (position >= length_first_[k + 1])
        ++k;
      return k;
    }

    // Where the arcs of the cycle at `position`, of length k, start
    [[nodiscard]] size_t first_arc_of(const size_t position, const size_t k) const noexcept
    {
      return arcs_before_[k] + (position - length_first_[k]) * k;
    }

    [[nodiscard]] size_t checked_arc(const size_t arc) const
    {
      ah_out_of_range_error_if(arc >= arcs_.size())
        << "Bounded_Cycle_Index: there is no arc with the number " << arc;
      return arc;
    }

    [[nodiscard]] size_t checked_position(const size_t position) const
    {
      ah_out_of_range_error_if(position >= num_cycles())
        << "Bounded_Cycle_Index: there is no cycle at the position " << position;
      return position;
    }

    static void check_weight(const double w)
    {
      ah_domain_error_if(std::isnan(w) or w == -std::numeric_limits<double>::infinity())
        << "Bounded_Cycle_Index: a weight is a number or plus infinity";
    }

    static void check_bound(const double bound)
    {
      ah_domain_error_if(std::isnan(bound)) << "Bounded_Cycle_Index: the bound is not a number";
    }

    // The longest length of the set that is not over max_length
    [[nodiscard]] size_t longest_up_to(const size_t max_length) const noexcept
    {
      return max_length < longest_ ? max_length : longest_;
    }

    // The sum of a cycle of k arcs from `first`: its first weight, plus
    // each of the others in order. look_at() adds the same way.
    [[nodiscard]] double sum_at(const size_t first, const size_t k) const noexcept
    {
      double sum = weight_(cycle_arcs_(first));
      for (size_t j = 1; j < k; ++j)
        sum += weight_(cycle_arcs_(first + j));
      return sum;
    }

    // The cycles of `arc` whose length is at most max_length: each sum,
    // from the block of the arc, and `op` on those below the bound.
    // With `skip`, a cycle through an arc that is marked in done_ is not
    // looked at. Returns how many cycles were looked at.
    template <class Number, class Op>
    size_t look_at(const Array<Number> & block, const size_t arc, const double bound,
                   const size_t max_length, const bool skip, Op & op) const
    {
      const size_t *first = &group_first_(arc * Stride);
      const uint32_t *member = &group_member_(arc * Stride);
      const double *weights = &weight_(0);
      const unsigned char *done = &done_(0);
      size_t seen = 0;
      for (size_t k = 1; k <= max_length; ++k)
        {
          const size_t n = member[k + 1] - member[k];
          if (n == 0)
            continue;
          const Number *o = &block(first[k]);
          const uint32_t *position = &members_(member[k]);
          if (skip)
            for (size_t c = 0; c < n; ++c, o += k)
              {
                double sum = weights[o[0]];
                unsigned char seen_before = done[o[0]];
                for (size_t j = 1; j < k; ++j)
                  {
                    sum += weights[o[j]];
                    seen_before |= done[o[j]];
                  }
                if (seen_before != 0)
                  continue;
                ++seen;
                if (sum < bound) [[unlikely]]
                  op(size_t(position[c]), sum);
              }
          else
            {
              for (size_t c = 0; c < n; ++c, o += k)
                {
                  double sum = weights[o[0]];
                  for (size_t j = 1; j < k; ++j)
                    sum += weights[o[j]];
                  if (sum < bound) [[unlikely]]
                    op(size_t(position[c]), sum);
                }
              seen += n;
            }
        }
      return seen;
    }

    template <class Op>
    size_t look_at(const size_t arc, const double bound, const size_t max_length,
                   const bool skip, Op & op) const
    {
      return narrow_.size() != 0 or wide_.size() == 0
               ? look_at(narrow_, arc, bound, max_length, skip, op)
               : look_at(wide_, arc, bound, max_length, skip, op);
    }

    // Clears the marks of for_each_cycle_below(), also if `op` throws
    struct Marks
    {
      Array<unsigned char> & done;
      const size_t *arcs;
      size_t n;

      ~Marks()
      {
        for (size_t i = 0; i < n; ++i)
          if (arcs[i] < done.size())
            done(arcs[i]) = 0;
      }
    };

    template <class Op>
    size_t look_at_each(const size_t *arcs, const size_t n, const double bound,
                        const size_t max_length, Op & op)
    {
      check_bound(bound);
      for (size_t i = 0; i < n; ++i)
        (void) checked_arc(arcs[i]);
      const size_t longest = longest_up_to(max_length);
      Marks marks{done_, arcs, n};
      size_t seen = 0;
      for (size_t i = 0; i < n; ++i)
        {
          if (done_(arcs[i]) != 0)
            continue; // given again: all its cycles have it marked
          seen += look_at(arcs[i], bound, longest, i > 0, op);
          done_(arcs[i]) = 1;
        }
      return seen;
    }

  public:
    /** @brief Build the index of the cycles of `g` with the lengths given,
     *         with the arcs in the order of a comparison.
     *
     *  @param[in] g the digraph, or a view of it. It must outlive the
     *             index, with the nodes and the arcs that it has now.
     *  @param[in] lengths the lengths of the cycles, each from 1 to
     *             `Max_Length`.
     *  @param[in] less the order of the arcs: `less(a, b)` is true if the
     *             arc `a` takes a number before `b`. It must be a strict
     *             weak order; the arcs that it does not tell apart keep
     *             the order of the arc iterator of `g`. What it throws
     *             goes to the caller, and no index is built.
     *  @param[in] max_cycles the most cycles that the index may hold.
     *  @throw std::domain_error if `g` is not directed, if a length is
     *         not from 1 to `Max_Length` or if there is no length.
     *  @throw std::length_error if `g` has more than `max_cycles` cycles
     *         of those lengths, or more nodes, arcs or arcs of cycles than
     *         fit in 32 bits.
     *  @throw std::bad_alloc if there is no memory.
     */
    template <class Arc_Less>
      requires StrictWeakOrder<Arc_Less, Arc *>
    Bounded_Cycle_Index(const GT & g, std::initializer_list<size_t> lengths, Arc_Less less,
                        const size_t max_cycles = Default_Max_Cycles)
    {
      build(g, length_bits(lengths.begin(), lengths.end()), less, max_cycles);
    }

    /** @brief Build the index of the cycles of `g` with the lengths given,
     *         with the arcs in the order of the arc iterator of `g`.
     *
     *  @param[in] g the digraph, or a view of it. It must outlive the
     *             index, with the nodes and the arcs that it has now.
     *  @param[in] lengths the lengths of the cycles, each from 1 to
     *             `Max_Length`.
     *  @param[in] max_cycles the most cycles that the index may hold.
     *  @throw std::domain_error if `g` is not directed, if a length is
     *         not from 1 to `Max_Length` or if there is no length.
     *  @throw std::length_error if `g` has more than `max_cycles` cycles
     *         of those lengths, or more nodes, arcs or arcs of cycles than
     *         fit in 32 bits.
     *  @throw std::bad_alloc if there is no memory.
     */
    Bounded_Cycle_Index(const GT & g, std::initializer_list<size_t> lengths,
                        const size_t max_cycles = Default_Max_Cycles)
      : Bounded_Cycle_Index(g, lengths, Arc_List_Order(), max_cycles) {}

    /** @brief Build the index of the cycles of `g` with the lengths of an
     *         array, with the arcs in the order of a comparison.
     *
     *  @param[in] g the digraph, or a view of it. It must outlive the
     *             index, with the nodes and the arcs that it has now.
     *  @param[in] lengths the lengths of the cycles, each from 1 to
     *             `Max_Length`.
     *  @param[in] less the order of the arcs, a strict weak order; the
     *             arcs that it does not tell apart keep the order of the
     *             arc iterator of `g`. What it throws goes to the caller,
     *             and no index is built.
     *  @param[in] max_cycles the most cycles that the index may hold.
     *  @throw std::domain_error if `g` is not directed, if a length is
     *         not from 1 to `Max_Length` or if there is no length.
     *  @throw std::length_error if `g` has more than `max_cycles` cycles
     *         of those lengths, or more nodes, arcs or arcs of cycles than
     *         fit in 32 bits.
     *  @throw std::bad_alloc if there is no memory.
     */
    template <class Arc_Less>
      requires StrictWeakOrder<Arc_Less, Arc *>
    Bounded_Cycle_Index(const GT & g, const Array<size_t> & lengths, Arc_Less less,
                        const size_t max_cycles = Default_Max_Cycles)
    {
      uint32_t bits = 0;
      for (size_t i = 0; i < lengths.size(); ++i)
        bits |= length_bits(&lengths(i), &lengths(i) + 1);
      ah_domain_error_if(bits == 0) << "Bounded_Cycle_Index: no length of cycle";
      build(g, bits, less, max_cycles);
    }

    /** @brief Build the index of the cycles of `g` with the lengths of an
     *         array, with the arcs in the order of the arc iterator of
     *         `g`.
     *
     *  @param[in] g the digraph, or a view of it. It must outlive the
     *             index, with the nodes and the arcs that it has now.
     *  @param[in] lengths the lengths of the cycles, each from 1 to
     *             `Max_Length`.
     *  @param[in] max_cycles the most cycles that the index may hold.
     *  @throw std::domain_error if `g` is not directed, if a length is
     *         not from 1 to `Max_Length` or if there is no length.
     *  @throw std::length_error if `g` has more than `max_cycles` cycles
     *         of those lengths, or more nodes, arcs or arcs of cycles than
     *         fit in 32 bits.
     *  @throw std::bad_alloc if there is no memory.
     */
    Bounded_Cycle_Index(const GT & g, const Array<size_t> & lengths,
                        const size_t max_cycles = Default_Max_Cycles)
      : Bounded_Cycle_Index(g, lengths, Arc_List_Order(), max_cycles) {}

    /** @brief Copy an index: its catalog, its weights, its stamps and its
     *         clock.
     *
     *  The copy is another index of the same graph, which must outlive it
     *  too. It is in no query: a copy made by the function that receives
     *  the cycles of `for_each_cycle_below()` hands every cycle that it
     *  is asked for afterwards.
     *
     *  @param[in] other the index to copy.
     *  @throw std::bad_alloc if there is no memory.
     *  @note \f$O(E + C k^2)\f$ time and memory.
     */
    Bounded_Cycle_Index(const Bounded_Cycle_Index & other)
      : arc_numbers_(other.arc_numbers_), arcs_(other.arcs_), lengths_(other.lengths_),
        longest_(other.longest_), cycle_arcs_(other.cycle_arcs_),
        member_first_(other.member_first_), members_(other.members_),
        group_member_(other.group_member_), group_first_(other.group_first_),
        narrow_(other.narrow_), wide_(other.wide_), weight_(other.weight_),
        stamp_(other.stamp_), clock_(other.clock_),
        done_(other.done_.size(), static_cast<unsigned char>(0)) // no mark of a query
    {
      for (size_t k = 0; k < Stride; ++k)
        {
          length_first_[k] = other.length_first_[k];
          arcs_before_[k] = other.arcs_before_[k];
        }
    }

    /** @brief Take the index of another, in constant time and without
     *         failing.
     *
     *  @param[in,out] other the index to take. It is left without arcs
     *                 and without cycles: it can be asked, assigned and
     *                 destroyed.
     */
    Bounded_Cycle_Index(Bounded_Cycle_Index && other) noexcept
      : arc_numbers_(std::move(other.arc_numbers_)), arcs_(std::move(other.arcs_)),
        lengths_(std::exchange(other.lengths_, uint32_t(0))),
        longest_(std::exchange(other.longest_, size_t(0))),
        cycle_arcs_(std::move(other.cycle_arcs_)),
        member_first_(std::move(other.member_first_)), members_(std::move(other.members_)),
        group_member_(std::move(other.group_member_)),
        group_first_(std::move(other.group_first_)), narrow_(std::move(other.narrow_)),
        wide_(std::move(other.wide_)), weight_(std::move(other.weight_)),
        stamp_(std::move(other.stamp_)), clock_(std::exchange(other.clock_, uint64_t(0))),
        done_(std::move(other.done_))
    {
      for (size_t k = 0; k < Stride; ++k)
        {
          length_first_[k] = std::exchange(other.length_first_[k], size_t(0));
          arcs_before_[k] = std::exchange(other.arcs_before_[k], size_t(0));
        }
    }

    /** @brief Make the index a copy of another.
     *
     *  The copy is made apart and then exchanged with the index: if there
     *  is no memory for it, the index keeps all that it had. For a
     *  moment both are in memory.
     *
     *  @param[in] other the index to copy.
     *  @return the index.
     *  @throw std::bad_alloc if there is no memory; the index does not
     *         change.
     *  @note \f$O(E + C k^2)\f$ time and memory.
     */
    Bounded_Cycle_Index & operator = (const Bounded_Cycle_Index & other)
    {
      if (this != &other)
        Bounded_Cycle_Index(other).swap(*this);
      return *this;
    }

    /** @brief Take the index of another, in constant time besides freeing
     *         what the index had, and without failing.
     *
     *  @param[in,out] other the index to take. It is left without arcs
     *                 and without cycles.
     *  @return the index.
     */
    Bounded_Cycle_Index & operator = (Bounded_Cycle_Index && other) noexcept
    {
      Bounded_Cycle_Index(std::move(other)).swap(*this);
      return *this;
    }

    /// @brief The number of arcs of the graph when the index was built.
    [[nodiscard]] size_t num_arcs() const noexcept { return arcs_.size(); }

    /** @brief The number of an arc of the graph, in O(1) expected.
     *
     *  @param[in] a an arc of the graph of the index.
     *  @return its number, from 0 to `num_arcs() - 1`.
     *  @throw std::domain_error if `a` was not an arc of the graph.
     */
    [[nodiscard]] size_t arc_number(const Arc *a) const
    {
      const uint32_t number = arc_numbers_.find(a);
      ah_domain_error_if(number == Not_Found)
        << "Bounded_Cycle_Index: the arc is not of the indexed graph";
      return number;
    }

    /** @brief The arc with a number.
     *
     *  @param[in] number the number of an arc.
     *  @return the arc.
     *  @throw std::out_of_range if there is no arc with that number.
     */
    [[nodiscard]] Arc * arc(const size_t number) const { return arcs_(checked_arc(number)); }

    /// @brief The number of cycles of the catalog.
    [[nodiscard]] size_t num_cycles() const noexcept { return length_first_[Max_Length + 1]; }

    /** @brief The number of cycles of a length.
     *
     *  @param[in] length a length.
     *  @return how many cycles of the catalog have that length; 0 if the
     *          length is not of the set.
     */
    [[nodiscard]] size_t num_cycles(const size_t length) const noexcept
    {
      return length < 1 or length > Max_Length
               ? 0 : length_first_[length + 1] - length_first_[length];
    }

    /** @brief The number of cycles whose length is at most `max_length`:
     *         the size of the prefix of the catalog that they are.
     *
     *  @param[in] max_length a length.
     *  @return how many cycles of the catalog are not longer.
     */
    [[nodiscard]] size_t num_cycles_up_to(const size_t max_length) const noexcept
    {
      return length_first_[(max_length < Max_Length ? max_length : Max_Length) + 1];
    }

    /// @brief Return true if `length` is one of the lengths of the index.
    [[nodiscard]] bool has_length(const size_t length) const noexcept
    {
      return length >= 1 and length <= Max_Length and (lengths_ >> length & 1u) != 0;
    }

    /// @brief The longest length of the set of the index.
    [[nodiscard]] size_t longest_length() const noexcept { return longest_; }

    /** @brief The length of a cycle.
     *
     *  @param[in] position the position of a cycle in the catalog.
     *  @return the number of its arcs.
     *  @throw std::out_of_range if there is no cycle at that position.
     */
    [[nodiscard]] size_t length(const size_t position) const
    {
      return length_of(checked_position(position));
    }

    /** @brief The arcs of a cycle, by their numbers, in its canonical
     *         rotation: from the arc of least number, in the order of the
     *         cycle.
     *
     *  @param[in] position the position of a cycle in the catalog.
     *  @return the numbers of its arcs: a range of the memory of the
     *          index, valid until the index is destroyed, assigned or
     *          moved from. `swap()` takes that memory to the other index.
     *  @throw std::out_of_range if there is no cycle at that position.
     */
    [[nodiscard]] std::span<const uint32_t> cycle_arcs(const size_t position) const
    {
      const size_t k = length_of(checked_position(position));
      return {&cycle_arcs_(first_arc_of(position, k)), k};
    }

    /** @brief The cycles that go through an arc, by their positions, in
     *         ascending order.
     *
     *  @param[in] arc the number of an arc.
     *  @return the positions, none if the arc is in no cycle of the
     *          catalog: a range of the memory of the index, valid until
     *          the index is destroyed, assigned or moved from. `swap()`
     *          takes that memory to the other index.
     *  @throw std::out_of_range if there is no arc with that number.
     */
    [[nodiscard]] std::span<const uint32_t> cycles_of(const size_t arc) const
    {
      const size_t first = member_first_(checked_arc(arc));
      const size_t n = member_first_(arc + 1) - first;
      return n == 0 ? std::span<const uint32_t>() : std::span<const uint32_t>(&members_(first), n);
    }

    /** @brief Change the weight of an arc, without looking at its cycles.
     *
     *  @param[in] arc the number of an arc.
     *  @param[in] w its weight; \f$+\infty\f$ for an arc that cannot be
     *             used.
     *  @throw std::out_of_range if there is no arc with that number.
     *  @throw std::domain_error if `w` is not a number or is
     *         \f$-\infty\f$.
     */
    void set_weight(const size_t arc, const double w)
    {
      check_weight(w);
      weight_(checked_arc(arc)) = w;
      stamp_(arc) = ++clock_;
    }

    /** @brief The weight of an arc.
     *
     *  @param[in] arc the number of an arc.
     *  @return its weight; \f$+\infty\f$ if it was given none.
     *  @throw std::out_of_range if there is no arc with that number.
     */
    [[nodiscard]] double weight(const size_t arc) const { return weight_(checked_arc(arc)); }

    /** @brief The weight of a cycle: the sum of the weights of its arcs,
     *         in the order of its canonical rotation.
     *
     *  It is the number that `update()` and `for_each_cycle_below()` hand
     *  for the cycle, with the same weights.
     *
     *  @param[in] position the position of a cycle in the catalog.
     *  @return the sum.
     *  @throw std::out_of_range if there is no cycle at that position.
     */
    [[nodiscard]] double weight_sum(const size_t position) const
    {
      const size_t k = length_of(checked_position(position));
      return sum_at(first_arc_of(position, k), k);
    }

    /** @brief Change the weight of an arc and hand the cycles through it
     *         whose weight is below a bound.
     *
     *  `op(position, sum)` is called for each cycle of the arc whose
     *  length is at most `max_length` and whose sum is below `bound`, in
     *  ascending order of position. It must not change the index. If it
     *  throws, the weight is changed and the cycles that were not reached
     *  are not handed.
     *
     *  @param[in] arc the number of an arc.
     *  @param[in] w its weight; \f$+\infty\f$ for an arc that cannot be
     *             used.
     *  @param[in] bound a cycle is handed if its sum is less than it; see
     *             the description of the file for its margin.
     *  @param[in] max_length the longest length of the cycles to look at.
     *  @param[in] op the function that receives each cycle.
     *  @return the number of cycles looked at.
     *  @throw std::out_of_range if there is no arc with that number.
     *  @throw std::domain_error if `w` is not a number or is
     *         \f$-\infty\f$, or if `bound` is not a number.
     *  @note \f$O(c\,k)\f$ besides what `op` takes, for the `c` cycles
     *        of the arc of at most `max_length` arcs, of `k` arcs each:
     *        the weight of each of their arcs is read.
     */
    template <class Op>
      requires std::invocable<Op &, size_t, double>
    size_t update(const size_t arc, const double w, const double bound,
                  const size_t max_length, Op && op)
    {
      check_bound(bound);
      set_weight(arc, w);
      return look_at(arc, bound, longest_up_to(max_length), false, op);
    }

    /** @brief Hand the cycles through any of some arcs whose weight is
     *         below a bound, each cycle once.
     *
     *  It is how to look at the cycles after `set_weight()` has changed
     *  several arcs: no cycle is seen with only a part of the changes.
     *  `op(position, sum)` is called once for each cycle that goes
     *  through any of the arcs, whose length is at most `max_length` and
     *  whose sum is below `bound`: by arc, in the order given, and by
     *  ascending position among those of an arc. It must not change the
     *  index. If it throws, the cycles that were not reached are not
     *  handed, and the index is ready for another query.
     *
     *  @param[in] arcs the numbers of the arcs; a number may be repeated.
     *  @param[in] bound a cycle is handed if its sum is less than it.
     *  @param[in] max_length the longest length of the cycles to look at.
     *  @param[in] op the function that receives each cycle.
     *  @return the number of cycles looked at.
     *  @throw std::out_of_range if there is no arc with one of the
     *         numbers; no cycle is handed then.
     *  @throw std::domain_error if `bound` is not a number.
     *  @note \f$O(n + c\,k)\f$ besides what `op` takes, for `n` arcs
     *        given and the `c` cycles of at most `max_length` arcs, of `k`
     *        arcs each, counted once for each different arc given that
     *        they go through: a cycle through several of those arcs is
     *        read for each of them and handed once. An arc that is given
     *        again costs \f$O(1)\f$.
     */
    template <class Op>
      requires std::invocable<Op &, size_t, double>
    size_t for_each_cycle_below(const Array<size_t> & arcs, const double bound,
                                const size_t max_length, Op && op)
    {
      return look_at_each(arcs.size() == 0 ? nullptr : &arcs(0), arcs.size(), bound,
                          max_length, op);
    }

    /** @brief Hand the cycles through any of the arcs of a list whose
     *         weight is below a bound, each cycle once.
     *
     *  It is `for_each_cycle_below()` for an array of arcs, with the arcs
     *  written in the call: `for_each_cycle_below({a, b}, bound, m, op)`.
     *
     *  @param[in] arcs the numbers of the arcs; a number may be repeated.
     *  @param[in] bound a cycle is handed if its sum is less than it.
     *  @param[in] max_length the longest length of the cycles to look at.
     *  @param[in] op the function that receives each cycle.
     *  @return the number of cycles looked at.
     *  @throw std::out_of_range if there is no arc with one of the
     *         numbers; no cycle is handed then.
     *  @throw std::domain_error if `bound` is not a number.
     *  @note \f$O(n + c\,k)\f$ besides what `op` takes, as with an array
     *        of arcs: `n` arcs given, and `c` cycles of `k` arcs counted
     *        once for each different arc given that they go through.
     */
    template <class Op>
      requires std::invocable<Op &, size_t, double>
    size_t for_each_cycle_below(std::initializer_list<size_t> arcs, const double bound,
                                const size_t max_length, Op && op)
    {
      return look_at_each(arcs.begin(), arcs.size(), bound, max_length, op);
    }

    /** @brief Hand the cycles of the catalog whose weight is below a
     *         bound.
     *
     *  `op(position, sum)` is called for each cycle whose length is at
     *  most `max_length` and whose sum is below `bound`, in ascending
     *  order of position. If it throws, the cycles that were not reached
     *  are not handed.
     *
     *  @param[in] bound a cycle is handed if its sum is less than it.
     *  @param[in] max_length the longest length of the cycles to look at.
     *  @param[in] op the function that receives each cycle.
     *  @return the number of cycles looked at.
     *  @throw std::domain_error if `bound` is not a number.
     *  @note \f$O(C\,k)\f$ besides what `op` takes, for the `C` cycles
     *        of the catalog of at most `max_length` arcs, of `k` arcs
     *        each.
     */
    template <class Op>
      requires std::invocable<Op &, size_t, double>
    size_t for_each_cycle_below(const double bound, const size_t max_length, Op && op) const
    {
      check_bound(bound);
      const size_t longest = longest_up_to(max_length);
      for (size_t k = 1; k <= longest; ++k)
        for (size_t position = length_first_[k]; position < length_first_[k + 1]; ++position)
          if (const double sum = sum_at(first_arc_of(position, k), k); sum < bound)
            op(position, sum);
      return length_first_[longest + 1];
    }

    /** @brief A bound of the rounding error of the sum of the weights of
     *         a cycle.
     *
     *  The sum of `k` weights of magnitude at most `max_abs_weight`, as
     *  the index adds them, is within this of their exact sum, provided
     *  that `k * max_abs_weight` is finite. It says nothing of the error
     *  of each weight, which is of who computes it.
     *
     *  @param[in] k the number of arcs of the cycle.
     *  @param[in] max_abs_weight a bound of the magnitude of each weight.
     *  @return the bound: 0 for a cycle of one arc.
     */
    [[nodiscard]] static constexpr double sum_error_bound(const size_t k,
                                                          const double max_abs_weight) noexcept
    {
      if (k < 2)
        return 0;
      // k - 1 rounded additions of partial sums of magnitude at most
      // k * max_abs_weight: gamma(k - 1) = (k - 1) u / (1 - (k - 1) u),
      // with u = 2^-53, times that magnitude; the last factor covers the
      // rounding of this very product
      const double u = 0x1p-53;
      const auto additions = double(k - 1);
      return additions * u / (1 - additions * u) * double(k) * max_abs_weight * (1 + 0x1p-40);
    }

    /** @brief The number of writes of a weight so far: each
     *         `set_weight()`, `update()` and `touch()` adds one.
     */
    [[nodiscard]] uint64_t clock() const noexcept { return clock_; }

    /** @brief The value of `clock()` when an arc was last written.
     *
     *  @param[in] arc the number of an arc.
     *  @return that value; 0 if the arc was never written.
     *  @throw std::out_of_range if there is no arc with that number.
     */
    [[nodiscard]] uint64_t stamp(const size_t arc) const { return stamp_(checked_arc(arc)); }

    /** @brief Mark an arc as written without changing its weight, for
     *         what the caller keeps of its cycles and depends on more
     *         than the weight.
     *
     *  @param[in] arc the number of an arc.
     *  @throw std::out_of_range if there is no arc with that number.
     */
    void touch(const size_t arc)
    {
      (void) checked_arc(arc);
      stamp_(arc) = ++clock_;
    }

    /** @brief Return true if an arc of a cycle was written after a
     *         moment: what was computed of the cycle then is old.
     *
     *  @param[in] position the position of a cycle in the catalog.
     *  @param[in] computed_at the value of `clock()` at that moment.
     *  @return true if `stamp()` of one of its arcs is greater.
     *  @throw std::out_of_range if there is no cycle at that position.
     */
    [[nodiscard]] bool is_stale(const size_t position, const uint64_t computed_at) const
    {
      for (const uint32_t arc : cycle_arcs(position))
        if (stamp_(arc) > computed_at)
          return true;
      return false;
    }

    /** @brief Exchange two indexes in constant time, without failing: how
     *         to replace an index by another built apart, with another
     *         set of lengths.
     *
     *  The ranges that `cycle_arcs()` and `cycles_of()` gave go with their
     *  memory: those of this index are of `other` afterwards.
     *
     *  @param[in,out] other the other index.
     */
    void swap(Bounded_Cycle_Index & other) noexcept
    {
      arc_numbers_.swap(other.arc_numbers_);
      arcs_.swap(other.arcs_);
      std::swap(lengths_, other.lengths_);
      std::swap(longest_, other.longest_);
      std::swap(length_first_, other.length_first_);
      std::swap(arcs_before_, other.arcs_before_);
      cycle_arcs_.swap(other.cycle_arcs_);
      member_first_.swap(other.member_first_);
      members_.swap(other.members_);
      group_member_.swap(other.group_member_);
      group_first_.swap(other.group_first_);
      narrow_.swap(other.narrow_);
      wide_.swap(other.wide_);
      weight_.swap(other.weight_);
      stamp_.swap(other.stamp_);
      std::swap(clock_, other.clock_);
      done_.swap(other.done_);
    }
  };
} // namespace Aleph

# endif // BOUNDED_CYCLE_INDEX_H
