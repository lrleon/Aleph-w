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

/** @file tpl_ca_stochastic_rules.H
 *  @brief Reproducible stochastic CA rules (Phase 8).
 *
 *  Implements four canonical stochastic transition rules on top of the
 *  Phase 8 RNG framework (`ca-rng.H`):
 *
 *    - `Forest_Fire_Rule(p_growth, p_lightning)`
 *      Drossel–Schwabl forest fire automaton.
 *    - `SIR_Rule(beta, gamma)`
 *      Discrete-time SIR epidemic on a lattice.
 *    - `Ising_Glauber_Rule(temperature, J, H)` /
 *      `Ising_Metropolis_Rule(temperature, J, H)`
 *      Single-spin-flip dynamics with two flip-acceptance strategies.
 *    - `Schelling_Rule(threshold, p_move, p_fill)`
 *      Local approximation of Schelling segregation.
 *
 *  Every rule satisfies `ContextualRuleLike<R, L>` and therefore plugs
 *  directly into `Synchronous_Engine`, `Parallel_Synchronous_Engine`
 *  and `Graph_Synchronous_Engine` via `apply_rule()`.
 *
 *  Determinism contract: for any rule listed above, given the same
 *  initial frame, the same `master_seed`, the same configuration and
 *  the same number of steps, the resulting frame is bit-for-bit
 *  identical regardless of the number of worker threads or the
 *  partitioning strategy used by the engine. This is achieved by
 *  seeding a fresh `Engine` per `(master_seed, step, coord)` triple
 *  via `cell_seed()`.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_STOCHASTIC_RULES_H
#define TPL_CA_STOCHASTIC_RULES_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <type_traits>

#include <ah-errors.H>

#include <ca-rng.H>
#include <ca-traits.H>

namespace Aleph {
namespace CA {

// -----------------------------------------------------------------------
// Forest fire rule (Drossel-Schwabl)
// -----------------------------------------------------------------------

/** @brief Discrete states of the forest-fire automaton.
 *
 *  Encoded as a small integer so that the rule plays nicely with any
 *  integral cell type (`std::uint8_t`, `int`, ...).
 */
enum class Forest_Cell : std::uint8_t
{
  EMPTY = 0,   ///< Empty ground cell.
  TREE = 1,    ///< Healthy tree.
  BURNING = 2  ///< Tree currently on fire (lasts exactly one step).
};

/** @brief Forest-fire rule (Drossel & Schwabl, 1992).
 *
 *  Transition (synchronous, neighbourhood-defined by the engine):
 *
 *    - `BURNING` → `EMPTY`.
 *    - `TREE`    → `BURNING` if at least one neighbour is `BURNING`,
 *                  else `BURNING` with probability `p_lightning`,
 *                  else stays `TREE`.
 *    - `EMPTY`   → `TREE` with probability `p_growth`, else stays
 *                  `EMPTY`.
 *
 *  Probabilities are sampled from a per-cell deterministic engine,
 *  so the trajectory only depends on `master_seed`, the initial
 *  frame and the chosen probabilities.
 *
 *  @tparam Engine RNG engine; defaults to `std::mt19937_64`.
 */
template <typename Engine = std::mt19937_64>
class Forest_Fire_Rule
{
  double p_growth_;
  double p_lightning_;
  std::uint64_t master_seed_;

public:
  /// Underlying RNG engine type.
  using engine_type = Engine;

  /** @brief Build a forest-fire rule with the given probabilities.
   *
   *  @param[in] p_growth probability that an `EMPTY` cell grows a tree.
   *  @param[in] p_lightning probability that a `TREE` ignites
   *             spontaneously when no neighbour is burning.
   *  @param[in] master_seed master seed forwarded to `cell_seed()`.
   *  @throws std::domain_error if `p_growth` or `p_lightning` is not
   *          in `[0, 1]`.
   */
  Forest_Fire_Rule(const double p_growth, const double p_lightning, std::uint64_t master_seed = 0)
    : p_growth_(p_growth), p_lightning_(p_lightning), master_seed_(master_seed)
  {
    ah_domain_error_if(p_growth < 0.0 or p_growth > 1.0)
      << "Forest_Fire_Rule: p_growth=" << p_growth << " outside [0, 1]";
    ah_domain_error_if(p_lightning < 0.0 or p_lightning > 1.0)
      << "Forest_Fire_Rule: p_lightning=" << p_lightning << " outside [0, 1]";
  }

  /// @return the configured tree-growth probability.
  [[nodiscard]] double p_growth() const noexcept
  {
    return p_growth_;
  }

  /// @return the configured lightning probability.
  [[nodiscard]] double p_lightning() const noexcept
  {
    return p_lightning_;
  }

  /// @return the master seed feeding the per-cell streams.
  [[nodiscard]] std::uint64_t master_seed() const noexcept
  {
    return master_seed_;
  }

  /// Replace the master seed; takes effect on the next step.
  void set_master_seed(std::uint64_t s) noexcept
  {
    master_seed_ = s;
  }

  /** @brief Compute the next forest-fire state for cell `current`.
   *
   *  @tparam State integral or enum-typed cell state convertible to
   *          `Forest_Cell`.
   *  @tparam Rank lattice rank carried by `ctx`.
   *  @param[in] current current centre-cell state.
   *  @param[in] neighbours read-only neighbour values.
   *  @param[in] ctx per-cell context with `step` and `coord`.
   *  @return next state produced by the rule.
   *  @throws Any exception thrown by the RNG constructor.
   */
  template <typename State, std::size_t Rank>
  [[nodiscard]] State operator()(const State &current,
                                 Neighbor_View<State> neighbours,
                                 const Cell_Context<Rank> &ctx) const
  {
    const auto cur = static_cast<int>(current);

    if (cur == static_cast<int>(Forest_Cell::BURNING))
      return static_cast<State>(Forest_Cell::EMPTY);

    if (cur == static_cast<int>(Forest_Cell::TREE))
      {
        for (const auto &n : neighbours)
          if (static_cast<int>(n) == static_cast<int>(Forest_Cell::BURNING))
            return static_cast<State>(Forest_Cell::BURNING);

        if (p_lightning_ > 0.0)
          {
            Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
            if (uniform_unit(eng) < p_lightning_)
              return static_cast<State>(Forest_Cell::BURNING);
          }
        return static_cast<State>(Forest_Cell::TREE);
      }

    // EMPTY
    if (p_growth_ > 0.0)
      {
        Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
        if (uniform_unit(eng) < p_growth_)
          return static_cast<State>(Forest_Cell::TREE);
      }
    return static_cast<State>(Forest_Cell::EMPTY);
  }
};

// -----------------------------------------------------------------------
// SIR rule
// -----------------------------------------------------------------------

/// Discrete states of the SIR automaton.
enum class SIR_Cell : std::uint8_t
{
  S = 0,  ///< Susceptible.
  I = 1,  ///< Infected (and infectious).
  R = 2   ///< Recovered (and immune).
};

/** @brief SIR epidemic transition rule.
 *
 *  Synchronous discrete-time SIR on the cellular automaton:
 *
 *    - `R` stays `R`.
 *    - `I` recovers to `R` with probability `gamma`, else stays `I`.
 *    - `S` becomes `I` with probability `1 - (1 - beta)^k` where
 *      `k` is the number of infected neighbours; else stays `S`.
 *
 *  When `R0 = beta * <degree> / gamma < 1`, the epidemic asymptotically
 *  goes extinct on the lattice for any finite seed of infections; the
 *  unit tests cover this regime explicitly.
 *
 *  @tparam Engine RNG engine; defaults to `std::mt19937_64`.
 */
template <typename Engine = std::mt19937_64>
class SIR_Rule
{
  double beta_;
  double gamma_;
  std::uint64_t master_seed_;

public:
  using engine_type = Engine;

  /** @brief Build an SIR rule with infection rate `beta` and
   *         recovery rate `gamma`.
   *
   *  @param[in] beta per-contact infection probability.
   *  @param[in] gamma per-step recovery probability.
   *  @param[in] master_seed master seed forwarded to `cell_seed()`.
   *  @throws std::domain_error if either probability is outside
   *          `[0, 1]`.
   */
  SIR_Rule(double beta, double gamma, std::uint64_t master_seed = 0)
    : beta_(beta), gamma_(gamma), master_seed_(master_seed)
  {
    ah_domain_error_if(beta < 0.0 or beta > 1.0) << "SIR_Rule: beta=" << beta << " outside [0, 1]";
    ah_domain_error_if(gamma < 0.0 or gamma > 1.0)
      << "SIR_Rule: gamma=" << gamma << " outside [0, 1]";
  }

  /// @return the configured infection probability.
  [[nodiscard]] double beta() const noexcept
  {
    return beta_;
  }

  /// @return the configured recovery probability.
  [[nodiscard]] double gamma() const noexcept
  {
    return gamma_;
  }

  /// @return the master seed.
  [[nodiscard]] std::uint64_t master_seed() const noexcept
  {
    return master_seed_;
  }

  /// Replace the master seed.
  void set_master_seed(std::uint64_t s) noexcept
  {
    master_seed_ = s;
  }

  /** @brief Compute the next SIR state for cell `current`.
   *
   *  @tparam State integral or enum-typed cell state convertible to
   *          `SIR_Cell`.
   *  @tparam Rank lattice rank carried by `ctx`.
   *  @param[in] current current centre-cell state.
   *  @param[in] neighbours read-only neighbour values.
   *  @param[in] ctx per-cell context with `step` and `coord`.
   *  @return next state produced by the rule.
   *  @throws Any exception thrown by the RNG constructor.
   */
  template <typename State, std::size_t Rank>
  [[nodiscard]] State operator()(const State &current,
                                 Neighbor_View<State> neighbours,
                                 const Cell_Context<Rank> &ctx) const
  {
    const auto cur = static_cast<int>(current);
    if (cur == static_cast<int>(SIR_Cell::R))
      return current;

    if (cur == static_cast<int>(SIR_Cell::I))
      {
        if (gamma_ <= 0.0)
          return current;
        Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
        return uniform_unit(eng) < gamma_ ? static_cast<State>(SIR_Cell::R) : current;
      }

    // S
    std::size_t infected = 0;
    for (const auto &n : neighbours)
      if (static_cast<int>(n) == static_cast<int>(SIR_Cell::I))
        ++infected;

    if (infected == 0 or beta_ <= 0.0)
      return current;

    const double survive = std::pow(1.0 - beta_, static_cast<double>(infected));
    const double infect_prob = 1.0 - survive;

    Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
    return uniform_unit(eng) < infect_prob ? static_cast<State>(SIR_Cell::I) : current;
  }
};

// -----------------------------------------------------------------------
// Ising-style rules
// -----------------------------------------------------------------------

namespace ca_ising_detail {

/// Map any integral state to a spin in `{-1, +1}`. Signed types use
/// the sign of the value (zero is treated as +1); unsigned types
/// treat zero as `-1` and any non-zero as `+1`.
template <typename State>
[[nodiscard]] inline int state_to_spin(const State &v) noexcept
{
  if constexpr (std::is_signed_v<State>)
    return v >= State{0} ? +1 : -1;
  else
    return v != State{} ? +1 : -1;
}

/// Inverse of `state_to_spin`. Signed types receive `±1`; unsigned
/// types receive `0` for `-1` and `1` for `+1`.
template <typename State>
[[nodiscard]] inline State spin_to_state(int spin) noexcept
{
  if constexpr (std::is_signed_v<State>)
    return spin > 0 ? static_cast<State>(1) : static_cast<State>(-1);
  else
    return spin > 0 ? static_cast<State>(1) : static_cast<State>(0);
}

/// Compute the local energy delta for flipping the centre spin.
template <typename State>
[[nodiscard]] inline double delta_energy(const State &current,
                                         Neighbor_View<State> neighbours,
                                         double J,
                                         double H)
{
  const int s = state_to_spin(current);
  long long sum = 0;
  for (const auto &n : neighbours)
    sum += state_to_spin(n);
  return 2.0 * static_cast<double>(s) * (J * static_cast<double>(sum) + H);
}

}  // namespace ca_ising_detail

/** @brief Ising rule with Glauber single-spin-flip dynamics.
 *
 *  At each cell the flip probability is
 *
 *      p_flip = 1 / (1 + exp(deltaE / T))
 *
 *  where `deltaE = 2 * s * (J * sum_neighbours + H)`. Glauber dynamics
 *  satisfy detailed balance and converge to the canonical Boltzmann
 *  distribution for the Ising Hamiltonian
 *
 *      H = -J * sum_<i,j> s_i s_j - H * sum_i s_i.
 *
 *  Cell states must be integral. Signed types are treated as `±1`;
 *  unsigned types as `{0, 1}` (with `0` representing spin `-1`).
 *
 *  @tparam Engine RNG engine; defaults to `std::mt19937_64`.
 */
template <typename Engine = std::mt19937_64>
class Ising_Glauber_Rule
{
  double T_;
  double J_;
  double H_;
  std::uint64_t master_seed_;

public:
  using engine_type = Engine;

  /** @brief Build a Glauber Ising rule.
   *
   *  @param[in] T temperature; must be strictly positive.
   *  @param[in] J coupling constant (default `+1`, ferromagnetic).
   *  @param[in] H external magnetic field (default `0`).
   *  @param[in] master_seed master seed forwarded to `cell_seed()`.
   *  @throws std::domain_error if `T <= 0`.
   */
  Ising_Glauber_Rule(const double T,
                     const double J = 1.0,
                     const double H = 0.0,
                     const std::uint64_t master_seed = 0)
    : T_(T), J_(J), H_(H), master_seed_(master_seed)
  {
    ah_domain_error_if(T <= 0.0) << "Ising_Glauber_Rule: temperature must be positive, got " << T;
  }

  [[nodiscard]] double temperature() const noexcept
  {
    return T_;
  }

  [[nodiscard]] double coupling() const noexcept
  {
    return J_;
  }

  [[nodiscard]] double field() const noexcept
  {
    return H_;
  }

  [[nodiscard]] std::uint64_t master_seed() const noexcept
  {
    return master_seed_;
  }

  void set_master_seed(std::uint64_t s) noexcept
  {
    master_seed_ = s;
  }

  /** @brief Compute the next Ising state for cell `current`.
   *
   *  @tparam State integral cell state.
   *  @tparam Rank lattice rank.
   *  @param[in] current current centre-cell state.
   *  @param[in] neighbours read-only neighbour values.
   *  @param[in] ctx per-cell context.
   *  @return next state produced by the rule.
   */
  template <typename State, std::size_t Rank>
  [[nodiscard]] State operator()(const State &current,
                                 Neighbor_View<State> neighbours,
                                 const Cell_Context<Rank> &ctx) const
  {
    const double dE = ca_ising_detail::delta_energy(current, neighbours, J_, H_);
    // Numerically stable Fermi function 1 / (1 + exp(dE / T)):
    //   for dE >= 0 we compute 1 / (1 + exp(dE/T));
    //   for dE <  0 we use      exp(-dE/T) / (1 + exp(-dE/T))
    const double x = dE / T_;
    const double p_flip = x >= 0.0 ? 1.0 / (1.0 + std::exp(x)) : std::exp(-x) / (1.0 + std::exp(-x));

    if (Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
        uniform_unit(eng) < p_flip)
      return ca_ising_detail::spin_to_state<State>(-ca_ising_detail::state_to_spin(current));
    return current;
  }
};

/** @brief Ising rule with Metropolis–Hastings single-spin-flip dynamics.
 *
 *  Always flip when `deltaE <= 0`; otherwise flip with probability
 *  `exp(-deltaE / T)`. Like Glauber dynamics, Metropolis dynamics
 *  satisfy detailed balance and target the Boltzmann distribution of
 *  the Ising Hamiltonian.
 *
 *  @tparam Engine RNG engine; defaults to `std::mt19937_64`.
 */
template <typename Engine = std::mt19937_64>
class Ising_Metropolis_Rule
{
  double T_;
  double J_;
  double H_;
  std::uint64_t master_seed_;

public:
  using engine_type = Engine;

  /** @brief Build a Metropolis Ising rule.
   *
   *  @param[in] T temperature; must be strictly positive.
   *  @param[in] J coupling constant (default `+1`).
   *  @param[in] H external magnetic field (default `0`).
   *  @param[in] master_seed master seed forwarded to `cell_seed()`.
   *  @throws std::domain_error if `T <= 0`.
   */
  Ising_Metropolis_Rule(const double T,
                        const double J = 1.0,
                        const double H = 0.0,
                        const std::uint64_t master_seed = 0)
    : T_(T), J_(J), H_(H), master_seed_(master_seed)
  {
    ah_domain_error_if(T <= 0.0) << "Ising_Metropolis_Rule: temperature must be positive, got " << T;
  }

  [[nodiscard]] double temperature() const noexcept
  {
    return T_;
  }

  [[nodiscard]] double coupling() const noexcept
  {
    return J_;
  }

  [[nodiscard]] double field() const noexcept
  {
    return H_;
  }

  [[nodiscard]] std::uint64_t master_seed() const noexcept
  {
    return master_seed_;
  }

  void set_master_seed(std::uint64_t s) noexcept
  {
    master_seed_ = s;
  }

  /** @copydoc Ising_Glauber_Rule::operator() */
  template <typename State, std::size_t Rank>
  [[nodiscard]] State operator()(const State &current,
                                 Neighbor_View<State> neighbours,
                                 const Cell_Context<Rank> &ctx) const
  {
    const double dE = ca_ising_detail::delta_energy(current, neighbours, J_, H_);
    if (dE <= 0.0)
      return ca_ising_detail::spin_to_state<State>(-ca_ising_detail::state_to_spin(current));

    const double p_flip = std::exp(-dE / T_);
    if (Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
        uniform_unit(eng) < p_flip)
      return ca_ising_detail::spin_to_state<State>(-ca_ising_detail::state_to_spin(current));
    return current;
  }
};

// -----------------------------------------------------------------------
// Schelling segregation rule
// -----------------------------------------------------------------------

/// Discrete states of the Schelling automaton.
enum class Schelling_Cell : std::uint8_t
{
  EMPTY = 0,   ///< Vacant cell.
  TYPE_A = 1,  ///< Agent of type A.
  TYPE_B = 2   ///< Agent of type B.
};

/** @brief Local approximation of the Schelling segregation model.
 *
 *  The classical Schelling model is non-local: unhappy agents *move*
 *  to a vacant cell anywhere on the lattice. The CA-friendly local
 *  approximation implemented here updates each cell synchronously:
 *
 *    - For an occupied cell, count same-type and other-type
 *      *occupied* neighbours. The cell is *unhappy* when the fraction
 *      `same / (same + other)` is strictly below `threshold`. An
 *      unhappy cell vacates with probability `p_move` (becomes
 *      `EMPTY`); otherwise it stays put.
 *    - For an empty cell, look at its occupied neighbours: if one
 *      type strictly dominates, the cell is filled with that type
 *      with probability `p_fill`. With equal counts (or no occupied
 *      neighbours), the cell stays `EMPTY`.
 *
 *  This is *not* the canonical Schelling dynamics — it is the
 *  standard local CA approximation used in pedagogical settings
 *  (Vinkovic & Kirman, 2006, similar). It preserves the qualitative
 *  segregation behaviour while remaining fully synchronous and local.
 *
 *  @tparam Engine RNG engine; defaults to `std::mt19937_64`.
 */
template <typename Engine = std::mt19937_64>
class Schelling_Rule
{
  double threshold_;
  double p_move_;
  double p_fill_;
  std::uint64_t master_seed_;

public:
  using engine_type = Engine;

  /** @brief Build a Schelling rule.
   *
   *  @param[in] threshold minimum same-type fraction below which an
   *             agent becomes unhappy. Must be in `[0, 1]`.
   *  @param[in] p_move probability that an unhappy agent vacates the
   *             cell (default `1.0`). Must be in `[0, 1]`.
   *  @param[in] p_fill probability that an empty cell with a strict
   *             majority of one neighbour type is filled (default
   *             `1.0`). Must be in `[0, 1]`.
   *  @param[in] master_seed master seed forwarded to `cell_seed()`.
   *  @throws std::domain_error if any probability is outside `[0, 1]`.
   */
  Schelling_Rule(const double threshold,
                 const double p_move = 1.0,
                 const double p_fill = 1.0,
                 const std::uint64_t master_seed = 0)
    : threshold_(threshold), p_move_(p_move), p_fill_(p_fill), master_seed_(master_seed)
  {
    ah_domain_error_if(threshold < 0.0 or threshold > 1.0)
      << "Schelling_Rule: threshold=" << threshold << " outside [0, 1]";
    ah_domain_error_if(p_move < 0.0 or p_move > 1.0)
      << "Schelling_Rule: p_move=" << p_move << " outside [0, 1]";
    ah_domain_error_if(p_fill < 0.0 or p_fill > 1.0)
      << "Schelling_Rule: p_fill=" << p_fill << " outside [0, 1]";
  }

  [[nodiscard]] double threshold() const noexcept
  {
    return threshold_;
  }

  [[nodiscard]] double p_move() const noexcept
  {
    return p_move_;
  }

  [[nodiscard]] double p_fill() const noexcept
  {
    return p_fill_;
  }

  [[nodiscard]] std::uint64_t master_seed() const noexcept
  {
    return master_seed_;
  }

  void set_master_seed(std::uint64_t s) noexcept
  {
    master_seed_ = s;
  }

  /** @brief Compute the next Schelling state for cell `current`.
   *  @tparam State integral or enum-typed cell state.
   *  @tparam Rank lattice rank.
   */
  template <typename State, std::size_t Rank>
  [[nodiscard]] State operator()(const State &current,
                                 Neighbor_View<State> neighbours,
                                 const Cell_Context<Rank> &ctx) const
  {
    std::size_t a = 0, b = 0;
    for (const auto &n : neighbours)
      if (const auto x = static_cast<int>(n); x == static_cast<int>(Schelling_Cell::TYPE_A))
        ++a;
      else if (x == static_cast<int>(Schelling_Cell::TYPE_B))
        ++b;

    const auto cur = static_cast<int>(current);

    if (cur == static_cast<int>(Schelling_Cell::EMPTY))
      {
        if (a == b or p_fill_ <= 0.0)
          return current;
        if (Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
            uniform_unit(eng) < p_fill_)
          return a > b ? static_cast<State>(Schelling_Cell::TYPE_A)
                       : static_cast<State>(Schelling_Cell::TYPE_B);
        return current;
      }

    const std::size_t total = a + b;
    if (total == 0)
      return current;  // Isolated agent: no peer pressure either way.

    const std::size_t same = (cur == static_cast<int>(Schelling_Cell::TYPE_A)) ? a : b;

    if (const double fraction = static_cast<double>(same) / static_cast<double>(total);
        fraction >= threshold_ or p_move_ <= 0.0)
      return current;

    if (Engine eng{static_cast<typename Engine::result_type>(cell_seed<Rank>(master_seed_, ctx))};
        uniform_unit(eng) < p_move_)
      return static_cast<State>(Schelling_Cell::EMPTY);
    return current;
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_STOCHASTIC_RULES_H
