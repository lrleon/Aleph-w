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

/** @file tpl_ca_multi_field_rule.H
 *  @brief Phase 14 multi-field local rules for `Aleph::CA`.
 *
 *  A *multi-field rule* is the natural transition function for a
 *  `Multi_Field_Lattice`: it consumes the per-field current values
 *  and per-field neighbour views of a single cell, and returns the
 *  tuple of next-step values for that cell.
 *
 *  Two signature flavours are supported, both detected via
 *  `apply_multi_field_rule`:
 *
 *    - Pure: `r(current_tuple, neighbours_tuple)` →
 *            `std::tuple<Fields...>`.
 *    - Contextual: `r(current_tuple, neighbours_tuple, ctx)` →
 *            `std::tuple<Fields...>` (`ctx` is `Cell_Context<Rank>`).
 *
 *  `current_tuple` is `std::tuple<Field_0, Field_1, ...>` and
 *  `neighbours_tuple` is `std::tuple<Neighbor_View<Field_0>,
 *  Neighbor_View<Field_1>, ...>`. The two tuples carry one element
 *  per field, in the order the fields were declared in the lattice.
 *
 *  Adapters bridge this contract to the existing mono-field rules:
 *
 *    - `Field_Slice_Rule<I, Inner>`: applies a regular `RuleLike` to
 *      field `I`, propagating the other fields untouched.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_MULTI_FIELD_RULE_H
#define TPL_CA_MULTI_FIELD_RULE_H

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_mf_rule_detail {

template <typename T>
struct as_neighbor_view
{
  using type = Neighbor_View<T>;
};

template <typename Tuple>
struct neighbor_view_tuple;

template <typename... Fields>
struct neighbor_view_tuple<std::tuple<Fields...>>
{
  using type = std::tuple<Neighbor_View<Fields>...>;
};

}  // namespace ca_mf_rule_detail

/** @brief `std::tuple<Neighbor_View<F>...>` for the field tuple
 *         `std::tuple<F...>`. */
template <typename FieldsTuple>
using Field_Neighbor_Views_Tuple =
  typename ca_mf_rule_detail::neighbor_view_tuple<FieldsTuple>::type;

/** @brief Concept: a pure multi-field rule.
 *
 *  Invocable as `r(current, neighbours)` returning the next-state
 *  tuple, with no per-cell context.
 *
 *  @tparam R candidate rule type.
 *  @tparam FieldsTuple `std::tuple<Fields...>` of the lattice fields.
 */
template <typename R, typename FieldsTuple>
concept BasicMultiFieldRule
  = requires(const R &r, const FieldsTuple &cur, const Field_Neighbor_Views_Tuple<FieldsTuple> &nb) {
      { r(cur, nb) } -> std::convertible_to<FieldsTuple>;
    };

/** @brief Concept: a contextual multi-field rule.
 *
 *  Invocable as `r(current, neighbours, ctx)`, where `ctx` is
 *  `Cell_Context<Rank>`.
 *
 *  @tparam R candidate rule type.
 *  @tparam FieldsTuple `std::tuple<Fields...>` of the lattice fields.
 *  @tparam Rank lattice rank.
 */
template <typename R, typename FieldsTuple, std::size_t Rank>
concept ContextualMultiFieldRule = requires(const R &r,
                                            const FieldsTuple &cur,
                                            const Field_Neighbor_Views_Tuple<FieldsTuple> &nb,
                                            const Cell_Context<Rank> &ctx) {
  { r(cur, nb, ctx) } -> std::convertible_to<FieldsTuple>;
};

/** @brief Concept: any supported multi-field rule signature. */
template <typename R, typename FieldsTuple, std::size_t Rank>
concept MultiFieldRuleLike
  = BasicMultiFieldRule<R, FieldsTuple> or ContextualMultiFieldRule<R, FieldsTuple, Rank>;

/** @brief Invoke a multi-field rule with optional context forwarding.
 *
 *  Mirrors the role of `apply_rule` in the mono-field path: the
 *  contextual signature is preferred when supported; otherwise the
 *  pure signature is used.
 *
 *  @tparam Rule rule type.
 *  @tparam FieldsTuple field tuple type.
 *  @tparam Rank lattice rank.
 *  @param[in] r rule instance.
 *  @param[in] cur current per-field values tuple.
 *  @param[in] nb per-field neighbour views tuple.
 *  @param[in] ctx per-cell context (forwarded to the rule when accepted).
 *  @return new per-field values tuple.
 */
template <typename Rule, typename FieldsTuple, std::size_t Rank>
[[nodiscard]] inline FieldsTuple apply_multi_field_rule(const Rule &r,
                                                        const FieldsTuple &cur,
                                                        const Field_Neighbor_Views_Tuple<FieldsTuple> &nb,
                                                        const Cell_Context<Rank> &ctx)
{
  if constexpr (requires { r(cur, nb, ctx); })
    return r(cur, nb, ctx);
  else
    return r(cur, nb);
}

// =======================================================================
// Field_Slice_Rule
// =======================================================================

/** @brief Adaptor: apply a mono-field rule to field `I`, leave the
 *         other fields unchanged.
 *
 *  Lets the user reuse any existing `RuleLike` (Game of Life,
 *  outer-totalistic, continuous, lookup, ...) on a single channel of
 *  a multi-field lattice without touching the other channels. The
 *  inner rule sees only field `I`'s current value and field `I`'s
 *  neighbours.
 *
 *  @tparam I index of the targeted field.
 *  @tparam Inner mono-field rule type.
 */
template <std::size_t I, typename Inner>
class Field_Slice_Rule
{
  Inner inner_;

public:
  /** @brief Wrap a mono-field rule.
   *  @param[in] inner mono-field rule (consumed by move).
   */
  constexpr explicit Field_Slice_Rule(Inner inner) : inner_(std::move(inner)) {}

  /** @brief Pure-form invocation.
   *
   *  @tparam FieldsTuple lattice's `std::tuple<Fields...>`.
   *  @param[in] cur current per-field values.
   *  @param[in] nb per-field neighbour views.
   *  @return tuple where field `I` is replaced by `inner(cur, nb)`
   *          and the other fields keep their current values.
   */
  template <typename FieldsTuple>
  [[nodiscard]] FieldsTuple operator()(const FieldsTuple &cur,
                                       const Field_Neighbor_Views_Tuple<FieldsTuple> &nb) const
  {
    static_assert(I < std::tuple_size_v<FieldsTuple>, "Field_Slice_Rule: I out of range");
    FieldsTuple out = cur;
    std::get<I>(out) = inner_(std::get<I>(cur), std::get<I>(nb));
    return out;
  }

  /** @brief Contextual-form invocation.
   *
   *  Forwards the context to `inner` when it accepts it.
   */
  template <typename FieldsTuple, std::size_t Rank>
  [[nodiscard]] FieldsTuple operator()(const FieldsTuple &cur,
                                       const Field_Neighbor_Views_Tuple<FieldsTuple> &nb,
                                       const Cell_Context<Rank> &ctx) const
  {
    static_assert(I < std::tuple_size_v<FieldsTuple>, "Field_Slice_Rule: I out of range");
    FieldsTuple out = cur;
    if constexpr (requires { inner_(std::get<I>(cur), std::get<I>(nb), ctx); })
      std::get<I>(out) = inner_(std::get<I>(cur), std::get<I>(nb), ctx);
    else
      std::get<I>(out) = inner_(std::get<I>(cur), std::get<I>(nb));
    return out;
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_MULTI_FIELD_RULE_H