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

/** @file tpl_ca_continuous_rules.H
 *  @brief Continuous and memory-bearing CA rules (Phase 9).
 *
 *  This header extends the rule catalogue with:
 *
 *    - `Continuous_Rule<F, Kernel>` for scalar `float`/`double` states
 *      updated from a real convolution kernel.
 *    - `Reaction_Diffusion_Cell<Real>` plus Gray-Scott and
 *      FitzHugh-Nagumo reaction-diffusion rules.
 *    - `History_Cell<State, Depth>` and `History_Rule<Depth, F>` for
 *      per-cell ring-buffer memory.
 *
 *  The reaction-diffusion rules use explicit Euler integration over a
 *  square lattice. Their constructors enforce conservative `dt` ranges,
 *  and each transition checks for non-finite or out-of-contract values
 *  with `ah_runtime_error_if`, so numerical blow-up fails loudly instead
 *  of silently contaminating later frames.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_CONTINUOUS_RULES_H
#define TPL_CA_CONTINUOUS_RULES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-kernels.H>
#include <ca-traits.H>

namespace Aleph {
namespace CA {

namespace ca_continuous_detail {

template <typename Real>
inline void require_finite(const Real value, const char *label)
{
  ah_runtime_error_if(not std::isfinite(static_cast<double>(value))) << label << " is not finite";
}

template <typename Real>
inline void require_domain(const Real value, const Real lo, const Real hi, const char *label)
{
  require_finite(value, label);
  ah_domain_error_if(value < lo or value > hi)
    << label << "=" << value << " outside [" << lo << ", " << hi << "]";
}

template <typename Real>
inline void require_positive_dt(const Real dt, const Real max_dt, const char *label)
{
  require_finite(dt, label);
  ah_domain_error_if(dt <= Real{}) << label << " must be positive";
  ah_domain_error_if(dt > max_dt) << label << "=" << dt << " exceeds the documented stable maximum "
                                  << max_dt;
}

template <typename Real>
[[nodiscard]] inline Real clamp_unit_after_check(const Real value, const char *label)
{
  require_finite(value, label);
  constexpr Real eps = Real{1024} * std::numeric_limits<Real>::epsilon();
  ah_runtime_error_if(value < -eps or value > Real{1} + eps)
    << label << "=" << value << " outside the stable [0, 1] interval";
  return std::clamp(value, Real{}, Real{1});
}

template <typename Real>
[[nodiscard]] inline Real checked_bounded(const Real value, const Real max_abs, const char *label)
{
  require_finite(value, label);
  ah_runtime_error_if(value < -max_abs or value > max_abs)
    << label << "=" << value << " outside [-" << max_abs << ", " << max_abs << "]";
  return value;
}

}  // namespace ca_continuous_detail

// -----------------------------------------------------------------------
// Continuous_Rule
// -----------------------------------------------------------------------

/** @brief Scalar continuous rule driven by a real convolution kernel.
 *
 *  The wrapped functor may expose either:
 *
 *    - `F(State current, State convolution, Real dt) -> State`
 *    - `F(State current, State convolution) -> State`
 *
 *  where `convolution` is `kernel.apply(current, neighbours)` and
 *  neighbours must be gathered with `Moore<2, Kernel::radius_v>`.
 *  Results are checked for finiteness and against the optional output
 *  range `[min_value, max_value]`.
 *
 *  @tparam F callable rule body.
 *  @tparam Kernel kernel type, normally `Kernel2D<Real, 3, 3>` or
 *          `Kernel2D<Real, 5, 5>`.
 */
template <typename F, typename Kernel>
class Continuous_Rule
{
public:
  /// Numeric type used by the kernel.
  using value_type = typename Kernel::value_type;
  /// Kernel type.
  using kernel_type = Kernel;

private:
  F f_;
  Kernel kernel_;
  value_type dt_;
  value_type min_value_;
  value_type max_value_;

public:
  /** @brief Build a scalar continuous rule.
   *
   *  @param[in] func transition functor.
   *  @param[in] kernel convolution kernel.
   *  @param[in] dt explicit Euler time step; must be finite and positive.
   *  @param[in] min_value minimum accepted next state.
   *  @param[in] max_value maximum accepted next state.
   *  @throws std::domain_error if `dt` or the output interval is invalid.
   */
  Continuous_Rule(F func,
                  Kernel kernel,
                  value_type dt = value_type{1},
                  value_type min_value = -std::numeric_limits<value_type>::infinity(),
                  value_type max_value = std::numeric_limits<value_type>::infinity())
    : f_(std::move(func)), kernel_(std::move(kernel)), dt_(dt), min_value_(min_value),
      max_value_(max_value)
  {
    ca_continuous_detail::require_finite(dt_, "Continuous_Rule::dt");
    ah_domain_error_if(dt_ <= value_type{}) << "Continuous_Rule: dt must be positive";
    ah_domain_error_if(min_value_ > max_value_) << "Continuous_Rule: min_value exceeds max_value";
  }

  /// @return the stored convolution kernel.
  [[nodiscard]] const Kernel &kernel() const noexcept
  {
    return kernel_;
  }

  /// @return the explicit Euler time step.
  [[nodiscard]] value_type dt() const noexcept
  {
    return dt_;
  }

  /// @return the minimum accepted output value.
  [[nodiscard]] value_type min_value() const noexcept
  {
    return min_value_;
  }

  /// @return the maximum accepted output value.
  [[nodiscard]] value_type max_value() const noexcept
  {
    return max_value_;
  }

  /** @brief Compute the next scalar state.
   *
   *  @tparam State `float` or `double`.
   *  @param[in] current current centre-cell value.
   *  @param[in] neighbours values in `Moore<2, Kernel::radius_v>` order.
   *  @return next scalar state.
   *  @throws std::length_error if neighbour count does not match the kernel.
   *  @throws std::runtime_error if the functor returns a non-finite or
   *          out-of-range value.
   */
  template <typename State>
  [[nodiscard]] State operator()(const State &current, Neighbor_View<State> neighbours) const
  {
    static_assert(std::is_floating_point_v<State>,
                  "Continuous_Rule requires float or double states");

    const State conv = static_cast<State>(kernel_.apply(current, neighbours));
    State next{};
    if constexpr (std::is_invocable_r_v<State, const F &, State, State, value_type>)
      next = f_(current, conv, dt_);
    else if constexpr (std::is_invocable_r_v<State, const F &, State, State>)
      next = f_(current, conv);
    else
      static_assert(std::is_invocable_r_v<State, const F &, State, State>,
                    "Continuous_Rule functor must accept (State, convolution[, dt])");

    ca_continuous_detail::require_finite(next, "Continuous_Rule::next");
    ah_runtime_error_if(next < static_cast<State>(min_value_) or next > static_cast<State>(max_value_))
      << "Continuous_Rule::next=" << next << " outside [" << min_value_ << ", " << max_value_ << "]";
    return next;
  }
};

/** @brief Helper factory for `Continuous_Rule`.
 *
 *  @tparam F callable rule body.
 *  @tparam Kernel kernel type.
 *  @param[in] f transition functor.
 *  @param[in] k convolution kernel.
 *  @param[in] dt explicit Euler time step.
 *  @param[in] min_value minimum accepted next state.
 *  @param[in] max_value maximum accepted next state.
 *  @return configured `Continuous_Rule`.
 *  @throws Any exception thrown by the constructor.
 */
template <typename F, typename Kernel>
[[nodiscard]] Continuous_Rule<F, Kernel> make_continuous_rule(
  F f,
  Kernel k,
  typename Kernel::value_type dt = typename Kernel::value_type{1},
  typename Kernel::value_type min_value
  = -std::numeric_limits<typename Kernel::value_type>::infinity(),
  typename Kernel::value_type max_value
  = std::numeric_limits<typename Kernel::value_type>::infinity())
{
  return Continuous_Rule<F, Kernel>(std::move(f), std::move(k), dt, min_value, max_value);
}

// -----------------------------------------------------------------------
// Reaction-diffusion state.
// -----------------------------------------------------------------------

/** @brief Two-field state for reaction-diffusion cellular automata.
 *
 *  Gray-Scott and FitzHugh-Nagumo both evolve an activator-like `u`
 *  field and an inhibitor-like `v` field. The type is intentionally
 *  small, regular and equality-comparable so it satisfies `CellState`
 *  and can be stored in `Dense_Cell_Storage`.
 *
 *  @tparam Real `float` or `double`.
 */
template <typename Real = double>
struct Reaction_Diffusion_Cell
{
  static_assert(std::is_floating_point_v<Real>, "Reaction_Diffusion_Cell requires float or double");

  /// First concentration / activator field.
  Real u = Real{};
  /// Second concentration / inhibitor field.
  Real v = Real{};

  /** @brief Compare two cells exactly.
   *
   *  @param[in] rhs cell to compare with.
   *  @return true when both fields match exactly.
   *  @throws This function does not throw.
   */
  [[nodiscard]] constexpr bool operator==(const Reaction_Diffusion_Cell &rhs) const noexcept
  {
    return u == rhs.u and v == rhs.v;
  }
};

// -----------------------------------------------------------------------
// Gray-Scott reaction-diffusion.
// -----------------------------------------------------------------------

/** @brief Gray-Scott reaction-diffusion rule.
 *
 *  Explicit Euler update:
 *
 *  ```
 *  du/dt = Du * Lap(u) - u*v*v + F*(1 - u)
 *  dv/dt = Dv * Lap(v) + u*v*v - (F + K)*v
 *  ```
 *
 *  The default 5-point Laplacian expects a `Moore<2, 1>` neighbourhood;
 *  diagonal neighbour weights are zero. Concentrations are contracted to
 *  `[0, 1]` for documented stable parameters. The constructor enforces
 *  `0 < dt <= 1.0`, which covers the standard Gray-Scott presets with
 *  `dx = 1` used by CA examples.
 *
 *  @tparam Real floating-point field type.
 *  @tparam Kernel Laplacian kernel type.
 */
template <typename Real = double, typename Kernel = Kernel2D<Real, 3, 3>>
class Gray_Scott_Rule
{
public:
  /// Cell state consumed and produced by the rule.
  using state_type = Reaction_Diffusion_Cell<Real>;
  /// Laplacian kernel type.
  using kernel_type = Kernel;

private:
  Real feed_;
  Real kill_;
  Real du_;
  Real dv_;
  Real dt_;
  Kernel lap_;

  [[nodiscard]] Real laplace_u(Neighbor_View<state_type> neighbours, const state_type &current) const
  {
    Real acc = current.u * lap_.center();
    for (std::size_t k = 0; k < neighbours.size(); ++k)
      acc += neighbours[k].u * lap_.neighbour_weight(k);
    return acc;
  }

  [[nodiscard]] Real laplace_v(Neighbor_View<state_type> neighbours, const state_type &current) const
  {
    Real acc = current.v * lap_.center();
    for (std::size_t k = 0; k < neighbours.size(); ++k)
      acc += neighbours[k].v * lap_.neighbour_weight(k);
    return acc;
  }

public:
  /** @brief Build a Gray-Scott rule.
   *
   *  @param[in] feed feed rate `F`, usually in `[0, 0.1]`.
   *  @param[in] kill kill rate `K`, usually in `[0, 0.1]`.
   *  @param[in] du diffusion coefficient for `u`.
   *  @param[in] dv diffusion coefficient for `v`.
   *  @param[in] dt explicit Euler time step; stable contract:
   *             `0 < dt <= 1.0`.
   *  @param[in] laplacian convolution kernel; defaults to 5-point
   *             Laplacian.
   *  @throws std::domain_error if any parameter is outside its documented
   *          range.
   */
  Gray_Scott_Rule(Real feed,
                  Real kill,
                  Real du,
                  Real dv,
                  Real dt = Real{1},
                  Kernel laplacian = laplacian_5p_kernel<Real>())
    : feed_(feed), kill_(kill), du_(du), dv_(dv), dt_(dt), lap_(std::move(laplacian))
  {
    ca_continuous_detail::require_domain(feed_, Real{}, Real{1}, "Gray_Scott_Rule::feed");
    ca_continuous_detail::require_domain(kill_, Real{}, Real{1}, "Gray_Scott_Rule::kill");
    ca_continuous_detail::require_domain(du_, Real{}, std::numeric_limits<Real>::max(),
                                         "Gray_Scott_Rule::Du");
    ca_continuous_detail::require_domain(dv_, Real{}, std::numeric_limits<Real>::max(),
                                         "Gray_Scott_Rule::Dv");
    ca_continuous_detail::require_positive_dt(dt_, Real{1}, "Gray_Scott_Rule::dt");
  }

  /// @return feed rate `F`.
  [[nodiscard]] Real feed() const noexcept
  {
    return feed_;
  }

  /// @return kill rate `K`.
  [[nodiscard]] Real kill() const noexcept
  {
    return kill_;
  }

  /// @return `u` diffusion coefficient.
  [[nodiscard]] Real Du() const noexcept
  {
    return du_;
  }

  /// @return `v` diffusion coefficient.
  [[nodiscard]] Real Dv() const noexcept
  {
    return dv_;
  }

  /// @return explicit Euler time step.
  [[nodiscard]] Real dt() const noexcept
  {
    return dt_;
  }

  /// @return Laplacian kernel.
  [[nodiscard]] const Kernel &laplacian() const noexcept
  {
    return lap_;
  }

  /** @brief Compute the next Gray-Scott state.
   *
   *  @param[in] current current `(u, v)` concentration.
   *  @param[in] neighbours neighbour cells in `Moore<2, Kernel::radius_v>`
   *             order.
   *  @return next concentration pair.
   *  @throws std::length_error if neighbour count does not match the kernel.
   *  @throws std::runtime_error if the update leaves the stable `[0, 1]`
   *          concentration interval beyond roundoff tolerance.
   */
  [[nodiscard]] state_type operator()(const state_type &current,
                                      Neighbor_View<state_type> neighbours) const
  {
    ah_length_error_if(neighbours.size() != Kernel::neighbour_count_v)
      << "Gray_Scott_Rule: expected " << Kernel::neighbour_count_v << " neighbours, got "
      << neighbours.size();

    const Real lu = laplace_u(neighbours, current);
    const Real lv = laplace_v(neighbours, current);
    const Real uvv = current.u * current.v * current.v;

    const Real next_u = current.u + dt_ * (du_ * lu - uvv + feed_ * (Real{1} - current.u));
    const Real next_v = current.v + dt_ * (dv_ * lv + uvv - (feed_ + kill_) * current.v);

    return {ca_continuous_detail::clamp_unit_after_check(next_u, "Gray_Scott_Rule::u"),
            ca_continuous_detail::clamp_unit_after_check(next_v, "Gray_Scott_Rule::v")};
  }
};

// -----------------------------------------------------------------------
// FitzHugh-Nagumo reaction-diffusion.
// -----------------------------------------------------------------------

/** @brief FitzHugh-Nagumo excitable-media rule.
 *
 *  Explicit Euler update:
 *
 *  ```
 *  du/dt = Du * Lap(u) + u - u^3 / 3 - v + stimulus
 *  dv/dt = Dv * Lap(v) + epsilon * (u + a - b*v)
 *  ```
 *
 *  The constructor enforces `0 < dt <= 0.25`. This conservative bound
 *  keeps the explicit update stable for the usual tutorial parameters
 *  (`a=0.7`, `b=0.8`, `epsilon=0.08`, `dt=0.01`) and catches accidental
 *  blow-ups early.
 *
 *  @tparam Real floating-point field type.
 *  @tparam Kernel Laplacian kernel type.
 */
template <typename Real = double, typename Kernel = Kernel2D<Real, 3, 3>>
class FitzHugh_Nagumo_Rule
{
public:
  /// Cell state consumed and produced by the rule.
  using state_type = Reaction_Diffusion_Cell<Real>;
  /// Laplacian kernel type.
  using kernel_type = Kernel;

private:
  Real a_;
  Real b_;
  Real epsilon_;
  Real du_;
  Real dv_;
  Real dt_;
  Real stimulus_;
  Kernel lap_;

  [[nodiscard]] Real laplace_u(Neighbor_View<state_type> neighbours, const state_type &current) const
  {
    Real acc = current.u * lap_.center();
    for (std::size_t k = 0; k < neighbours.size(); ++k)
      acc += neighbours[k].u * lap_.neighbour_weight(k);
    return acc;
  }

  [[nodiscard]] Real laplace_v(Neighbor_View<state_type> neighbours, const state_type &current) const
  {
    Real acc = current.v * lap_.center();
    for (std::size_t k = 0; k < neighbours.size(); ++k)
      acc += neighbours[k].v * lap_.neighbour_weight(k);
    return acc;
  }

public:
  /** @brief Build a FitzHugh-Nagumo rule.
   *
   *  @param[in] a recovery nullcline offset.
   *  @param[in] b recovery coupling coefficient; must be positive.
   *  @param[in] epsilon recovery time-scale parameter; must be positive.
   *  @param[in] du diffusion coefficient for `u`.
   *  @param[in] dv diffusion coefficient for `v`.
   *  @param[in] dt explicit Euler time step; stable contract:
   *             `0 < dt <= 0.25`.
   *  @param[in] stimulus constant external stimulus added to `u`.
   *  @param[in] laplacian convolution kernel; defaults to 5-point
   *             Laplacian.
   *  @throws std::domain_error if any parameter is outside its documented
   *          range.
   */
  FitzHugh_Nagumo_Rule(Real a,
                       Real b,
                       Real epsilon,
                       Real du,
                       Real dv,
                       Real dt = Real{0.01},
                       Real stimulus = Real{},
                       Kernel laplacian = laplacian_5p_kernel<Real>())
    : a_(a), b_(b), epsilon_(epsilon), du_(du), dv_(dv), dt_(dt), stimulus_(stimulus),
      lap_(std::move(laplacian))
  {
    ca_continuous_detail::require_finite(a_, "FitzHugh_Nagumo_Rule::a");
    ca_continuous_detail::require_domain(b_, std::numeric_limits<Real>::epsilon(),
                                         std::numeric_limits<Real>::max(),
                                         "FitzHugh_Nagumo_Rule::b");
    ca_continuous_detail::require_domain(epsilon_, std::numeric_limits<Real>::epsilon(),
                                         std::numeric_limits<Real>::max(),
                                         "FitzHugh_Nagumo_Rule::epsilon");
    ca_continuous_detail::require_domain(du_, Real{}, std::numeric_limits<Real>::max(),
                                         "FitzHugh_Nagumo_Rule::Du");
    ca_continuous_detail::require_domain(dv_, Real{}, std::numeric_limits<Real>::max(),
                                         "FitzHugh_Nagumo_Rule::Dv");
    ca_continuous_detail::require_finite(stimulus_, "FitzHugh_Nagumo_Rule::stimulus");
    ca_continuous_detail::require_positive_dt(dt_, Real{0.25}, "FitzHugh_Nagumo_Rule::dt");
  }

  /// @return parameter `a`.
  [[nodiscard]] Real a() const noexcept
  {
    return a_;
  }

  /// @return parameter `b`.
  [[nodiscard]] Real b() const noexcept
  {
    return b_;
  }

  /// @return recovery time-scale parameter.
  [[nodiscard]] Real epsilon() const noexcept
  {
    return epsilon_;
  }

  /// @return `u` diffusion coefficient.
  [[nodiscard]] Real Du() const noexcept
  {
    return du_;
  }

  /// @return `v` diffusion coefficient.
  [[nodiscard]] Real Dv() const noexcept
  {
    return dv_;
  }

  /// @return explicit Euler time step.
  [[nodiscard]] Real dt() const noexcept
  {
    return dt_;
  }

  /// @return constant external stimulus.
  [[nodiscard]] Real stimulus() const noexcept
  {
    return stimulus_;
  }

  /// @return Laplacian kernel.
  [[nodiscard]] const Kernel &laplacian() const noexcept
  {
    return lap_;
  }

  /** @brief Compute the next FitzHugh-Nagumo state.
   *
   *  @param[in] current current `(u, v)` fields.
   *  @param[in] neighbours neighbour cells in `Moore<2, Kernel::radius_v>`
   *             order.
   *  @return next field pair.
   *  @throws std::length_error if neighbour count does not match the kernel.
   *  @throws std::runtime_error if either field is non-finite or leaves
   *          `[-100, 100]`, which signals numerical blow-up.
   */
  [[nodiscard]] state_type operator()(const state_type &current,
                                      Neighbor_View<state_type> neighbours) const
  {
    ah_length_error_if(neighbours.size() != Kernel::neighbour_count_v)
      << "FitzHugh_Nagumo_Rule: expected " << Kernel::neighbour_count_v << " neighbours, got "
      << neighbours.size();

    const Real lu = laplace_u(neighbours, current);
    const Real lv = laplace_v(neighbours, current);
    const Real u3 = current.u * current.u * current.u;

    const Real next_u
      = current.u + dt_ * (du_ * lu + current.u - u3 / Real{3} - current.v + stimulus_);
    const Real next_v = current.v + dt_ * (dv_ * lv + epsilon_ * (current.u + a_ - b_ * current.v));

    return {ca_continuous_detail::checked_bounded(next_u, Real{100}, "FitzHugh_Nagumo_Rule::u"),
            ca_continuous_detail::checked_bounded(next_v, Real{100}, "FitzHugh_Nagumo_Rule::v")};
  }
};

// -----------------------------------------------------------------------
// Per-cell history ring buffer.
// -----------------------------------------------------------------------

/** @brief Cell state carrying a fixed-depth ring buffer of past values.
 *
 *  `current()` returns the value at the active ring head. `previous(1)`
 *  returns the value that was current before the last `push()`, and so
 *  on up to `previous(Depth - 1)`.
 *
 *  @tparam State value stored in each history slot.
 *  @tparam Depth number of slots retained per cell; must be positive.
 */
template <typename State, std::size_t Depth>
class History_Cell
{
  static_assert(Depth >= 1, "History_Cell requires Depth >= 1");

public:
  /// Base value type stored in the ring buffer.
  using value_type = State;
  /// Number of retained states.
  static constexpr std::size_t depth_v = Depth;

private:
  std::array<State, Depth> values_{};
  std::size_t head_ = 0;

public:
  /** @brief Construct a zero/default history.
   *
   *  @throws This function does not throw if `State` construction does
   *          not throw.
   */
  History_Cell() = default;

  /** @brief Construct a history with every slot initialized to `value`.
   *
   *  @param[in] value value copied into every slot.
   *  @throws Any exception thrown by `State` copy assignment.
   */
  explicit History_Cell(const State &value)
  {
    values_.fill(value);
  }

  /** @brief Construct a history from raw ring contents.
   *
   *  @param[in] values ring storage.
   *  @param[in] head index of the current value.
   *  @throws std::out_of_range if `head >= Depth`.
   */
  History_Cell(const std::array<State, Depth> &values, const std::size_t head = 0)
    : values_(values), head_(head)
  {
    ah_out_of_range_error_if(head_ >= Depth)
      << "History_Cell: head " << head_ << " outside [0, " << Depth << ")";
  }

  /** @brief Return the current value.
   *
   *  @return value at the ring head.
   *  @throws This function does not throw if `State` copy does not throw.
   */
  [[nodiscard]] State current() const
  {
    return values_[head_];
  }

  /** @brief Return a previous value by age.
   *
   *  `age == 0` is the current value. `age == 1` is the value before
   *  the most recent `push()`.
   *
   *  @param[in] age age in `[0, Depth)`.
   *  @return selected historical value.
   *  @throws std::out_of_range if `age >= Depth`.
   */
  [[nodiscard]] State previous(const std::size_t age) const
  {
    ah_out_of_range_error_if(age >= Depth)
      << "History_Cell::previous: age " << age << " outside [0, " << Depth << ")";
    const std::size_t idx = (head_ + Depth - age) % Depth;
    return values_[idx];
  }

  /** @brief Push a new current value into the ring.
   *
   *  @param[in] value value that becomes `current()`.
   *  @throws Any exception thrown by `State` assignment.
   */
  void push(const State &value)
  {
    head_ = (head_ + 1) % Depth;
    values_[head_] = value;
  }

  /** @brief Return the current ring head index.
   *
   *  @return index in `[0, Depth)`.
   *  @throws This function does not throw.
   */
  [[nodiscard]] std::size_t head() const noexcept
  {
    return head_;
  }

  /** @brief Return the raw ring storage.
   *
   *  @return const reference to the stored values.
   *  @throws This function does not throw.
   */
  [[nodiscard]] const std::array<State, Depth> &values() const noexcept
  {
    return values_;
  }

  /** @brief Compare two history cells exactly.
   *
   *  @param[in] rhs cell to compare with.
   *  @return true when head and all slots match.
   *  @throws This function does not throw if `State` comparison does not
   *          throw.
   */
  [[nodiscard]] bool operator==(const History_Cell &rhs) const
  {
    return head_ == rhs.head_ and values_ == rhs.values_;
  }
};

/** @brief Rule adapter for `History_Cell<State, Depth>`.
 *
 *  The wrapped functor can be written in one of three convenient forms:
 *
 *    - `F(const History_Cell<State, Depth>&, Neighbor_View<Cell>) -> State`
 *    - `F(State current, const History_Cell<State, Depth>&,
 *         Neighbor_View<Cell>) -> State`
 *    - `F(State current, State previous) -> State`
 *
 *  The adapter pushes the returned base value into the cell ring and
 *  returns the updated `History_Cell`. A functor that already returns a
 *  full `History_Cell` with signature
 *  `F(const Cell&, Neighbor_View<Cell>) -> Cell` is also accepted.
 *
 *  @tparam Depth number of historical slots retained per cell.
 *  @tparam F wrapped transition functor.
 */
template <std::size_t Depth, typename F>
class History_Rule
{
  static_assert(Depth >= 1, "History_Rule requires Depth >= 1");

  F f_;

public:
  /** @brief Construct a history rule from a functor.
   *
   *  @param[in] func transition functor.
   *  @throws Any exception thrown while moving/copying `func`.
   */
  explicit History_Rule(F func) : f_(std::move(func)) {}

  /** @brief Compute the next history-bearing cell state.
   *
   *  @tparam State base value stored in the history.
   *  @param[in] current current history cell.
   *  @param[in] neighbours neighbour history cells.
   *  @return updated history cell.
   *  @throws Any exception thrown by the wrapped functor.
   */
  template <typename State>
  [[nodiscard]] History_Cell<State, Depth> operator()(
    const History_Cell<State, Depth> &current,
    Neighbor_View<History_Cell<State, Depth>> neighbours) const
  {
    using Cell = History_Cell<State, Depth>;

    if constexpr (std::is_invocable_r_v<Cell, const F &, const Cell &, Neighbor_View<Cell>>)
      {
        return f_(current, neighbours);
      }
    else
      {
        State next{};
        if constexpr (std::is_invocable_r_v<State, const F &, const Cell &, Neighbor_View<Cell>>)
          next = f_(current, neighbours);
        else if constexpr (std::is_invocable_r_v<State, const F &, State, const Cell &,
                                                 Neighbor_View<Cell>>)
          next = f_(current.current(), current, neighbours);
        else if constexpr (std::is_invocable_r_v<State, const F &, State, State>)
          next = f_(current.current(), current.previous(1));
        else
          static_assert(std::is_invocable_r_v<State, const F &, State, State>,
                        "History_Rule functor has an unsupported signature");

        Cell ret = current;
        ret.push(next);
        return ret;
      }
  }
};

/** @brief Helper factory for `History_Rule`.
 *
 *  @tparam Depth history depth.
 *  @tparam F transition functor type.
 *  @param[in] f transition functor.
 *  @return configured `History_Rule`.
 *  @throws Any exception thrown by the constructor.
 */
template <std::size_t Depth, typename F>
[[nodiscard]] History_Rule<Depth, F> make_history_rule(F f)
{
  return History_Rule<Depth, F>(std::move(f));
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_CONTINUOUS_RULES_H
