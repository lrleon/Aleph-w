# Tutorial: Number Theoretic Transform in Aleph-w

This tutorial covers the current industrial NTT core in `ntt.H`.  The focus of
this phase is the exact modular transform and its first layer of formal
polynomial algebra: reusable plans, Montgomery arithmetic in the butterfly,
arbitrary-size support through Bluestein when the modulus admits the required
roots, batch APIs, explicit parallelism with `ThreadPool`, exact multi-prime
CRT reconstruction, modular polynomial operators, and fixed-base big-integer
multiplication, including negacyclic convolution.

## 1. What is the NTT?

The Number Theoretic Transform is the modular analogue of the FFT.  Instead of
working over complex numbers, it works over residues modulo a prime `p`:

- FFT domain: `std::complex<Real>`
- NTT domain: `uint64_t` modulo `p`
- FFT result: approximate (floating-point rounding)
- NTT result: exact modulo `p`

For a power-of-two transform of size `N`, the modulus must satisfy `N | (p - 1)`
so that an `N`-th primitive root of unity exists in `Z / p Z`.

In that case, the forward and inverse transforms keep the same algebraic shape
as the classical DFT, but over a finite field:

```text
F[k] = sum_{j=0}^{N-1} f[j] * w^(j*k) mod p
f[j] = N^(-1) * sum_{k=0}^{N-1} F[k] * w^(-j*k) mod p
```

where `w` is an `N`-th primitive root of unity and `N^(-1)` is the modular
inverse of `N`.

### 1.1 FFT vs NTT

| Aspect | FFT | NTT |
|--------|-----|-----|
| Domain | `std::complex<Real>` | `uint64_t mod p` |
| Twiddles | `exp(-2 pi i / N)` | `g^((p - 1) / N) mod p` |
| Precision | Approximate | Exact modulo `p` |
| Size constraint | Any `N` with Bluestein | Requires the needed roots to exist in `Z/pZ` |
| Primary use case | DSP, spectra, filtering | Exact modular products, CRT, formal algebra |
| Overflow / range | Accumulated numerical error | Coefficients bounded by `p` or by the CRT pack |

## 2. Architecture of `ntt.H`

The header is organized in layers:

- `NTT<MOD, ROOT>`: sequential and parallel transform/convolution core
- `NTT<MOD, ROOT>::Plan`: reusable cache for bit-reversal, twiddles, or
  Bluestein data
- `modular_arithmetic.H`: `MontgomeryCtx`, `mont_mul()`, and `mont_reduce()`
  for the butterfly hot loop
- runtime SIMD dispatch: AVX2/NEON on the power-of-two path
- `NTTExact`: three-prime CRT pack reconstructed in `__uint128_t`
- high-level algebra layer: formal series, multipoint evaluation,
  interpolation, bigint, and negacyclic convolution

The current implementation provides:

- `NTT<MOD, ROOT>::transform()` and `transformed()`
- `NTT<MOD, ROOT>::multiply()` and `multiply_inplace()`
- `NTT<MOD, ROOT>::transform_batch()`
- `NTT<MOD, ROOT>::ptransform()` and `pmultiply()`
- `NTT<MOD, ROOT>::Plan` for repeated transforms of the same size
- `NTT<MOD, ROOT>::simd_backend_name()`,
  `avx2_dispatch_available()`, and `neon_dispatch_available()`
- `NTTExact::multiply()` and `NTTExact::pmultiply()` for exact `__uint128_t`
  products
- `NTT<MOD, ROOT>::poly_eval()`, `poly_inverse()`, and `poly_divmod()`
- `NTT<MOD, ROOT>::poly_log()`, `poly_exp()`, `poly_sqrt()`, and `poly_power()`
- `NTT<MOD, ROOT>::multipoint_eval()` and `interpolate()`
- `NTT<MOD, ROOT>::bigint_multiply()` and `pbigint_multiply()` for big
  integers represented as base-`B` digits
- `NTT<MOD, ROOT>::negacyclic_multiply()` for products modulo `x^N + 1`

The current scope is intentionally strict:

- power-of-two sizes use the direct Cooley-Tukey kernel
- non-power-of-two sizes use Bluestein when `2 * N` divides `MOD - 1`
- the internal Bluestein convolution size must still fit the maximum supported
  power of two for the modulus
- all coefficients are reduced modulo `MOD`
- the current SIMD path accelerates the power-of-two butterfly with grouped
  AVX2 or NEON modular add/sub steps while keeping Montgomery multiplication
  per lane
- multi-threaded `ThreadPool` paths still prefer the scalar kernel to avoid
  mixing stage parallelism with SIMD dispatch
- `NTTExact` uses a fixed three-prime pack and only accepts inputs whose exact
  product can be guaranteed inside the CRT reconstruction range
- formal polynomial operators work on truncated series modulo `x^n` and enforce
  the usual invertibility/log/exp/square-root preconditions
- the bigint API takes little-endian digits (`a[0]` least significant),
  trims high zeros, and returns `{0}` for the zero product
- negacyclic convolution requires `a.size() == b.size() == N`, `N` to be a
  power of two, and a primitive root of order `2N` under the current modulus

## 3. Basic Usage

```cpp
# include <ntt.H>

using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {1, 2, 3};
Aleph::Array<uint64_t> b = {4, 5};

auto c = DefaultNTT::multiply(a, b);
// c = {4, 13, 22, 15} modulo 998244353
```

For direct transforms:

```cpp
Aleph::Array<uint64_t> values = {3, 1, 4, 1, 5, 9, 2, 6};

DefaultNTT::transform(values, false); // forward NTT
DefaultNTT::transform(values, true);  // inverse NTT
```

After the inverse transform, `values` returns to the original coefficients
modulo `MOD`.

## 4. Reusable Plans

If the same size is transformed repeatedly, use a plan:

```cpp
using DefaultNTT = Aleph::NTT<>;

DefaultNTT::Plan plan(7);

Aleph::Array<uint64_t> x = ...; // size 7
Aleph::Array<uint64_t> y = ...; // size 7

plan.transform(x, false);
plan.transform(y, false);
```

For power-of-two sizes, `Plan` precomputes:

- bit-reversal permutation
- forward twiddles in Montgomery form
- inverse twiddles in Montgomery form
- `N^{-1} mod MOD` for inverse normalization

For supported non-power-of-two sizes, `Plan` switches to Bluestein and
precomputes:

- chirp factors
- transformed convolution kernels
- an internal power-of-two plan for the convolution

## 5. Why Montgomery Arithmetic Matters

The transform performs one modular multiplication per butterfly.  In the
current core, those multiplications are done with `MontgomeryCtx`, `mont_mul()`, and
`mont_reduce()` from `modular_arithmetic.H`.

The important consequence is:

- additions/subtractions stay as ordinary modular operations
- multiplications avoid generic division-based reduction inside the hot loop

This keeps the public API in standard residues while the transform core runs in
Montgomery form internally.

## 6. Batch Transforms

When many arrays have the same transform size, batch APIs remove repeated plan
construction and make the intent explicit:

```cpp
Aleph::NTT<>::Plan plan(8);

Aleph::Array<Aleph::Array<uint64_t>> batch = {
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 0, 0, 0, 0}
};

plan.transform_batch(batch, false);
```

All batch items must match the plan size exactly.

## 7. Parallel Execution

Parallel APIs use the repository `ThreadPool`:

```cpp
# include <thread_pool.H>

Aleph::ThreadPool pool(4);
Aleph::NTT<>::Plan plan(1 << 12);

Aleph::Array<uint64_t> values = ...;
plan.ptransform(pool, values, false);

auto product = Aleph::NTT<>::pmultiply(pool, a, b);
```

The current strategy is:

- one transform: parallelize work inside stages
- batch transform: parallelize across batch items when possible

This avoids nested oversubscription.

## 8. SIMD Dispatch

The active SIMD backend can be queried at runtime:

```cpp
using DefaultNTT = Aleph::NTT<>;

const char * backend = DefaultNTT::simd_backend_name();
bool has_avx2 = DefaultNTT::avx2_dispatch_available();
bool has_neon = DefaultNTT::neon_dispatch_available();
```

Current policy:

- `ALEPH_NTT_SIMD=auto|scalar|avx2|neon`
- `auto` selects the best available backend
- SIMD is currently applied only on the power-of-two path
- Bluestein and multi-threaded variants still use the scalar kernel

## 9. Exact Multiplication with CRT

When the true convolution coefficients may exceed a single modulus, `NTTExact`
runs three independent NTTs and reconstructs each coefficient in
`__uint128_t`:

```cpp
Aleph::Array<uint64_t> a = {
  1000000000000ULL,
  Aleph::NTT<>::mod + 77ULL,
  1234567890123ULL
};
Aleph::Array<uint64_t> b = {
  777777777777ULL,
  42ULL,
  Aleph::NTT<>::mod + 11ULL
};

Aleph::Array<__uint128_t> exact = Aleph::NTTExact::multiply(a, b);
```

Important contract:

- reconstruction uses the fixed primes `998244353`, `469762049`, and
  `1004535809`
- the result is exact only when every true coefficient is `< p1 * p2 * p3`
- the implementation applies conservative sufficient bounds and throws
  `std::invalid_argument` when exactness cannot be guaranteed
- `NTTExact::pmultiply(pool, a, b)` parallelizes first across the three primes
  and then across CRT reconstruction

## 10. Supported Size Queries

The header exposes two useful utilities:

```cpp
using DefaultNTT = Aleph::NTT<>;

auto max_n = DefaultNTT::max_transform_size();
bool ok = DefaultNTT::supports_size(1024);
auto w = DefaultNTT::primitive_root_of_unity(1024);
```

For the default modulus `998244353`:

- the maximum direct power-of-two size is `2^23`
- sizes such as `7`, `14`, `17`, and `119` are also supported
- sizes such as `3`, `10`, or `13` are not supported because the required roots
  do not exist under that modulus

## 11. Formal Polynomial Algebra

On top of the same NTT core, `ntt.H` now exposes standard modular polynomial
operators.

### 11.1 Evaluation and Division

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> f = {9, 7, 5, 3};
uint64_t value = DefaultNTT::poly_eval(f, 9);

auto qr = DefaultNTT::poly_divmod(f, Aleph::Array<uint64_t>({1, 1, 1}));
Aleph::Array<uint64_t> q = qr.first;
Aleph::Array<uint64_t> r = qr.second;
```

`poly_divmod()` returns normalized quotient and remainder.  Dividing by the
zero polynomial raises `std::invalid_argument`.

### 11.2 Formal Series

The following operators are truncated modulo `x^n`:

```cpp
Aleph::Array<uint64_t> unit = {1, 3, 4, 0, 2, 1};

auto inv = DefaultNTT::poly_inverse(unit, 6);
auto lg = DefaultNTT::poly_log(unit, 6);
auto ex = DefaultNTT::poly_exp(Aleph::Array<uint64_t>({0, 5, 7, 0}), 6);
auto pw = DefaultNTT::poly_power(unit, 3, 6);
```

Algebraic contract:

- `poly_inverse(f, n)` requires `f(0) != 0 mod MOD`
- `poly_log(f, n)` requires `f(0) = 1`
- `poly_exp(f, n)` requires `f(0) = 0`
- `poly_power(f, k, n)` follows the standard formal-series decomposition after
  extracting the leading shift and also covers `k = 0`

Internally:

- inversion uses Newton doubling
- `poly_log()` computes `f' / f` and then applies the formal integral
- `poly_exp()` iterates with `g *= (1 - log(g) + f)`
- `poly_power()` reduces to `log/exp` after separating the leading term

### 11.3 Formal Square Root

```cpp
Aleph::Array<uint64_t> square = {4, 12, 25, 24, 16};
auto root = DefaultNTT::poly_sqrt(square, 5);
```

`poly_sqrt()` supports two cases:

- non-zero constant term with a modular square root, solved with
  Tonelli-Shanks plus Newton iteration
- polynomials with leading zeros at an even degree, where the square root is
  reduced recursively after factoring `x^(2m)`

If the first non-zero term appears at an odd degree or the constant term is
not a quadratic residue modulo `MOD`, the function raises
`std::invalid_argument`.

### 11.4 Multipoint Evaluation and Interpolation

```cpp
Aleph::Array<uint64_t> points = {2, 4, 8, 16};
Aleph::Array<uint64_t> values = DefaultNTT::multipoint_eval(f, points);
Aleph::Array<uint64_t> recovered = DefaultNTT::interpolate(points, values);
```

The implementation uses a subproduct tree:

- `multipoint_eval()` reduces remainders recursively against each node
- `interpolate()` evaluates the derivative of the total product on the points
  and combines subtree polynomials bottom-up

`multipoint_eval()` accepts repeated points.  `interpolate()` does require
pairwise-distinct points modulo `MOD`; duplicates raise `std::invalid_argument`.

## 12. Big Integer Multiplication

The same exact convolution infrastructure can also multiply non-negative
integers represented as digits in base `B`.

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {0, 9, 8, 7, 6}; // 67890 in base 10
Aleph::Array<uint64_t> b = {0, 1, 2, 3, 4}; // 43210 in base 10

auto product = DefaultNTT::bigint_multiply<10>(a, b);
```

Representation contract:

- `a[0]` and `b[0]` are the least-significant digits
- every digit must satisfy `0 <= digit < Base`
- the output is normalized by trimming high zeros, except that zero is
  represented as `{0}`
- the default base is `2^15`, which packs dense chunks efficiently in
  `uint64_t`

Exact-backend selection:

- if `min(a.size(), b.size()) * (Base - 1)^2 < MOD` and the required
  convolution size is supported by `NTT<MOD, ROOT>`, a single-prime exact path
  is used
- otherwise the API falls back to `NTTExact`, reconstructs exact convolution
  coefficients, and then propagates carries
- `pbigint_multiply()` parallelizes the convolution; carry propagation remains
  a linear pass

For large decimal integers, `Base = 10` is convenient in tests and demos.  For
actual workloads, the default base reduces the total digit count.

## 13. Negacyclic Convolution

`ntt.H` also exposes multiplication modulo `(x^N + 1)`, which is useful in
lattice-based cryptography and in Schonhage-Strassen-style kernels.

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {1, 1, 1, 1};
Aleph::Array<uint64_t> b = {1, 1, 0, 0};

auto c = DefaultNTT::negacyclic_multiply(a, b);
// c = {0, 2, 2, 2} modulo 998244353
```

Contract:

- `a.size() == b.size() == N`
- `N` must be a power of two
- a primitive root of order `2N` must exist, so for power-of-two-friendly
  moduli this effectively requires `2N <= max_transform_size()`

The implementation follows the classical scheme:

- twist by a `2N`-th root `zeta`
- perform an `N`-point forward NTT on `a[i] * zeta^i` and `b[i] * zeta^i`
- multiply pointwise
- apply the inverse NTT and untwist by `zeta^{-i}`

## 14. Limitations of This Phase

This tutorial describes the implemented phase only.  The following are planned
next steps, but are not part of the current header:

- automatic prime selection
- exact reconstruction beyond the three-prime range
- acceptance of every valid CRT-fit input: the current bound is conservative
  and may reject some inputs that would still fit

## 15. Example Program

See `Examples/ntt_example.cc` for a compact executable walkthrough of:

- polynomial multiplication
- active SIMD backend
- exact CRT multiplication
- formal polynomial algebra
- base-10 big integer multiplication
- negacyclic convolution
- reusable plans
- batch transforms
- parallel multiplication

`math_nt_example.cc` still shows the broader number-theory toolbox around the
NTT support.

## 16. Quick Reference

| Need | API |
|------|-----|
| Forward/inverse transform | `NTT<>::transform(a, invert)` |
| Modular product | `NTT<>::multiply(a, b)` |
| Reusable plan | `NTT<>::Plan plan(n)` |
| Explicit parallelism | `NTT<>::ptransform(pool, a, invert)`, `pmultiply()` |
| Batch execution | `plan.transform_batch(batch, invert)` |
| SIMD backend | `simd_backend_name()`, `avx2_dispatch_available()`, `neon_dispatch_available()` |
| Exact CRT product | `NTTExact::multiply(a, b)`, `NTTExact::pmultiply(pool, a, b)` |
| Formal algebra | `poly_inverse`, `poly_divmod`, `poly_log`, `poly_exp`, `poly_sqrt`, `poly_power` |
| Evaluation / interpolation | `poly_eval`, `multipoint_eval`, `interpolate` |
| Big integers | `bigint_multiply<Base>()`, `pbigint_multiply<Base>()` |
| Modulo `x^N + 1` | `negacyclic_multiply(a, b)` |
| Modulus capability | `max_transform_size()`, `supports_size(n)`, `primitive_root_of_unity(n)` |
