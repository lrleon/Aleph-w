# Tutorial: Number Theoretic Transform en Aleph-w

Este tutorial describe el NTT industrial actual de `ntt.H`.  En esta fase el
objetivo es el nucleo exacto de la transformada y su primera capa de algebra
formal: planes reutilizables, aritmetica Montgomery en el butterfly, soporte
de tamano arbitrario via Bluestein cuando el modulo admite las raices
necesarias, lotes, paralelismo explicito con `ThreadPool`, reconstruccion
exacta multi-primo via CRT, operaciones de polinomios modulo `p` y
multiplicacion de enteros grandes por bloques de base fija, incluyendo
convolucion negaciclica.

## 1. Que es la NTT?

La Number Theoretic Transform es el analogo modular de la FFT.  En vez de
trabajar con numeros complejos, trabaja con residuos modulo un primo `p`:

- Dominio FFT: `std::complex<Real>`
- Dominio NTT: `uint64_t` modulo `p`
- Resultado FFT: aproximado (redondeo de punto flotante)
- Resultado NTT: exacto modulo `p`

Para una transformada de tamano `N = 2^k`, el modulo debe satisfacer
`N | (p - 1)` para que exista una raiz primitiva `N`-esima de la unidad en
`Z / p Z`.

En ese caso, la transformada directa e inversa toman la misma forma algebraica
que la DFT clasica, pero sobre un campo finito:

```text
F[k] = sum_{j=0}^{N-1} f[j] * w^(j*k) mod p
f[j] = N^(-1) * sum_{k=0}^{N-1} F[k] * w^(-j*k) mod p
```

donde `w` es una raiz primitiva `N`-esima de la unidad y `N^(-1)` es el
inverso modular de `N`.

### 1.1 Relacion FFT vs NTT

| Aspecto | FFT | NTT |
|---------|-----|-----|
| Dominio | `std::complex<Real>` | `uint64_t mod p` |
| Twiddles | `exp(-2 pi i / N)` | `g^((p - 1) / N) mod p` |
| Precision | Aproximada | Exacta modulo `p` |
| Restriccion de tamano | Cualquier `N` con Bluestein | Requiere que existan las raices en `Z/pZ` |
| Caso de uso principal | DSP, espectros, filtros | Productos exactos modulares, CRT, algebra formal |
| Overflow / rango | Error numerico acumulado | Coeficientes acotados por `p` o por el rango CRT |

## 2. Arquitectura de `ntt.H`

El header esta organizado por capas:

- `NTT<MOD, ROOT>`: core secuencial y paralelo de transformada/modulo
- `NTT<MOD, ROOT>::Plan`: cache reutilizable para bit-reversal, twiddles o
  datos de Bluestein
- `modular_arithmetic.H`: `MontgomeryCtx`, `mont_mul()` y `mont_reduce()` para
  el hot loop del butterfly
- dispatch SIMD: AVX2/NEON en tiempo de ejecucion sobre el camino
  potencia-de-dos
- `NTTExact`: paquete de tres primos con reconstruccion CRT en `__uint128_t`
- capa algebraica: series formales, evaluacion multipunto, interpolacion,
  bigint y convolucion negaciclica

La superficie publica de esta fase ofrece:

- `NTT<MOD, ROOT>::transform()` y `transformed()`
- `NTT<MOD, ROOT>::multiply()` y `multiply_inplace()`
- `NTT<MOD, ROOT>::transform_batch()`
- `NTT<MOD, ROOT>::ptransform()` y `pmultiply()`
- `NTT<MOD, ROOT>::Plan` para tamanos repetidos
- `NTT<MOD, ROOT>::simd_backend_name()`,
  `avx2_dispatch_available()` y `neon_dispatch_available()`
- `NTTExact::multiply()` y `NTTExact::pmultiply()` para productos exactos en
  `__uint128_t`
- `NTT<MOD, ROOT>::poly_eval()`, `poly_inverse()`, `poly_divmod()`
- `NTT<MOD, ROOT>::poly_log()`, `poly_exp()`, `poly_sqrt()` y `poly_power()`
- `NTT<MOD, ROOT>::multipoint_eval()` e `interpolate()`
- `NTT<MOD, ROOT>::bigint_multiply()` y `pbigint_multiply()` para enteros
  grandes representados como digitos en base `B`
- `NTT<MOD, ROOT>::negacyclic_multiply()` para productos modulo `x^N + 1`

El alcance actual es deliberadamente estricto:

- las potencias de dos usan el kernel directo de Cooley-Tukey
- los tamanos no potencia de dos usan Bluestein cuando `2 * N` divide `MOD - 1`
- el tamano interno de convolucion de Bluestein debe seguir cabiendo en la
  mayor potencia de dos soportada por el modulo
- todos los coeficientes se reducen modulo `MOD`
- el SIMD actual acelera el butterfly potencia-de-dos agrupando sumas/restas en
  AVX2 o NEON y deja la multiplicacion Montgomery por lane
- las rutas paralelas con `ThreadPool` siguen prefiriendo el kernel escalar para
  evitar mezclar paralelismo por etapas con dispatch SIMD
- `NTTExact` usa tres primos fijos y solo acepta entradas que una cota
  conservadora puede garantizar dentro del rango CRT
- el algebra formal trabaja sobre series truncadas modulo `x^n` y exige las
  precondiciones clasicas de invertibilidad, logaritmo, exponencial y raiz
  cuadrada en anillos formales
- la API bigint recibe digitos little-endian (`a[0]` menos significativo),
  normaliza ceros altos y devuelve `{0}` cuando el producto es cero
- la convolucion negaciclica exige `a.size() == b.size() == N`, `N` potencia de
  dos y una raiz primitiva de orden `2N` bajo el modulo actual

## 3. Uso basico

```cpp
# include <ntt.H>

using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {1, 2, 3};
Aleph::Array<uint64_t> b = {4, 5};

auto c = DefaultNTT::multiply(a, b);
// c = {4, 13, 22, 15} modulo 998244353
```

Para transformar directamente:

```cpp
Aleph::Array<uint64_t> values = {3, 1, 4, 1, 5, 9, 2, 6};

DefaultNTT::transform(values, false); // NTT directa
DefaultNTT::transform(values, true);  // NTT inversa
```

Despues de la inversa, `values` vuelve a los coeficientes originales modulo
`MOD`.

## 4. Planes reutilizables

Si vas a transformar muchas veces el mismo tamano, usa un plan:

```cpp
using DefaultNTT = Aleph::NTT<>;

DefaultNTT::Plan plan(7);

Aleph::Array<uint64_t> x = ...; // size 7
Aleph::Array<uint64_t> y = ...; // size 7

plan.transform(x, false);
plan.transform(y, false);
```

Para tamanos potencia de dos, `Plan` precomputa:

- permutacion bit-reversal
- twiddles forward en forma Montgomery
- twiddles inverse en forma Montgomery
- `N^{-1} mod MOD` para normalizar la inversa

Para tamanos no potencia de dos soportados, `Plan` cambia a Bluestein y
precomputa:

- factores chirp
- kernels de convolucion ya transformados
- un plan interno potencia de dos para la convolucion

## 5. Por que importa Montgomery?

La transformada ejecuta una multiplicacion modular por butterfly.  En esta
fase esas multiplicaciones usan `MontgomeryCtx`, `mont_mul()` y `mont_reduce()`
de `modular_arithmetic.H`.

La consecuencia practica es:

- las sumas y restas siguen siendo modulares ordinarias
- las multiplicaciones evitan reduccion generica basada en division dentro del
  hot loop

La API publica sigue trabajando con residuos normales, pero el nucleo corre en
forma Montgomery internamente.

## 6. Transformadas por lotes

Cuando muchas entradas comparten el mismo tamano, las APIs batch evitan
reconstruir el plan y hacen explicita la intencion:

```cpp
Aleph::NTT<>::Plan plan(8);

Aleph::Array<Aleph::Array<uint64_t>> batch = {
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 0, 0, 0, 0}
};

plan.transform_batch(batch, false);
```

Todos los items del lote deben coincidir exactamente con el tamano del plan.

## 7. Ejecucion paralela

Las variantes paralelas usan `ThreadPool`:

```cpp
# include <thread_pool.H>

Aleph::ThreadPool pool(4);
Aleph::NTT<>::Plan plan(1 << 12);

Aleph::Array<uint64_t> values = ...;
plan.ptransform(pool, values, false);

auto product = Aleph::NTT<>::pmultiply(pool, a, b);
```

La estrategia actual es:

- una sola transformada: paralelizar el trabajo dentro de las etapas
- un lote: paralelizar a traves de los items cuando conviene

Asi se evita oversubscription por anidamiento.

## 8. Dispatch SIMD

El backend SIMD activo puede consultarse en tiempo de ejecucion:

```cpp
using DefaultNTT = Aleph::NTT<>;

const char * backend = DefaultNTT::simd_backend_name();
bool has_avx2 = DefaultNTT::avx2_dispatch_available();
bool has_neon = DefaultNTT::neon_dispatch_available();
```

Politica actual:

- `ALEPH_NTT_SIMD=auto|scalar|avx2|neon`
- `auto` selecciona el mejor backend disponible
- el SIMD solo se aplica al camino potencia-de-dos
- Bluestein y las variantes multihilo siguen usando el kernel escalar

## 9. Multiplicacion exacta con CRT

Cuando los coeficientes reales del producto pueden exceder un solo primo,
`NTTExact` ejecuta tres NTT independientes y reconstruye cada coeficiente en
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

Contrato importante:

- la reconstruccion usa los primos `998244353`, `469762049` y `1004535809`
- el resultado es exacto solo si cada coeficiente verdadero es `< p1 * p2 * p3`
- la implementacion verifica una cota suficiente conservadora y lanza
  `std::invalid_argument` cuando no puede garantizar exactitud
- la variante paralela `NTTExact::pmultiply(pool, a, b)` paraleliza primero los
  tres primos y luego la reconstruccion CRT

## 10. Consultas sobre tamanos soportados

El header expone dos utilidades practicas:

```cpp
using DefaultNTT = Aleph::NTT<>;

auto max_n = DefaultNTT::max_transform_size();
bool ok = DefaultNTT::supports_size(1024);
auto w = DefaultNTT::primitive_root_of_unity(1024);
```

Para el modulo por defecto `998244353`:

- el maximo tamano directo potencia de dos es `2^23`
- tamanos como `7`, `14`, `17` y `119` tambien estan soportados
- tamanos como `3`, `10` o `13` no lo estan porque las raices necesarias no
  existen bajo ese modulo

## 11. Algebra formal de polinomios

Sobre el mismo nucleo NTT, `ntt.H` ahora expone primitivas clasicas de algebra
de polinomios modulo `MOD`.

### 11.1 Evaluacion y division

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> f = {9, 7, 5, 3};
uint64_t value = DefaultNTT::poly_eval(f, 9);

auto qr = DefaultNTT::poly_divmod(f, Aleph::Array<uint64_t>({1, 1, 1}));
Aleph::Array<uint64_t> q = qr.first;
Aleph::Array<uint64_t> r = qr.second;
```

`poly_divmod()` devuelve cociente y residuo normalizados.  Si el divisor es el
polinomio cero, la implementacion lanza `std::invalid_argument`.

### 11.2 Series formales

Las operaciones siguientes trabajan truncadas modulo `x^n`:

```cpp
Aleph::Array<uint64_t> unit = {1, 3, 4, 0, 2, 1};

auto inv = DefaultNTT::poly_inverse(unit, 6);
auto lg = DefaultNTT::poly_log(unit, 6);
auto ex = DefaultNTT::poly_exp(Aleph::Array<uint64_t>({0, 5, 7, 0}), 6);
auto pw = DefaultNTT::poly_power(unit, 3, 6);
```

Contrato algebraico:

- `poly_inverse(f, n)` exige `f(0) != 0 mod MOD`
- `poly_log(f, n)` exige `f(0) = 1`
- `poly_exp(f, n)` exige `f(0) = 0`
- `poly_power(f, k, n)` usa la descomposicion formal habitual con desplazamiento
  por ceros iniciales y cubre tambien `k = 0`

Internamente:

- la inversa usa Newton doubling
- `poly_log()` usa `f' / f` seguido de integral formal
- `poly_exp()` itera con `g *= (1 - log(g) + f)`
- `poly_power()` reduce a `log/exp` despues de separar el termino lider

### 11.3 Raiz cuadrada formal

```cpp
Aleph::Array<uint64_t> square = {4, 12, 25, 24, 16};
auto root = DefaultNTT::poly_sqrt(square, 5);
```

`poly_sqrt()` soporta dos casos:

- termino constante no nulo con raiz modular, resuelto con Tonelli-Shanks y
  Newton
- polinomios con ceros iniciales en grado par, donde la raiz se reduce
  recursivamente tras factorizar `x^(2m)`

Si la primera potencia no nula aparece en grado impar o el termino constante no
es un residuo cuadratico modulo `MOD`, la operacion lanza
`std::invalid_argument`.

### 11.4 Evaluacion multipunto e interpolacion

```cpp
Aleph::Array<uint64_t> points = {2, 4, 8, 16};
Aleph::Array<uint64_t> values = DefaultNTT::multipoint_eval(f, points);
Aleph::Array<uint64_t> recovered = DefaultNTT::interpolate(points, values);
```

La implementacion usa un arbol de subproductos:

- `multipoint_eval()` reduce por residuos sucesivos contra cada nodo
- `interpolate()` evalua la derivada del producto total en los puntos y combina
  recursivamente los subpolinomios

`multipoint_eval()` acepta puntos repetidos.  `interpolate()` si exige puntos
distintos modulo `MOD`; si hay duplicados, lanza `std::invalid_argument`.

## 12. Multiplicacion de enteros grandes

La misma infraestructura exacta sirve para multiplicar enteros no negativos
representados como digitos en base `B`.

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {0, 9, 8, 7, 6}; // 67890 en base 10
Aleph::Array<uint64_t> b = {0, 1, 2, 3, 4}; // 43210 en base 10

auto product = DefaultNTT::bigint_multiply<10>(a, b);
```

Contrato de representacion:

- `a[0]` y `b[0]` son los digitos menos significativos
- cada digito debe cumplir `0 <= digit < Base`
- la salida queda normalizada sin ceros altos, salvo el cero que se representa
  como `{0}`
- la base por defecto es `2^15`, util para empaquetar bloques densos en
  `uint64_t`

Seleccion de backend exacto:

- si `min(a.size(), b.size()) * (Base - 1)^2 < MOD` y el tamano de
  convolucion cabe en `NTT<MOD, ROOT>`, se usa un solo primo
- en caso contrario, la API cae a `NTTExact` y reconstruye coeficientes exactos
  antes de propagar carries
- `pbigint_multiply()` paraleliza la convolucion; la propagacion de carry sigue
  siendo un barrido lineal

Para enteros decimales grandes, `Base = 10` es comodo para pruebas y demos.
Para cargas reales, la base por defecto reduce el numero total de digitos.

## 13. Convolucion negaciclica

`ntt.H` tambien expone multiplicacion modulo `(x^N + 1)`, util para
criptografia basada en reticulos y para kernels estilo Schonhage-Strassen.

```cpp
using DefaultNTT = Aleph::NTT<>;

Aleph::Array<uint64_t> a = {1, 1, 1, 1};
Aleph::Array<uint64_t> b = {1, 1, 0, 0};

auto c = DefaultNTT::negacyclic_multiply(a, b);
// c = {0, 2, 2, 2} modulo 998244353
```

Contrato:

- `a.size() == b.size() == N`
- `N` debe ser potencia de dos
- debe existir una raiz primitiva de orden `2N`, asi que en la practica se
  requiere `2N <= max_transform_size()` para modulos potencia-de-dos-friendly

La implementacion usa el esquema clasico:

- twist por una raiz `2N`-esima `zeta`
- NTT directa de tamano `N` sobre `a[i] * zeta^i` y `b[i] * zeta^i`
- producto punto a punto
- NTT inversa y untwist por `zeta^{-i}`

## 14. Limites de esta fase

Este tutorial cubre solo lo implementado hoy.  Las siguientes capacidades
quedan preparadas para fases posteriores, pero no forman parte del header
actual:

- seleccion automatica de primos
- reconstruccion exacta mas alla del rango de tres primos
- aceptacion de todos los casos validos posibles: la cota actual es
  conservadora y puede rechazar algunas entradas que aun cabrian en CRT

## 15. Programa de ejemplo

Consulta `Examples/ntt_example.cc` para un recorrido ejecutable compacto de:

- multiplicacion de polinomios
- backend SIMD activo
- multiplicacion exacta con CRT
- algebra formal de polinomios
- multiplicacion de enteros grandes en base 10
- convolucion negaciclica
- planes reutilizables
- lotes
- multiplicacion paralela

`math_nt_example.cc` sigue mostrando el toolbox numerico mas amplio alrededor
de la NTT.

## 16. Referencia rapida

| Necesidad | API |
|-----------|-----|
| Transformada directa/inversa | `NTT<>::transform(a, invert)` |
| Producto modular | `NTT<>::multiply(a, b)` |
| Plan reutilizable | `NTT<>::Plan plan(n)` |
| Paralelismo explicito | `NTT<>::ptransform(pool, a, invert)`, `pmultiply()` |
| Lotes | `plan.transform_batch(batch, invert)` |
| Backend SIMD | `simd_backend_name()`, `avx2_dispatch_available()`, `neon_dispatch_available()` |
| CRT exacto | `NTTExact::multiply(a, b)`, `NTTExact::pmultiply(pool, a, b)` |
| Algebra formal | `poly_inverse`, `poly_divmod`, `poly_log`, `poly_exp`, `poly_sqrt`, `poly_power` |
| Evaluacion/interpolacion | `poly_eval`, `multipoint_eval`, `interpolate` |
| Big integers | `bigint_multiply<Base>()`, `pbigint_multiply<Base>()` |
| Modulo `x^N + 1` | `negacyclic_multiply(a, b)` |
| Capacidad del modulo | `max_transform_size()`, `supports_size(n)`, `primitive_root_of_unity(n)` |
