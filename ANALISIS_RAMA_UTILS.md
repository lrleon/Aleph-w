# Análisis Completo de la Rama `utils`

**Fecha**: 2026-02-27
**Rama**: `utils` (vs `master`)
**Cambios**: 30 archivos, +5577 líneas (sort, combinatorics, NT, streaming)

---

## Resumen Ejecutivo

La rama añade 6 algoritmos de teoría de números, 5 estructuras de datos probabilísticas, y mejoras a sorting y combinatorios. **Hay 5-6 bugs críticos que causan UB o resultados incorrectos**, sistemáticas violaciones de convención en **todos** los archivos nuevos de math/NT, y gaps de cobertura de tests.

---

# 1. BUGS CRÍTICOS (UB o resultados incorrectos)

## Bug 1: `modular_arithmetic.H:149` — Cast `uint64_t` → `int64_t` causa UB

```cpp
const int64_t g = ext_gcd(static_cast<int64_t>(a), static_cast<int64_t>(m), x, y);
```

**Problema**: Si `a` o `m` >= 2^63, el `static_cast<int64_t>` produce **undefined behavior** (signed overflow en C++).

**Propagación**: `mod_inv` es llamado desde:
- `modular_combinatorics.H` (constructor, línea 81)
- `modular_linalg.H` (líneas 181, 293)
- `ntt.H` (línea 122)

**Propuesta de arreglo**:
```cpp
// Opción 1: Reescribir ext_gcd para uint64_t
template <typename T>
  requires std::is_unsigned_v<T>
T ext_gcd(T a, T b, T & x, T & y);

// Opción 2: Validar el rango
ah_domain_error_if(a > INT64_MAX or m > INT64_MAX)
  << "mod_inv: input exceeds int64_t range";
```

---

## Bug 2: `modular_linalg.H:181,293` — Mismo cast peligroso

```cpp
const uint64_t inv = mod_inv(static_cast<int64_t>(diag), static_cast<int64_t>(mod));
uint64_t pivot_inv = mod_inv(static_cast<int64_t>(pivot_val), static_cast<int64_t>(mod));
```

**Problema**: Los casts a `int64_t` son **innecesarios y dañinos**. `mod_inv` actualmente acepta `uint64_t`, así que los casts rompen el tipo.

**Solución rápida**:
```cpp
const uint64_t inv = mod_inv(diag, mod);
uint64_t pivot_inv = mod_inv(pivot_val, mod);
```

---

## Bug 3: `modular_arithmetic.H:64,95` — División por cero con `m == 0`

### En `mod_mul` (línea 64):
```cpp
template <typename T>
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m) noexcept
{
  __uint128_t res = static_cast<__uint128_t>(a) * b;
  return res % m;  // ← UB si m == 0
}
```

### En `mod_exp` (línea 95):
```cpp
result = mod_mul(result, base, m);  // ← Llamará mod_mul con m == 0
```

**Propuesta**:
```cpp
ah_domain_error_if(m == 0) << "Modulus cannot be 0";
```

---

## Bug 4: `ntt.H:72,94` — `int n = a.size()` trunca `size_t`

```cpp
int n = a.size();  // ← a.size() retorna size_t
// ... luego:
for (size_t len = 1; len < n; len <<= 1)  // ← signed shift con int
```

**Problemas**:
1. Trunca arrays mayores a 2^31 elementos
2. Signed left shift sobre `int` puede ser UB si sign bit está involucrado
3. Mezcla signed/unsigned en expresiones

**Propuesta**:
```cpp
size_t n = a.size();
// Y cambiar 'len' de int a size_t en todo el archivo
```

---

## Bug 5: `count-min-sketch.H:153` — `Array<T>` no define `operator!=`

```cpp
template <typename CounterT>
void merge(const Count_Min_Sketch<CounterT> & other)
{
  ah_domain_error_if(seeds_ != other.seeds_)  // ← ERROR: Array no tiene operator!=
    << "...";
}
```

**Propuesta**:
```cpp
ah_domain_error_if(seeds_.size() != other.seeds_.size())
  << "Sketch seeds size mismatch";
for (size_t i = 0; i < seeds_.size(); ++i)
  ah_domain_error_if(seeds_(i) != other.seeds_(i))
    << "Sketch seeds mismatch";
```

---

## Bug 6: `modular_arithmetic.H:171-173` — CRT overflow silencioso

```cpp
uint64_t prod = 1;
for (const uint64_t m : mod)
    prod *= m;  // ← Overflows sin detección
```

**Ejemplo que falla**: 3 módulos de ~2^22 cada uno overflow en `uint64_t`.

**Propuesta**:
```cpp
uint64_t prod = 1;
for (const uint64_t m : mod) {
  ah_domain_error_if(prod > UINT64_MAX / m)
    << "CRT: moduli product overflows uint64_t";
  prod *= m;
}
```

---

# 2. BUGS MODERADOS (Edge cases, comportamiento incorrecto)

## Bug 7: `reservoir-sampling.H:101` — Overflow de contador

```cpp
const size_t j = gsl_rng_uniform_int(rng_.get(), n_seen_ + 1);
++n_seen_;
```

Si `n_seen_ == SIZE_MAX`, entonces `n_seen_ + 1` wraps a 0 y `gsl_rng_uniform_int(rng, 0)` es UB.

**Propuesta**:
```cpp
ah_domain_error_if(n_seen_ == std::numeric_limits<size_t>::max())
  << "reservoir_sampler: stream length exceeded";
```

---

## Bug 8: `reservoir-sampling.H:130-135` — `clear()` no resetea RNG

```cpp
void clear()
{
  reservoir_.clear();
  n_seen_ = 0;
  // ← RNG no se reinicia, aunque doc dice "keeping seed"
}
```

El problema: el seed nunca se guarda, así que no se puede reiniciar. Después de `clear()` y reprocesar el mismo stream, obtienes resultados *diferentes*.

**Propuesta**:
```cpp
class Reservoir_Sampler {
  unsigned long seed_;  // ← Guardar el seed

  Reservoir_Sampler(size_t k, unsigned long seed = time(nullptr))
    : k_(k), seed_(seed), rng_(gsl_rng_alloc(gsl_rng_taus),
                             [](gsl_rng *r) { gsl_rng_free(r); }) {
    gsl_rng_set(rng_.get(), seed_);
  }

  void clear() {
    reservoir_.clear();
    n_seen_ = 0;
    gsl_rng_set(rng_.get(), seed_);  // ← Reiniciar RNG
  }
};
```

---

## Bug 9: `modular_combinatorics.H:99` — Acceso out-of-bounds con `noexcept`

```cpp
[[nodiscard]] uint64_t nCk(size_t n, size_t k) const noexcept
{
  ah_domain_error_if(k > n) << "k must be <= n";
  const uint64_t num = fact_[n];  // ← UB si n > max_n
  const uint64_t den = mod_mul(invFact_[k], invFact_[n - k], mod_);
}
```

La función es `noexcept` pero puede acceder fuera de `fact_` si `n > max_n`. El Doxygen dice "Requires n <= max_n" pero no se valida.

**Propuesta**:
```cpp
[[nodiscard]] uint64_t nCk(size_t n, size_t k) const
{
  ah_domain_error_if(k > n) << "k must be <= n";
  ah_domain_error_if(n > max_n) << "n must be <= max_n (" << max_n << ")";
  return mod_mul(fact_[n], mod_mul(invFact_[k], invFact_[n - k], mod_), mod_);
}
```

---

## Bug 10: `pollard_rho.H:97-104` — Loop infinito sin límite

```cpp
uint64_t d = n;
while (d == n)  // ← Sin límite de intentos
{
    const uint64_t seed = rng() % (n - 2) + 2;
    const uint64_t c = rng() % (n - 1) + 1;
    d = pollard_rho_step(n, seed, c);
}
```

Si los seeds son patológicamente malos, puede loopear indefinidamente.

**Propuesta**:
```cpp
uint64_t d = n;
for (size_t attempts = 0; attempts < 100 and d == n; ++attempts) {
  const uint64_t seed = rng() % (n - 2) + 2;
  const uint64_t c = rng() % (n - 1) + 1;
  d = pollard_rho_step(n, seed, c);
}
ah_runtime_error_if(d == n) << "pollard_rho: could not find factor after 100 attempts";
```

---

## Bug 11: `ntt.H:94` — `transform` no valida potencia de 2

```cpp
template <typename T>
void transform(DynArray<T> & a, bool inverse)
{
  int n = a.size();
  // ← Sin validación de que n sea potencia de 2
  // NTT produce resultados incorrectos si n no es potencia de 2
}
```

`multiply` lo cumple, pero `transform` es público.

**Propuesta**:
```cpp
void transform(DynArray<T> & a, bool inverse)
{
  const size_t n = a.size();
  ah_domain_error_if((n & (n - 1)) != 0)
    << "NTT requires array size to be a power of 2, got " << n;
  // ...
}
```

---

## Bug 12: `tpl_sort_utils.H:5461` — `bucket_sort` con 0 buckets es no-op silencioso

```cpp
void bucket_sort_impl(T *a, const size_t n, const size_t num_buckets, ...)
{
  if (n < 2)
    return;
  // ← Sin validación de num_buckets

  // Código...
}

// Y en bucket_sort_detail la función retorna inmediatamente si num_buckets == 0
```

Con 0 buckets, la función retorna silenciosamente sin ordenar. Debería ser error.

**Propuesta**:
```cpp
ah_domain_error_if(num_buckets == 0)
  << "bucket_sort: num_buckets must be > 0";
```

---

# 3. VIOLACIONES DE CONVENCIÓN

## Convención 1: `# include` con espacio — SISTEMÁTICA en 6 archivos

**Archivos afectados**:
- `modular_arithmetic.H` (líneas 41-49)
- `modular_combinatorics.H` (líneas 40-46)
- `modular_linalg.H` (líneas 41-51)
- `ntt.H` (líneas 41-47)
- `pollard_rho.H` (líneas 43-53)
- `primality.H` (líneas 40-45)
- `Tests/math_nt_test.cc` (líneas 36-43)

**Actual**:
```cpp
#include <cstdint>
#ifndef MODULAR_ARITHMETIC_H
#define MODULAR_ARITHMETIC_H
```

**Debe ser**:
```cpp
# include <cstdint>
# ifndef MODULAR_ARITHMETIC_H
# define MODULAR_ARITHMETIC_H
```

**Solución**: Buscar y reemplazar `#include` → `# include`, `#ifndef` → `# ifndef`, `#define` → `# define` en los 7 archivos.

---

## Convención 2: `"..."` en vez de `<...>` para includes internos

**Archivos afectados**:
- `modular_combinatorics.H:46` — `#include "modular_arithmetic.H"`
- `modular_linalg.H:51` — `#include "modular_arithmetic.H"`
- `ntt.H:47` — `#include "modular_arithmetic.H"`
- `pollard_rho.H:52,53` — `#include "primality.H"`, `"modular_arithmetic.H"`

**Debe ser**: `# include <modular_arithmetic.H>`

---

## Convención 3: `||` y `!` en vez de `or` y `not`

| Archivo | Línea | Actual | Debe ser |
|---------|-------|--------|----------|
| `primality.H` | 63 | `if (x == 1 \|\| x == n - 1)` | `if (x == 1 or x == n - 1)` |
| `primality.H` | 86 | `if (n == 2 \|\| n == 3)` | `if (n == 2 or n == 3)` |
| `ntt.H` | 138 | `if (a.is_empty() \|\| b.is_empty())` | `if (a.is_empty() or b.is_empty())` |
| `count-min-sketch.H` | 109 | `(delta <= 0.0 \|\| delta >= 1.0)` | `(delta <= 0.0 or delta >= 1.0)` |
| `Tests/ah-comb.cc` | 59 | `if (!first)` | `if (not first)` |

---

## Convención 4: Indentación en `Tests/math_nt_test.cc`

**Problema**: Todo el archivo usa 4 espacios. La convención Aleph-w es 2 espacios.

```cpp
TEST(MathNT, ModMulAndExp)
{
    uint64_t m = 1000000007;  // ← 4 espacios
    EXPECT_EQ(mod_mul(2, 3, m), 6);
```

**Debe ser**:
```cpp
TEST(MathNT, ModMulAndExp)
{
  uint64_t m = 1000000007;  // ← 2 espacios
  EXPECT_EQ(mod_mul(2, 3, m), 6);
```

---

## Convención 5: `[[nodiscard]]` faltante en funciones puras

Funciones que no tienen efectos secundarios y cuyo valor es importante:
- `mod_mul` (línea 64)
- `mod_exp` (línea 95)
- `ext_gcd` (línea 120)
- `mod_inv` (línea 146)
- `crt` (línea 165)
- `check_composite` (primality.H:60)
- `miller_rabin` (primality.H:83)
- `pollard_rho` (pollard_rho.H:131)
- `find_any_factor` (pollard_rho.H:89)
- `extract_prime_factors` (pollard_rho.H:109)
- `multiply` (ntt.H:136)
- `HyperLogLog::estimate()` (ya lo tiene, bien)
- Y más...

**Propuesta**: Agregar `[[nodiscard]]` antes de `uint64_t` o `T` en todas estas.

---

## Convención 6: Includes innecesarios

| Archivo | Línea | Include | Uso |
|---------|-------|---------|-----|
| `modular_arithmetic.H` | 45 | `<stdexcept>` | No se usa (se usan macros `ah_*`) |
| `modular_arithmetic.H` | 46 | `<utility>` | No se usa (`std::pair` no aparece) |
| `modular_linalg.H` | 46 | `<stdexcept>` | No se usa (se usan macros `ah_*`) |
| `pollard_rho.H` | 48 | `<algorithm>` | No se usa (`std::swap` no aparece) |
| `count-min-sketch.H` | 47 | `<vector>` | No se usa |

**Propuesta**: Eliminar estos includes.

---

## Convención 7: `constexpr` — Oportunidades perdidas

Funciones que deberían ser `constexpr`:
- `mod_mul(uint64_t, uint64_t, uint64_t)` — sólo la rama sin `__uint128_t`
- `mod_exp` — con constexpr en bucle
- `ext_gcd` — recursión constexpr viable
- `miller_rabin` — bucle over base list
- `compute_minrun` (timsort) — ya lo tiene ✓

**Propuesta**:
```cpp
[[nodiscard]] constexpr uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m) {
  #ifdef __SIZEOF_INT128__
    return ((__uint128_t)a * b) % m;
  #else
    // ... constexpr fallback
  #endif
}
```

---

# 4. PROBLEMAS DE DISEÑO

## Diseño 1: `modular_linalg.H` — Lambdas duplicadas

Las funciones `get_val`, `set_val`, `swap_rows` se definen idénticamente en:
- `determinant()` (líneas 131-158)
- `inverse()` (líneas 244-271)

**Propuesta**: Refactorizar como funciones privadas:
```cpp
class Modular_Matrix {
private:
  T get_val(const MatrixT & mat, size_t i, size_t j) const { ... }
  void set_val(MatrixT & mat, size_t i, size_t j, T val) { ... }
  void swap_rows(MatrixT & mat, size_t i, size_t j) { ... }

public:
  uint64_t determinant() {
    // usa this->get_val, this->set_val, etc.
  }
};
```

---

## Diseño 2: `reservoir-sampling.H:79` — Seed con granularidad de segundo

```cpp
const unsigned long seed = static_cast<unsigned long>(std::time(nullptr));
```

Dos `Reservoir_Sampler` creados en el mismo segundo dan resultados idénticos.

**Propuesta**:
```cpp
#include <chrono>

Reservoir_Sampler(size_t k, unsigned long seed = 0)
  : k_(k) {
  if (seed == 0) {
    seed = static_cast<unsigned long>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  }
  // ...
}
```

---

## Diseño 3: `hyperloglog.H:133` — `m_ * m_` puede overflow en 32-bit

```cpp
double e = alpha_m_ * static_cast<double>(m_ * m_) / z;
```

En 32-bit, `size_t m_ = 65536` → `m_ * m_ = 4294967296` overflows.

**Propuesta**:
```cpp
double e = alpha_m_ * (static_cast<double>(m_) * static_cast<double>(m_)) / z;
```

---

## Diseño 4: `pollard_rho.H:95` — Seed fijo en RNG

```cpp
std::mt19937_64 rng(1337);  // ← Fixed seed
```

Cada llamada usa la misma secuencia de números random, reduciendo efectividad.

**Propuesta**:
```cpp
std::random_device rd;
std::mt19937_64 rng(rd());
// O al menos:
std::mt19937_64 rng(1337 + reinterpret_cast<uintptr_t>(n));
```

---

## Diseño 5: Constructores con copy-assign innecesario

### En `simhash.H:72-75`:
```cpp
SimHash() {
  v_ = Array<double>(64, 0.0);  // Crea temporal, luego copy-assign
}
```

**Debe ser**:
```cpp
SimHash() : v_(64, 0.0) {}  // Member initializer list
```

### En `minhash.H:79-80` (similar):
```cpp
MinHash(size_t num_hashes)
  : num_hashes_(num_hashes) {
  signature_ = Array<uint64_t>(num_hashes, std::numeric_limits<uint64_t>::max());
}
```

**Debe ser**:
```cpp
MinHash(size_t num_hashes)
  : num_hashes_(num_hashes),
    signature_(num_hashes, std::numeric_limits<uint64_t>::max()) {}
```

---

## Diseño 6: `modular_linalg.H` — Sin multiplicación de matrices

La clase `Modular_Matrix<MatrixT>` tiene determinante e inversa, pero no multiplicación. Para una clase de álgebra lineal, esto es una característica fundamental faltante.

**Propuesta**: Agregar:
```cpp
Modular_Matrix operator*(const Modular_Matrix & other) const {
  ah_domain_error_if(n_cols != other.n_rows)
    << "matrix multiply: dimension mismatch";
  // ... multiplicación estándar modular
}
```

---

## Diseño 7: `minhash.H` — Sin método `merge()`

A diferencia de `Count_Min_Sketch` y `HyperLogLog` que tienen `merge()`, `MinHash` no. Útil para combinar signatures de diferentes shards.

**Propuesta**:
```cpp
MinHash & merge(const MinHash & other) {
  ah_domain_error_if(num_hashes_ != other.num_hashes_)
    << "MinHash merge: hash count mismatch";
  for (size_t i = 0; i < num_hashes_; ++i)
    signature_(i) = std::min(signature_(i), other.signature_(i));
  return *this;
}
```

---

# 5. PROBLEMAS EN tpl_sort_utils.H (Bucket Sort y Timsort)

## Problema 1: Bloques Doxygen duplicados

Hay pares de bloques Doxygen consecutivos. El primero (breve) debe eliminarse:

| Función | Líneas | Acción |
|---------|--------|--------|
| `gallop_left` | 5874-5878 (eliminar) + 5880-5896 (mantener) | Eliminar el primero |
| `gallop_right` | 5946-5948 (eliminar) + 5949-5963 (mantener) | Eliminar el primero |
| `timsort_impl` | 6168-6178 (eliminar) + 6180-6193 (mantener) | Eliminar el primero |

---

## Problema 2: Aritmética unsigned frágil en `gallop_left`/`gallop_right`

```cpp
// Línea 5928 en gallop_left:
last_ofs = hint - ofs;  // ← Puede ser SIZE_MAX por underflow unsigned
```

Esto funciona correctamente debido a aritmética modular unsigned (el siguiente `++last_ofs` wraps de SIZE_MAX a 0), pero es frágil.

**Propuesta**: Documentar con comentario:
```cpp
// NOTE: This underflow is intentional. When ofs > hint, computing
// hint - ofs wraps to SIZE_MAX, and the next increment wraps back to
// the correct value (hint - ofs + 1). This is guaranteed to work due
// to unsigned arithmetic modular properties.
last_ofs = hint - ofs;
```

---

## Problema 3: Lambda en `bucket_sort` captura innecesariamente valores

```cpp
auto key = [min_val, max_val, range, num_buckets, cmp](const T & val) -> size_t {
  // ...
  if (not cmp(min_val, max_val)) // Descending order check
    b = (num_buckets - 1) - b;
  return b;
};
```

**Problemas**:
1. `max_val` se captura pero sólo se usa para esta comparación
2. La detección de orden descendente es frágil (¿y si `cmp` no es `std::less`?)

**Propuesta**:
```cpp
const bool descending = not cmp(min_val, max_val);

auto key = [min_val, range, num_buckets, descending](const T & val) -> size_t {
  auto b = static_cast<size_t>((val - min_val) / range *
                                static_cast<T>(num_buckets - 1));
  return (b >= num_buckets) ? (num_buckets - 1) :
         (descending ? (num_buckets - 1) - b : b);
};
```

---

# 6. COBERTURA DE TESTS — GAPS IMPORTANTES

## Test Gap 1: `mod_exp(base, 0, m > 1)`

```cpp
// Tests/math_nt_test.cc, línea 55:
EXPECT_EQ(mod_exp(2, 0, 1), 0);  // Correcto: 2^0 mod 1 = 0

// Falta:
EXPECT_EQ(mod_exp(2, 0, 7), 1);  // 2^0 mod 7 debería ser 1
EXPECT_EQ(mod_exp(5, 0, 11), 1); // Más casos
```

---

## Test Gap 2: `ext_gcd` con edge cases

```cpp
// Falta en math_nt_test.cc:
// ext_gcd(0, n, x, y) → debería retornar n con x=0, y=1
// ext_gcd(n, 0, x, y) → debería retornar n con x=1, y=0
// ext_gcd(0, 0, x, y) → behavior?
```

---

## Test Gap 3: `pollard_rho` con números pequeños

```cpp
// Falta en math_nt_test.cc:
EXPECT_EQ(pollard_rho(0).size(), 0);  // No factors
EXPECT_EQ(pollard_rho(1).size(), 0);  // No factors
EXPECT_EQ(pollard_rho(2).size(), 1);  // [2]
EXPECT_EQ(pollard_rho(4).size(), 2);  // [2, 2]
```

---

## Test Gap 4: CRT con casos especiales

```cpp
// Tests/math_nt_test.cc tiene sólo un caso. Falta:
// - CRT con módulos no coprimos (debería error o documentarse)
// - CRT con array vacío
// - CRT con un solo elemento
```

---

## Test Gap 5: Combinatorios — edge cases

### `next_permutation` en ah-comb.cc:
```cpp
// Falta:
DynList<int> empty;
EXPECT_FALSE(next_permutation(empty));  // Empty array

DynList<int> single = {5};
EXPECT_FALSE(next_permutation(single));  // Size 1
```

### `next_combination_indices`:
```cpp
// Falta:
DynArray<size_t> indices;
EXPECT_FALSE(next_combination_indices(indices, 10, 0));  // k == 0
```

---

## Test Gap 6: NTT `transform` con non-POT size

```cpp
// Falta en streaming_algorithms_test.cc (o nuevo test):
DynArray<uint64_t> a = {1, 2, 3};  // Size 3 (not power of 2)
EXPECT_THROW(transform(a, false), std::domain_error);
```

---

## Test Gap 7: Bucket sort con 0 buckets

```cpp
// sort_utils.cc test 'zero_buckets' sólo verifica NO_THROW
// Debería:
// 1. O documentar que es no-op (no ordena)
// 2. O convertir a error (lo recomendado)

// Ahora sólo:
EXPECT_NO_THROW(bucket_sort(a, n, 0, key));

// Debería ser:
EXPECT_THROW(bucket_sort(a, n, 0, key), std::domain_error);
```

---

## Test Gap 8: Streaming — streams vacíos

```cpp
// Tests/streaming_algorithms_test.cc falta:

// HyperLogLog con 0 elementos
HyperLogLog hll(10);
EXPECT_EQ(hll.estimate(), 0.0);  // ← Puede ser indefinido

// MinHash con 0 elementos
MinHash mh(64);
// ¿Cuál es el fingerprint de un conjunto vacío?

// Reservoir sampler con k > items
Reservoir_Sampler<int> rs(100);
rs.update(1);  // Sólo 1 elemento
auto sample = rs.get_sample();
EXPECT_EQ(sample.size(), 1);  // O 100? Documentar
```

---

## Test Gap 9: Typo en nombres de test

**Tests/ah-comb.cc**, líneas 726-1081: Múltiples tests nombrados `Nodescard*` cuando deberían ser `Nodiscard*`:

```cpp
TEST(Combinatorics, NodesCardBuildPerms)  // ← Typo "Nodescard"
TEST(Combinatorics, NodiscardBuildCombs)  // ← Correcto
```

**Propuesta**: Buscar y reemplazar `Nodescard` → `Nodiscard` en todas las líneas.

---

# 7. RESUMEN DE PRIORIDADES

## 🔴 CRÍTICO (Debe corregirse antes de merge)

1. **Bug #1-2**: Cast `uint64_t` → `int64_t` en `mod_inv` (UB real)
   - Archivos: `modular_arithmetic.H:149`, `modular_linalg.H:181,293`
   - Estimación: 30 min

2. **Bug #3**: `mod_mul`/`mod_exp` con `m == 0`
   - Archivo: `modular_arithmetic.H:64,95`
   - Estimación: 15 min

3. **Bug #4**: `int n = a.size()` en `ntt.H`
   - Archivos: `ntt.H:72,94` y referencias
   - Estimación: 45 min (cambio sistemático)

4. **Bug #5**: `Array<T>::operator!=` inexistente
   - Archivo: `count-min-sketch.H:153`
   - Estimación: 20 min (comparación element-wise)

5. **Convención #1**: `# include` con espacio — 7 archivos
   - Archivos: Todos los de math/NT + test
   - Estimación: 10 min (buscar/reemplazar global)

6. **Convención #2**: `<...>` en lugar de `"..."` para includes internos
   - Archivos: 5 archivos de math/NT
   - Estimación: 5 min (buscar/reemplazar)

7. **Convención #16**: Indentación en `math_nt_test.cc`
   - Archivo: `Tests/math_nt_test.cc` (completo)
   - Estimación: 10 min (reindent global)

**Tiempo total crítico**: ~2 horas

---

## 🟠 IMPORTANTE (Debería corregirse)

8. **Bugs moderados** (#6-7, #9-12): 6 bugs
   - Estimación: 2 horas

9. **`[[nodiscard]]`** en funciones puras
   - Estimación: 30 min

10. **Includes innecesarios**
    - Estimación: 10 min

11. **Convención #3**: `||` → `or`, `!` → `not` (4 casos)
    - Estimación: 5 min

---

## 🟡 DESEABLE (Mejoras de calidad)

12. **Diseño #1-7**: Refactorización y mejoras arquitectónicas
    - Estimación: 4-5 horas

13. **`constexpr`** en funciones matemáticas
    - Estimación: 1 hora

14. **Doxygen duplicados** en tpl_sort_utils.H
    - Estimación: 15 min

15. **Cobertura de tests** — 9 gaps
    - Estimación: 2-3 horas

---

## Tiempo total de remediar TODO

- **Crítico + Importante**: ~4 horas
- **+ Deseable**: ~10-12 horas

---

# 8. CHECKLIST DE CORRECCIONES

### Fase 1: Crítico (must-have para merge)

- [ ] Bug #1: `modular_arithmetic.H:149` — remover cast `int64_t`
- [ ] Bug #2: `modular_linalg.H:181,293` — remover casts `int64_t` innecesarios
- [ ] Bug #3: `modular_arithmetic.H:64,95` — agregar `ah_domain_error_if(m == 0)`
- [ ] Bug #4: `ntt.H` — cambiar `int n` a `size_t n` en todo el archivo
- [ ] Bug #5: `count-min-sketch.H:153` — implementar comparación element-wise
- [ ] Conv #1: Global search-replace `#include` → `# include` (7 archivos)
- [ ] Conv #2: Global search-replace `"*.H"` → `<*.H>` en includes (5 archivos)
- [ ] Conv #16: Reindent `Tests/math_nt_test.cc` de 4 a 2 espacios

### Fase 2: Importante

- [ ] Bug #6-7, #9-12: Implementar validaciones y correcciones
- [ ] Agregar `[[nodiscard]]` en ~20 funciones
- [ ] Remover includes innecesarios (5 casos)
- [ ] Conv #3: `||` → `or`, `!` → `not` (4 casos)

### Fase 3: Deseable

- [ ] Refactorización de lambdas en `modular_linalg.H`
- [ ] Agregar `constexpr` en funciones matemáticas
- [ ] Remover Doxygen duplicados en `tpl_sort_utils.H`
- [ ] Agregar cobertura de tests (15+ casos nuevos)
- [ ] Agregar `merge()` a `MinHash`
- [ ] Documentar aritmética unsigned en gallop functions

---

# Apéndice: Archivos afectados por categoría

## Por tipo de problema

### Bugs críticos
- `modular_arithmetic.H` (3 bugs)
- `modular_linalg.H` (1 bug)
- `ntt.H` (2 bugs)
- `count-min-sketch.H` (1 bug)

### Convenciones
- `modular_arithmetic.H`, `modular_combinatorics.H`, `modular_linalg.H`, `ntt.H`, `pollard_rho.H`, `primality.H` (6 archivos × violaciones múltiples)
- `Tests/math_nt_test.cc` (indentación + includes)
- `Tests/ah-comb.cc` (uses `!`, typos)

### Diseño
- `modular_linalg.H` (duplicación de lambdas, sin multiplicación)
- `reservoir-sampling.H` (seed, clear())
- `pollard_rho.H` (seed, infinidad)
- `simhash.H`, `minhash.H` (constructores)
- `hyperloglog.H` (overflow)
- `minhash.H` (sin merge)

### Tests
- `Tests/sort_utils.cc` (bucket_sort gaps)
- `Tests/math_nt_test.cc` (multiple gaps)
- `Tests/ah-comb.cc` (edge cases, typos)
- `Tests/streaming_algorithms_test.cc` (gaps)

---

**Fin del análisis**. Este documento es la referencia completa para correerciones y mejoras de la rama `utils`.
