# Plan: NTT Industrial — Llevar `ntt.H` al nivel de `fft.H`

## Contexto: Relación entre `fft.H` y `ntt.H`

### El mismo algoritmo, dos dominios aritméticos

`fft.H` y `ntt.H` implementan **exactamente la misma estructura algorítmica**
(butterfly de Cooley-Tukey), pero operan en dominios algebraicos distintos:

| Aspecto | `fft.H` | `ntt.H` |
|---------|---------|---------|
| **Dominio** | `std::complex<Real>` (punto flotante) | `uint64_t` (aritmética modular `Z/pZ`) |
| **Raíz N-ésima de la unidad** | `exp(-2πi/N)` | `g^{(p-1)/N} mod p` (raíz primitiva) |
| **Resultado** | Aproximado (error de redondeo) | Exacto (aritmética entera) |
| **Tamaño** | Cualquier N (Bluestein para no potencia de 2) | Solo potencias de 2 que dividan `p-1` |
| **Caso de uso principal** | DSP, señales, filtros, espectros | Multiplicación exacta de polinomios mod p, competencias, criptografía |
| **Líneas de código** | ~15,100 | ~210 |

### Analogía algebraica

La FFT opera en el grupo multiplicativo de las raíces complejas de la unidad.
La NTT opera en el grupo multiplicativo de `Z/pZ` donde `p` es primo y
`p - 1` tiene una potencia de 2 grande como factor (para que existan raíces
N-ésimas de la unidad en el campo).

La FFT **necesita** punto flotante y sufre errores de redondeo.
La NTT produce resultados **exactos** pero los coeficientes están acotados por `p`.

### Qué tiene `fft.H` que `ntt.H` no tiene

| Capacidad de `fft.H` | Equivalente NTT necesario |
|----------------------|--------------------------|
| `Plan` (precomputa twiddle factors) | `NTTPlan` (precomputa raíces primitivas y bit-reversal) |
| Bluestein para tamaños arbitrarios | Bluestein modular o Rader |
| Mixed radix (2,3,5) | Mixed radix modular |
| SIMD (AVX2/NEON) | SIMD con Montgomery vectorizado |
| `ThreadPool` paralelo (`ptransform`, etc.) | `ptransform`, `pmultiply` |
| Batch transforms | Batch NTT |
| N-dimensional (2D, 3D) | N-dimensional modular |
| Streaming (OverlapAdd, etc.) | No aplica directamente (NTT no es para señales) |
| Filtros FIR/IIR, Welch, STFT | No aplica (dominio DSP exclusivo de FFT) |
| `rfft`/`irfft` (real optimizado) | No aplica (NTT no tiene simetría hermitiana) |

### Qué necesita `ntt.H` que **no** tiene analogía en `fft.H`

| Capacidad exclusiva de NTT | Razón |
|---------------------------|-------|
| CRT multi-primo | Reconstruir coeficientes exactos > p |
| Montgomery/Barrett reduction | Multiplicación modular rápida en inner loop |
| Selección automática de primo | Elegir p óptimo según tamaño y rango de coeficientes |
| Convolución negacíclica | Schönhage-Strassen para enteros grandes |
| Álgebra de polinomios mod p | Inversión, división, composición, evaluación multi-punto |
| Splitting de enteros grandes | Multiplicar enteros de millones de dígitos |

---

## Estado actual de `ntt.H` (212 líneas)

```
API pública:
  static void transform(Array<uint64_t>& a, bool invert)
  static Array<uint64_t> multiply(Array<uint64_t> const& a, Array<uint64_t> const& b)

Limitaciones:
  - Solo potencias de 2
  - Un solo primo por defecto (998244353)
  - Sin Plan reutilizable
  - Sin SIMD
  - Sin paralelismo
  - Sin CRT multi-primo
  - mod_mul genérico (no Montgomery)
  - Sin batch, sin streaming, sin N-dimensional
```

---

## Infraestructura disponible en el codebase

| Componente | Archivo | Lo que provee |
|-----------|---------|--------------|
| Aritmética modular | `modular_arithmetic.H` | `mod_mul`, `mod_exp`, `mod_inv`, `ext_gcd`, `crt` |
| ThreadPool | `thread_pool.H` | Pool de hilos con `enqueue`, `wait_all`, estadísticas |
| Parallel patterns | `ah-parallel.H` | `pmaps`, `pfilter`, `pfoldl`, `pfor_each` |
| Array dinámico | `tpl_array.H` | `Array<T>` con CRTP mixins |
| Error macros | `ah-errors.H` | `ah_invalid_argument_if`, `ah_domain_error_if`, etc. |
| Primalidad | `primality.H` | `miller_rabin()` determinístico 64 bits |
| Factorización | `pollard_rho.H` | `pollard_rho()` |

---

## Plan de implementación

### Convenciones obligatorias

Antes de empezar, todo código generado **debe** seguir estas reglas (ver `CLAUDE.md`):

- **Namespace**: `Aleph`
- **Contenedores**: `Array<T>` (jamás `std::vector`)
- **Errores**: Solo macros `ah_*_error_*` (jamás `throw` directo)
- **Estilo**: 2 espacios, K&R braces, `# include` con espacio, `and`/`or`/`not`
- **Atributos**: `[[nodiscard]]`, `constexpr`, `noexcept` donde aplique
- **Functools**: `Aleph::less<T>`, `Aleph::plus<T>` (jamás `std::less`)
- **Documentación**: Doxygen completo (`@brief`, `@param`, `@return`, `@throws`, `@note` para complejidad)
- **Licencia**: MIT, 2002-2026, Leandro Rabindranath Leon
- **Header guard**: `# ifndef NTT_H` (ya existe)
- **Tests**: Google Test en `Tests/`, fixture con `::testing::Test`

---

### Fase 0: Montgomery Reduction en `modular_arithmetic.H`

**Objetivo**: Proveer multiplicación modular 3-5x más rápida que `mod_mul` para
uso repetido con el mismo módulo.

**Qué implementar**:

```cpp
// En modular_arithmetic.H (o un nuevo ah-montgomery.H)

/// Precomputed Montgomery context for a fixed odd modulus.
struct MontgomeryCtx
{
  uint64_t mod;        // The modulus p
  uint64_t mod2;       // 2*p (for conditional subtraction)
  uint64_t r;          // R = 2^64 mod p
  uint64_t r2;         // R^2 mod p  (for lifting into Montgomery form)
  uint64_t mod_inv_neg; // -p^{-1} mod 2^64  (for REDC)
};

[[nodiscard]] constexpr MontgomeryCtx montgomery_ctx(uint64_t mod);
[[nodiscard]] constexpr uint64_t mont_reduce(uint128 x, MontgomeryCtx const& ctx);
[[nodiscard]] constexpr uint64_t mont_mul(uint64_t a, uint64_t b, MontgomeryCtx const& ctx);
[[nodiscard]] constexpr uint64_t mont_exp(uint64_t base, uint64_t exp, MontgomeryCtx const& ctx);
[[nodiscard]] constexpr uint64_t to_mont(uint64_t a, MontgomeryCtx const& ctx);
[[nodiscard]] constexpr uint64_t from_mont(uint64_t a, MontgomeryCtx const& ctx);
```

**Justificación**: El butterfly de la NTT ejecuta una `mod_mul` por cada
mariposa. Con Montgomery, la reducción pasa de división (costosa) a
multiplicación + shift (barata). Esto es la optimización de mayor impacto.

**Dependencias**: Ninguna nueva. Usa `__uint128_t` que `modular_arithmetic.H`
ya usa.

**Tests** (`Tests/montgomery_test.cc`):
- Round-trip: `from_mont(to_mont(a)) == a % mod`
- Multiplicación: `from_mont(mont_mul(to_mont(a), to_mont(b))) == (a*b) % mod`
- Exponenciación: `from_mont(mont_exp(to_mont(base), exp)) == mod_exp(base, exp, mod)`
- Primos NTT-friendly: 998244353, 469762049, 1004535809
- Casos límite: `a = 0`, `a = mod - 1`, `a * b` cerca de overflow

---

### Fase 1: NTT Core refactorizado

**Objetivo**: Refactorizar `ntt.H` a la estructura de `fft.H`: clase template
con métodos estáticos, Plan reutilizable, Montgomery como aritmética interna.

**Archivo**: `ntt.H` (reescritura)

#### 1.1 Estructura de la clase

```cpp
template <uint64_t MOD = 998244353, uint64_t ROOT = 3>
class NTT
{
public:
  // --- Tipos ---
  static constexpr uint64_t mod = MOD;
  static constexpr uint64_t root = ROOT;

  // --- Transform core ---
  static void transform(Array<uint64_t>& a, bool invert);
  static Array<uint64_t> transformed(Array<uint64_t> const& a, bool invert);

  // --- Convolution / multiply ---
  static Array<uint64_t> multiply(Array<uint64_t> const& a,
                                  Array<uint64_t> const& b);
  static void multiply_inplace(Array<uint64_t>& a,
                                Array<uint64_t> const& b);

  // --- Batch ---
  static void transform_batch(Array<Array<uint64_t>>& batch, bool invert);
  static Array<Array<uint64_t>> transformed_batch(
    Array<Array<uint64_t>> const& batch, bool invert);

  // --- Parallel (ThreadPool) ---
  static void ptransform(ThreadPool& pool, Array<uint64_t>& a,
                          bool invert, size_t chunk_size = 0);
  static Array<uint64_t> ptransformed(ThreadPool& pool,
                                       Array<uint64_t> const& a,
                                       bool invert, size_t chunk_size = 0);
  static Array<uint64_t> pmultiply(ThreadPool& pool,
                                    Array<uint64_t> const& a,
                                    Array<uint64_t> const& b,
                                    size_t chunk_size = 0);
  static void ptransform_batch(ThreadPool& pool,
                                Array<Array<uint64_t>>& batch,
                                bool invert, size_t chunk_size = 0);

  // --- Utilidades ---
  [[nodiscard]] static constexpr uint64_t max_transform_size() noexcept;
  [[nodiscard]] static constexpr bool supports_size(size_t n) noexcept;
  [[nodiscard]] static constexpr uint64_t primitive_root_of_unity(size_t n);

  // --- Plan reutilizable ---
  class Plan;

private:
  static constexpr MontgomeryCtx mctx_ = montgomery_ctx(MOD);
  static void bit_reverse(Array<uint64_t>& a);
  // ... helpers internos ...
};
```

#### 1.2 Clase `NTT::Plan`

Análoga a `FFT::Plan`. Precomputa todo lo reutilizable para un tamaño fijo:

```cpp
class Plan
{
public:
  explicit Plan(size_t n);

  void transform(Array<uint64_t>& a, bool invert) const;
  Array<uint64_t> transformed(Array<uint64_t> const& a, bool invert) const;

  Array<uint64_t> multiply(Array<uint64_t> const& a,
                            Array<uint64_t> const& b) const;

  // Parallel
  void ptransform(ThreadPool& pool, Array<uint64_t>& a,
                   bool invert, size_t chunk_size = 0) const;
  Array<uint64_t> pmultiply(ThreadPool& pool,
                             Array<uint64_t> const& a,
                             Array<uint64_t> const& b,
                             size_t chunk_size = 0) const;

  // Batch
  void transform_batch(Array<Array<uint64_t>>& batch, bool invert) const;
  void ptransform_batch(ThreadPool& pool,
                         Array<Array<uint64_t>>& batch,
                         bool invert, size_t chunk_size = 0) const;

  [[nodiscard]] size_t size() const noexcept;

private:
  size_t n_;
  size_t log_n_;
  Array<size_t> bit_rev_;         // permutación bit-reversal
  Array<uint64_t> twiddles_fwd_;  // raíces en forma Montgomery
  Array<uint64_t> twiddles_inv_;  // raíces inversas en forma Montgomery
  uint64_t inv_n_;                // N^{-1} mod p en forma Montgomery
};
```

**Estado precomputado** (todo en forma Montgomery para evitar conversiones en el inner loop):
- `bit_rev_[i]` — permutación de índices para reordenar antes del butterfly
- `twiddles_fwd_[stage][j]` — `to_mont(g^{j*(p-1)/len})` para cada stage
- `twiddles_inv_[stage][j]` — inversas de las twiddles
- `inv_n_` — `to_mont(mod_inv(n, p))` para la normalización de la INTT

**Costo de memoria**: O(N) enteros de 64 bits.

#### 1.3 Butterfly con Montgomery

El inner loop actual:
```cpp
// ACTUAL (lento)
const uint64_t u = a[i + j];
const uint64_t v = mod_mul(a[i + j + len/2], w, MOD);
a(i + j) = (u + v) % MOD;
a(i + j + len/2) = (u - v + MOD) % MOD;
w = mod_mul(w, wlen, MOD);
```

Debe convertirse a:
```cpp
// NUEVO (rápido, todo en Montgomery form)
const uint64_t u = a[i + j];
const uint64_t v = mont_mul(a[i + j + len/2], w, mctx_);
a(i + j) = u + v < MOD ? u + v : u + v - MOD;         // mod add
a(i + j + len/2) = u >= v ? u - v : u - v + MOD;       // mod sub
w = mont_mul(w, wlen, mctx_);
```

**Nota**: La suma y resta modulares NO necesitan Montgomery (son O(1) con
comparación). Solo la multiplicación se beneficia.

#### 1.4 Tests para Fase 1

**Archivo**: `Tests/ntt_test.cc` (separar de `math_nt_test.cc`)

- Round-trip forward/inverse para N = 1, 2, 4, 8, ..., 2^23
- `multiply` vs multiplicación naive O(n²) para polinomios aleatorios
- `Plan` produce el mismo resultado que los métodos estáticos
- `Plan::transform_batch` consistencia
- Performance regression: N = 2^20 debe completar en < 200ms
- Variantes paralelas producen mismo resultado que secuenciales
- Error paths: N = 0, N no potencia de 2, N > max_transform_size

---

### Fase 2: Tamaños arbitrarios (Bluestein modular)

**Objetivo**: Soportar cualquier N, no solo potencias de 2.

**Algoritmo**: Idéntico al Bluestein de `fft.H`, pero con aritmética modular:

1. Calcular chirp: `chirp[k] = g^{k² * (p-1) / (2N)} mod p`
2. Pad a M = next_power_of_two(2N - 1)
3. Crear kernel como transformada del chirp conjugado
4. Convolución via NTT de tamaño M
5. Extraer y escalar resultado

**Integración con Plan**:
```cpp
// En Plan::constructor:
if (is_power_of_two(n))
  strategy_ = power_of_two;   // Cooley-Tukey directo
else
  strategy_ = bluestein;       // NTT circular de tamaño potencia de 2
```

**Almacenamiento adicional en Plan (Bluestein)**:
- `bluestein_chirp_` — Array<uint64_t> de tamaño N
- `bluestein_kernel_fwd_` — kernel pre-transformado (tamaño M)
- `bluestein_kernel_inv_` — kernel para INTT
- `bluestein_plan_` — shared_ptr<Plan> para la NTT interna de tamaño M

**Tests**:
- Tamaños primos: N = 7, 13, 127, 8191, 65537
- Tamaños compuestos no potencia de 2: N = 6, 10, 12, 15, 100, 1000
- Consistencia: Bluestein(x) == zero-pad-to-power-of-2(x) para datos que caben
- multiply con tamaños arbitrarios

---

### Fase 3: CRT multi-primo

**Objetivo**: Multiplicar polinomios con coeficientes arbitrariamente grandes
sin reducción a un solo primo.

**Primos NTT-friendly predefinidos**:

```cpp
// Todos con raíz primitiva 3 y soporte para N hasta 2^23
struct NTTPrime
{
  uint64_t mod;
  uint64_t root;
  uint64_t max_power_of_two;  // mayor k tal que 2^k | (mod - 1)
};

// Primos estándar (caben en 30 bits, producto cabe en 90 bits → uint128)
static constexpr NTTPrime primes[] = {
  {998244353,  3, 23},  // 2^23 * 119 + 1
  {469762049,  3, 26},  // 2^26 * 7 + 1
  {1004535809, 3, 21},  // 2^21 * 479 + 1
};
```

**API pública**:

```cpp
// En ntt.H o un nuevo ntt_crt.H

/// Multiply two polynomials with arbitrary uint64_t coefficients.
/// Uses 3-prime NTT + CRT to reconstruct exact coefficients.
/// Result coefficients can be up to ~ 2^90 (returned as __uint128_t or
/// two uint64_t limbs).
///
/// @param a First polynomial (coefficients in [0, 2^64)).
/// @param b Second polynomial (coefficients in [0, 2^64)).
/// @return Exact product polynomial.
/// @note Assumes max coefficient of product < p1*p2*p3 ≈ 2^{89.6}.
///       For coefficients that may exceed this, use the bigint variant.
static Array<uint64_t> multiply_exact(Array<uint64_t> const& a,
                                       Array<uint64_t> const& b);

/// Parallel multi-prime multiply.
static Array<uint64_t> pmultiply_exact(ThreadPool& pool,
                                        Array<uint64_t> const& a,
                                        Array<uint64_t> const& b,
                                        size_t chunk_size = 0);
```

**Algoritmo**:
1. Computar `c1 = NTT<P1>::multiply(a mod P1, b mod P1)`
2. Computar `c2 = NTT<P2>::multiply(a mod P2, b mod P2)`
3. Computar `c3 = NTT<P3>::multiply(a mod P3, b mod P3)`
4. Para cada coeficiente k, reconstruir `c[k]` via CRT de `(c1[k], c2[k], c3[k])`

**Nota**: Los tres NTTs son independientes → paralelizables trivialmente con
ThreadPool (3 tareas).

**Tests**:
- Multiplicar polinomios con coeficientes > 998244353
- Verificar contra multiplicación naive para polinomios pequeños
- Coeficientes cercanos a 2^63
- Consistencia: `multiply_exact(a, b) mod P1 == NTT<P1>::multiply(a%P1, b%P1)`

---

### Fase 4: SIMD

**Objetivo**: Vectorizar el butterfly con AVX2 (x86-64) y NEON (ARM).

**Estrategia**: Procesar 4 butterflies en paralelo (AVX2 opera en 4×uint64_t).

#### 4.1 AVX2 (x86-64)

```cpp
// 4 butterflies simultáneas con Montgomery multiplication vectorizada
ALEPH_NTT_AVX2_TARGET
static void butterfly_avx2(uint64_t* a, size_t len, size_t n,
                            uint64_t const* twiddles,
                            MontgomeryCtx const& ctx)
{
  // __m256i opera en 4 × uint64_t
  // Montgomery reduction vectorizada requiere:
  //   1. Multiplicación 64×64 → 128 bits (emulada con _mm256_mul_epu32 + shifts)
  //   2. Reducción con ctx.mod_inv_neg
  // Fallback escalar para residuos
}
```

**Nota crítica**: AVX2 no tiene multiplicación nativa 64×64→128. Hay dos
opciones:
- **Opción A**: Usar módulos < 2^31 y trabajar con `_mm256_mullo_epi32` +
  `_mm256_mul_epu32` (rápido, limita a primos < 2^31, suficiente para los
  primos estándar)
- **Opción B**: Emular multiplicación 64-bit con 4 multiplicaciones de 32
  bits (más lento pero general)

**Recomendación**: Opción A para primos NTT estándar (todos < 2^30).

#### 4.2 NEON (ARM64)

```cpp
// 2 butterflies simultáneas (NEON opera en 2 × uint64_t)
static void butterfly_neon(uint64_t* a, size_t len, size_t n,
                            uint64_t const* twiddles,
                            MontgomeryCtx const& ctx)
{
  // uint64x2_t para 2 butterflies
  // vmull_u32 + shift para Montgomery parcial
}
```

#### 4.3 Dispatch (siguiendo el patrón de `fft.H`)

```cpp
// Variables de entorno:
// ALEPH_NTT_SIMD=auto|scalar|avx2|neon

enum class NTTSimdBackend { scalar, avx2, neon };

[[nodiscard]] static NTTSimdBackend simd_backend();
[[nodiscard]] static std::string_view simd_backend_name();
[[nodiscard]] static bool avx2_dispatch_available();
[[nodiscard]] static bool neon_dispatch_available();
```

**Tests**:
- Forzar cada backend via variable de entorno y verificar que los resultados
  sean idénticos al escalar
- Benchmark: AVX2 debe ser >= 2x más rápido que escalar para N >= 2^16
- Verificar que el dispatch automático selecciona el backend correcto

---

### Fase 5: Álgebra de polinomios mod p

**Objetivo**: Suite completa de operaciones sobre polinomios en Z/pZ[x],
todas aceleradas con NTT.

```cpp
// Todos estos son métodos estáticos de NTT<MOD, ROOT>

/// Evaluate polynomial at a single point modulo MOD.
static uint64_t poly_eval(Array<uint64_t> const& coeffs, uint64_t x);

/// Polynomial inverse: find g(x) such that f(x)*g(x) ≡ 1 (mod x^n, mod p).
/// Uses Newton iteration with NTT-accelerated multiply.
/// @pre coeffs[0] must be invertible mod p.
/// @note Time: O(n log n)
static Array<uint64_t> poly_inverse(Array<uint64_t> const& coeffs, size_t n);

/// Polynomial division: quotient and remainder.
/// @return {quotient, remainder}
static std::pair<Array<uint64_t>, Array<uint64_t>>
poly_divmod(Array<uint64_t> const& dividend, Array<uint64_t> const& divisor);

/// Polynomial logarithm: ln(f(x)) mod (x^n, p).
/// @pre coeffs[0] == 1.
static Array<uint64_t> poly_log(Array<uint64_t> const& coeffs, size_t n);

/// Polynomial exponential: exp(f(x)) mod (x^n, p).
/// @pre coeffs[0] == 0.
static Array<uint64_t> poly_exp(Array<uint64_t> const& coeffs, size_t n);

/// Polynomial square root: sqrt(f(x)) mod (x^n, p).
/// @pre coeffs[0] must be a quadratic residue mod p.
static Array<uint64_t> poly_sqrt(Array<uint64_t> const& coeffs, size_t n);

/// Polynomial power: f(x)^k mod (x^n, p).
static Array<uint64_t> poly_power(Array<uint64_t> const& coeffs,
                                   uint64_t k, size_t n);

/// Multi-point evaluation: evaluate f(x) at points[0..m-1].
/// Uses subproduct-tree algorithm.
/// @note Time: O(n log^2 n)
static Array<uint64_t> multipoint_eval(Array<uint64_t> const& coeffs,
                                        Array<uint64_t> const& points);

/// Interpolation: recover f(x) of degree < n from n (point, value) pairs.
/// @note Time: O(n log^2 n)
static Array<uint64_t> interpolate(Array<uint64_t> const& points,
                                    Array<uint64_t> const& values);
```

**Orden de implementación** (cada una depende de las anteriores):
1. `poly_inverse` — base para todo lo demás (Newton iteration)
2. `poly_divmod` — usa inverse + multiply
3. `poly_log` — usa inverse + derivada + integral
4. `poly_exp` — usa log + Newton iteration
5. `poly_sqrt` — usa log + exp (o Newton directo)
6. `poly_power` — usa log + exp
7. `multipoint_eval` — subproduct tree (independiente)
8. `interpolate` — usa multipoint_eval + subproduct tree

**Tests para cada operación**:
- Casos pequeños verificados a mano
- Round-trips: `poly_inverse(f) * f ≡ 1 mod x^n`
- `poly_exp(poly_log(f)) == f` para f con f[0] = 1
- `multipoint_eval(interpolate(pts, vals), pts) == vals`
- Comparación con O(n²) naive para n <= 1000

---

### Fase 6: Multiplicación de enteros grandes (Big Integer Multiply)

**Objetivo**: Multiplicar enteros de millones de dígitos usando NTT.

```cpp
/// Multiply two big integers represented as arrays of base-B digits.
/// Each digit is in [0, B). B must be chosen so that N * B^2 < MOD.
///
/// @tparam Base Numeric base for digit representation (default: 2^15).
/// @param a First integer, a[0] = least significant digit.
/// @param b Second integer, b[0] = least significant digit.
/// @return Product with carries propagated, a.size() + b.size() digits.
template <uint64_t Base = (1ULL << 15)>
static Array<uint64_t> bigint_multiply(Array<uint64_t> const& a,
                                        Array<uint64_t> const& b);

/// Parallel variant.
template <uint64_t Base = (1ULL << 15)>
static Array<uint64_t> pbigint_multiply(ThreadPool& pool,
                                         Array<uint64_t> const& a,
                                         Array<uint64_t> const& b,
                                         size_t chunk_size = 0);
```

**Algoritmo**:
1. Elegir base B tal que `N * B² < p` (para un solo primo) o usar CRT
2. `c = NTT::multiply(a, b)` (sin carry, coeficientes pueden ser > B)
3. Propagar carries: `c[i+1] += c[i] / B; c[i] %= B;`

**Para enteros realmente grandes** (> 2^90 por coeficiente del producto):
usar la variante multi-primo de Fase 3.

**Tests**:
- Multiplicar 10^6 × 10^6 dígitos y comparar con resultado conocido
- Verificar contra multiplicación escolar para números pequeños
- Benchmark vs multiplicación naive

---

### Fase 7: Convolución negacíclica

**Objetivo**: Soporte para convolución negacíclica (útil en Schönhage-Strassen
y en criptografía basada en retículos).

```cpp
/// Negacyclic convolution: c[k] = sum_{i+j≡k (mod N)} a[i]*b[j]
///                               - sum_{i+j≡k+N (mod N)} a[i]*b[j]
/// Equivalent to polynomial multiplication modulo (x^N + 1).
///
/// @pre a.size() == b.size() == N, N must be a power of 2.
static Array<uint64_t> negacyclic_multiply(Array<uint64_t> const& a,
                                            Array<uint64_t> const& b);
```

**Algoritmo**: Twist por raíz 2N-ésima antes de la NTT de tamaño N, luego
untwist después.

**Tests**:
- Verificar que `negacyclic(a, b)` == `(a * b) mod (x^N + 1)` para
  polinomios de grado < N
- Casos: N = 2, 4, 8, 1024

---

### Fase 8: Documentación y tutorial

**Objetivo**: Tutorial análogo a `docs/fft-tutorial.md` pero para NTT.

**Archivos**:
- `docs/ntt-tutorial.md` (español)
- `docs/ntt-tutorial.en.md` (inglés)

**Contenido del tutorial**:
1. Fundamentos: DFT sobre campos finitos
2. Relación FFT vs NTT (tabla comparativa)
3. Arquitectura de `ntt.H`
4. Uso básico: transform, multiply
5. Plan reutilizable
6. CRT multi-primo para coeficientes grandes
7. Álgebra de polinomios mod p
8. Multiplicación de enteros grandes
9. Paralelismo con ThreadPool
10. Control SIMD
11. Ejemplo completo
12. Referencia rápida

**Actualizar READMEs**:
- Agregar sección "Number Theoretic Transform (NTT)" en ambos READMEs
  (análoga a la sección "Signal Processing (FFT)" que ya existe)
- Enlazar al tutorial desde las tablas de ejemplos

---

## Resumen de fases y dependencias

```
Fase 0: Montgomery reduction
   │
   v
Fase 1: NTT Core (Plan, batch, parallel, Montgomery butterfly)
   │
   ├──> Fase 2: Bluestein modular (tamaños arbitrarios)
   │
   ├──> Fase 3: CRT multi-primo
   │       │
   │       └──> Fase 6: Big integer multiply
   │
   ├──> Fase 4: SIMD (AVX2, NEON)
   │
   ├──> Fase 5: Álgebra de polinomios mod p
   │       │
   │       └──> Fase 7: Convolución negacíclica
   │
   └──> Fase 8: Documentación y tutorial
```

Las fases 2, 3, 4, 5 son **independientes entre sí** y pueden desarrollarse
en paralelo tras completar la Fase 1.

---

## Criterios de completitud

Para considerar `ntt.H` al nivel industrial de `fft.H`:

| Criterio | Métrica |
|----------|---------|
| **Correctitud** | Todos los tests pasan, incluyendo round-trips, comparación con naive, y cross-verificación entre backends |
| **Performance** | NTT N=2^20 en < 50ms (escalar), < 20ms (AVX2), < 100ms (paralelo 4 hilos para batch) |
| **Cobertura de tests** | >= 200 assertions en `Tests/ntt_test.cc` |
| **Documentación** | Doxygen completo en todos los métodos públicos, tutorial bilingüe |
| **API surface** | Plan reutilizable, batch, parallel, SIMD dispatch, CRT multi-primo, álgebra polinomial |
| **Robustez** | Error handling via macros `ah_*`, validación de precondiciones, basic exception guarantee |
| **Convenciones** | 100% adherencia a `CLAUDE.md` |

---

## Archivos a crear o modificar

| Archivo | Acción |
|---------|--------|
| `modular_arithmetic.H` | Agregar `MontgomeryCtx` y funciones `mont_*` |
| `ntt.H` | Reescritura completa (de 212 a ~3000-5000 líneas estimadas) |
| `Tests/ntt_test.cc` | Nuevo archivo de tests dedicado |
| `Tests/montgomery_test.cc` | Tests de Montgomery reduction |
| `Examples/ntt_example.cc` | Ejemplo expandido |
| `docs/ntt-tutorial.md` | Tutorial en español |
| `docs/ntt-tutorial.en.md` | Tutorial en inglés |
| `README.md` | Agregar sección NTT, enlace a tutorial |
| `README.es.md` | Agregar sección NTT, enlace a tutorial |
| `Tests/math_nt_test.cc` | Mover tests NTT a `ntt_test.cc`, mantener el resto |