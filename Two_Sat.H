
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

/** @file Two_Sat.H
 *  @brief 2-SAT solver based on Tarjan's SCC algorithm.
 *
 *  The 2-satisfiability problem (2-SAT) involves determining if a
 *  conjunction of clauses, each with exactly two literals, can be
 *  satisfied by a truth assignment.
 *
 *  This implementation uses an implication graph and Tarjan's strongly
 *  connected components (SCC) algorithm. A formula is satisfiable if
 *  and only if no variable and its negation are in the same SCC.
 *
 *  ## Literal encoding
 *
 *  Variables are 0-indexed from `0` to `n-1`. Each variable `i` has two
 *  literals:
 *  - Positive: `2*i`
 *  - Negative: `2*i + 1`
 *
 *  Signed 1-based indices are also supported: `+k` for x_{k-1} and
 *  `-k` for ~x_{k-1}.
 *
 *  @see Tarjan.H
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef TWO_SAT_H
# define TWO_SAT_H

# include <utility>
# include <limits>

# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_dynArray.H>
# include <tpl_graph.H>
# include <tpl_dynSetTree.H>
# include <Tarjan.H>

namespace Aleph {

/** @brief 2-SAT solver using Tarjan's SCC algorithm.
 *
 *  Determines satisfiability of 2-CNF formulas in linear time.
 *
 *  @tparam GT Directed graph type for the internal implication graph.
 *             Defaults to `List_Digraph`.
 */
template <class GT = List_Digraph<Graph_Node<Empty_Class>,
                                  Graph_Arc<Empty_Class>>>
class Two_Sat
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

private:
  size_t num_vars;
  GT ig;
  Array<Node *> lit_nodes;

public:

  /// @name Literal helpers
  /// @{

  /** @brief Get the positive literal for a variable.
   *  @param var Variable index (0 to n-1).
   *  @return Literal index (even).
   */
  [[nodiscard]] static constexpr size_t
  pos_lit(const size_t var) noexcept { return 2 * var; }

  /** @brief Get the negative literal for a variable.
   *  @param var Variable index (0 to n-1).
   *  @return Literal index (odd).
   */
  [[nodiscard]] static constexpr size_t
  neg_lit(const size_t var) noexcept { return 2 * var + 1; }

  /** @brief Negate a literal.
   *  @param lit Literal index.
   *  @return The opposite literal index.
   */
  [[nodiscard]] static constexpr size_t
  negate_lit(const size_t lit) noexcept { return lit ^ 1; }

  /** @brief Get the variable index of a literal.
   *  @param lit Literal index.
   *  @return Variable index.
   */
  [[nodiscard]] static constexpr size_t
  lit_var(const size_t lit) noexcept { return lit >> 1; }

  /** @brief Check if a literal is positive.
   *  @param lit Literal index.
   *  @return true if positive, false if negative.
   */
  [[nodiscard]] static constexpr bool
  is_pos_lit(const size_t lit) noexcept { return (lit & 1) == 0; }

  /** @brief Convert a signed 1-based variable to a literal index.
   *
   *  Maps `+k` to `pos_lit(k-1)` and `-k` to `neg_lit(k-1)`.
   *
   *  @param v Signed variable index (non-zero).
   *  @return Literal index.
   */
  [[nodiscard]] static constexpr size_t
  signed_to_lit(const long v) noexcept
  {
    if (v > 0)
      return pos_lit(static_cast<size_t>(v - 1));

    // Handle v < 0 safely, avoiding overflow for LONG_MIN
    // -v - 1 is unsafe for LONG_MIN. Use magnitude subtraction.
    const size_t magnitude_minus_one = static_cast<size_t>(-(v + 1));
    return neg_lit(magnitude_minus_one);
  }

  /// @}

  /** @brief Deleted copy constructor to avoid dangling internal Node* pointers. */
  Two_Sat(const Two_Sat &) = delete;

  /** @brief Deleted copy assignment to avoid dangling internal Node* pointers. */
  Two_Sat & operator=(const Two_Sat &) = delete;

  /** @brief Construct a solver for `n` variables.
   *  @param n Number of Boolean variables.
   */
  Two_Sat(const size_t n)
    : num_vars(n), lit_nodes(Array<Node *>::create(2 * n))
  {
    for (size_t i = 0; i < 2 * n; ++i)
      lit_nodes(i) = ig.insert_node();
  }

  /// @return Number of variables.
  [[nodiscard]] size_t size() const noexcept { return num_vars; }

  /// @return Number of nodes in the implication graph (2*n).
  [[nodiscard]] size_t num_nodes() const noexcept { return 2 * num_vars; }

  /// @return Read-only reference to the internal implication graph.
  [[nodiscard]] const GT & implication_graph() const noexcept { return ig; }

  /** @brief Add a clause `(a OR b)`.
   *  @param a First literal index.
   *  @param b Second literal index.
   *  @throw std::domain_error if literals are out of range.
   */
  void add_clause(size_t a, size_t b)
  {
    ah_domain_error_if(lit_var(a) >= num_vars)
      << "Two_Sat::add_clause: literal " << a
      << " refers to variable " << lit_var(a)
      << " but only " << num_vars << " variables exist";
    ah_domain_error_if(lit_var(b) >= num_vars)
      << "Two_Sat::add_clause: literal " << b
      << " refers to variable " << lit_var(b)
      << " but only " << num_vars << " variables exist";

    ig.insert_arc(lit_nodes(negate_lit(a)), lit_nodes(b));
    ig.insert_arc(lit_nodes(negate_lit(b)), lit_nodes(a));
  }

  /** @brief Add a clause using signed 1-based variables.
   *  @param a First signed variable.
   *  @param b Second signed variable.
   */
  void add_clause_signed(const long a, const long b)
  {
    ah_domain_error_if(a == 0 or b == 0)
      << "Two_Sat::add_clause_signed: variable index 0 is invalid";
    ah_domain_error_if(a == std::numeric_limits<long>::min() or
                       b == std::numeric_limits<long>::min())
      << "Two_Sat::add_clause_signed: variable index LONG_MIN is invalid";
    add_clause(signed_to_lit(a), signed_to_lit(b));
  }

  /** @brief Add an implication `a -> b`.
   *  @param a Antecedent literal.
   *  @param b Consequent literal.
   */
  void add_implication(const size_t a, const size_t b)
  {
    add_clause(negate_lit(a), b);
  }

  /** @brief Force literal `a` to be true.
   *  @param a Literal index.
   */
  void set_true(const size_t a)
  {
    add_clause(a, a);
  }

  /** @brief Force literal `a` to be false.
   *  @param a Literal index.
   */
  void set_false(const size_t a)
  {
    set_true(negate_lit(a));
  }

  /** @brief Add XOR constraint: `exactly_one(a, b)`.
   *  @param a First literal.
   *  @param b Second literal.
   */
  void add_xor(const size_t a, const size_t b)
  {
    add_clause(a, b);
    add_clause(negate_lit(a), negate_lit(b));
  }

  /** @brief Add equivalence constraint: `a <-> b`.
   *  @param a First literal.
   *  @param b Second literal.
   */
  void add_equiv(const size_t a, const size_t b)
  {
    add_implication(a, b);
    add_implication(b, a);
  }

  /** @brief Add at-most-one constraint over a set of literals.
   *
   *  Ensures that at most one of the given literals is true. This
   *  adds O(k^2) clauses where k is the number of distinct literals.
   *
   *  Singletons are validated but produce no clauses. Duplicate
   *  literals are normalized. Pairs of identical literals are skipped
   *  to avoid adding redundant forcing clauses.
   *
   *  @param lits Array of literal indices.
   *  @throw std::domain_error if any literal is out of range.
   */
  void add_at_most_one(const Array<size_t> & lits)
  {
    DynSetTree<size_t> unique_lits;
    for (size_t i = 0; i < lits.size(); ++i)
      {
        const size_t lit = lits(i);
        ah_domain_error_if(lit_var(lit) >= num_vars)
          << "Two_Sat::add_at_most_one: literal " << lit
          << " refers to variable " << lit_var(lit)
          << " but only " << num_vars << " variables exist";
        unique_lits.insert(lit);
      }

    if (unique_lits.size() < 2)
      return;

    DynArray<size_t> u_dyn;
    unique_lits.for_each([&u_dyn](const size_t & lit) { u_dyn.append(lit); });
    const Array<size_t> u = u_dyn.to_array();
    for (size_t i = 0; i < u.size(); ++i)
      for (size_t j = i + 1; j < u.size(); ++j)
        add_clause(negate_lit(u(i)), negate_lit(u(j)));
  }

  /** @brief Check if the formula is satisfiable.
   *  @return true if satisfiable.
   */
  [[nodiscard]] bool is_satisfiable()
  {
    return solve().first;
  }

  /** @brief Solve the 2-SAT instance.
   *  @return A pair {satisfiable, assignment}.
   */
  [[nodiscard]] std::pair<bool, Array<bool>> solve()
  {
    Tarjan_Connected_Components<GT> tarjan;
    auto sccs = tarjan.connected_components(ig);

    long scc_id = 0;
    sccs.for_each([&scc_id](auto & scc) {
      scc.for_each([&scc_id](Node * node) {
        NODE_COUNTER(node) = scc_id;
      });
      ++scc_id;
    });

    Array<bool> assignment = Array<bool>::create(num_vars);

    for (size_t i = 0; i < num_vars; ++i)
      {
        const long id_pos = NODE_COUNTER(lit_nodes(pos_lit(i)));
        const long id_neg = NODE_COUNTER(lit_nodes(neg_lit(i)));

        if (id_pos == id_neg)
          return {false, Array<bool>()};

        assignment(i) = id_pos < id_neg;
      }

    return {true, std::move(assignment)};
  }
};


} // end namespace Aleph

# endif /* TWO_SAT_H */