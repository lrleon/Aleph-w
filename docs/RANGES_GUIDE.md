# C++20 Ranges con Contenedores Aleph-w

## Introducción

A partir de C++20, la biblioteca estándar incluye **Ranges**, una forma moderna y composable de trabajar con secuencias de datos. Los contenedores de Aleph-w son completamente compatibles con esta funcionalidad, permitiendo usar vistas, adaptadores y algoritmos de rangos.

## Requisitos

- Compilador con soporte C++20 (GCC 10+, Clang 13+, MSVC 19.29+)
- Flag de compilación: `-std=c++20`

```cpp
#include <ranges>
#include <ah-ranges.H>  // Adaptadores de Aleph
```

## Contenedores Compatibles

Todos los contenedores principales de Aleph-w satisfacen `std::ranges::range`:

| Categoría | Contenedores |
|-----------|--------------|
| **Listas** | `DynList<T>`, `DynDlist<T>` |
| **Arrays** | `DynArray<T>` |
| **Pilas** | `DynListStack<T>`, `ArrayStack<T>` |
| **Colas** | `DynListQueue<T>`, `ArrayQueue<T>` |
| **Árboles** | `DynSetRbTree<T>`, `DynSetAvlTree<T>`, `DynSetSplayTree<T>`, `DynSetTreap<T>` |
| **Hash** | `DynSetLhash<T>`, `ODhashTable<T>`, `OLhashTable<T>` |
| **Heaps** | `DynBinHeap<T>` |
| **Otros** | `Random_Set<T>` |

## Uso Básico: For-Range

Todos los contenedores soportan la sintaxis for-range:

```cpp
#include <htlist.H>
#include <tpl_dynSetTree.H>

using namespace Aleph;

DynList<int> list;
list.append(1);
list.append(2);
list.append(3);

// For-range loop
for (auto x : list) 
    std::cout << x << " ";  // Output: 1 2 3

// Con árboles (orden sorted)
DynSetRbTree<int> set;
set.insert(5);
set.insert(2);
set.insert(8);

for (auto x : set)
    std::cout << x << " ";  // Output: 2 5 8 (ordenado)
```

## Vistas Estándar (std::views)

Los contenedores Aleph pueden usarse con todas las vistas de C++20:

### filter - Filtrar elementos

```cpp
DynArray<int> arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Solo pares
for (auto x : arr | std::views::filter([](int x) { return x % 2 == 0; }))
    std::cout << x << " ";  // Output: 2 4 6 8 10
```

### transform - Transformar elementos

```cpp
DynList<int> nums;
for (int i = 1; i <= 5; i++) nums.append(i);

// Cuadrados
for (auto x : nums | std::views::transform([](int x) { return x * x; }))
    std::cout << x << " ";  // Output: 1 4 9 16 25
```

### take / drop - Limitar elementos

```cpp
DynArray<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Primeros 3
for (auto x : data | std::views::take(3))
    std::cout << x << " ";  // Output: 1 2 3

// Saltar primeros 5
for (auto x : data | std::views::drop(5))
    std::cout << x << " ";  // Output: 6 7 8 9 10
```

### Composición de vistas (Pipelines)

Las vistas se pueden encadenar con el operador `|`:

```cpp
auto result = data 
    | std::views::filter([](int x) { return x % 2 == 0; })  // Solo pares
    | std::views::transform([](int x) { return x * 2; })    // Duplicar
    | std::views::take(3);                                   // Tomar 3

for (auto x : result)
    std::cout << x << " ";  // Output: 4 8 12
```

## Generación de Rangos

### iota - Secuencia de números

```cpp
// Números del 1 al 10
for (auto x : std::views::iota(1, 11))
    std::cout << x << " ";  // Output: 1 2 3 4 5 6 7 8 9 10

// Infinito (usar con take)
for (auto x : std::views::iota(1) | std::views::take(5))
    std::cout << x << " ";  // Output: 1 2 3 4 5
```

## Conversión a Contenedores Aleph

La biblioteca `ah-ranges.H` proporciona adaptadores pipe para convertir rangos a contenedores Aleph:

### Pipe Adaptors Disponibles

| Adaptor | Contenedor Destino |
|---------|-------------------|
| `to_dynlist_v` | `DynList<T>` |
| `to_dynarray_v` | `DynArray<T>` |
| `to_dyndlist_v` | `DynDlist<T>` |
| `to_dynliststack_v` | `DynListStack<T>` |
| `to_arraystack_v` | `ArrayStack<T>` |
| `to_dynlistqueue_v` | `DynListQueue<T>` |
| `to_arrayqueue_v` | `ArrayQueue<T>` |
| `to_randomset_v` | `Random_Set<T>` |

### Ejemplos de Conversión

```cpp
#include <ah-ranges.H>
using namespace Aleph;

// Rango a DynList
auto list = std::views::iota(1, 11) | to_dynlist_v;
// list contiene: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

// Pipeline completo a DynArray
auto squares = std::views::iota(1, 20)
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; })
    | to_dynarray_v;
// squares contiene: 4, 16, 36, 64, 100, 144, 196, 256, 324
```

### Conversión Genérica: to<Container>()

Para contenedores con plantillas más complejas, usa `to<Container>()`:

```cpp
// A un árbol ordenado
auto set = std::views::iota(1, 100) 
    | std::views::filter([](int x) { return x % 7 == 0; })
    | to<DynSetRbTree<int>>();

// A una cola
auto queue = std::views::iota(1, 6) | to<DynListQueue<int>>();
```

## Algoritmos STL con Iteradores

Los iteradores de Aleph son compatibles con algoritmos STL tradicionales:

```cpp
DynList<int> nums;
for (int i : {3, 1, 4, 1, 5, 9, 2, 6})
    nums.append(i);

// min/max con iteradores explícitos
auto min_it = std::min_element(nums.begin(), nums.end());
auto max_it = std::max_element(nums.begin(), nums.end());
std::cout << "Min: " << *min_it << ", Max: " << *max_it << std::endl;

// find
auto it = std::find(nums.begin(), nums.end(), 5);
if (it != nums.end())
    std::cout << "Found: " << *it << std::endl;

// count_if
auto count = std::count_if(nums.begin(), nums.end(), 
                           [](int x) { return x > 3; });
std::cout << "Count > 3: " << count << std::endl;
```

## Funciones de ah-ranges.H

### Generación Lazy

```cpp
#include <ah-ranges.H>
using namespace Aleph;

// Rango infinito de números
auto naturals = lazy_iota(1);  // 1, 2, 3, ...

// Primeros 10
auto first10 = naturals | std::views::take(10) | to_dynlist_v;

// Rango con paso personalizado (C++23 o fallback)
auto even_nums = lazy_iota(0, 2);  // 0, 2, 4, 6, ...
```

## Ejemplo Completo

```cpp
#include <ranges>
#include <iostream>
#include <htlist.H>
#include <tpl_dynArray.H>
#include <ah-ranges.H>

using namespace Aleph;

int main() {
    // Crear datos de prueba
    DynArray<int> data;
    for (int i = 1; i <= 20; i++)
        data.append(i);
    
    // Pipeline: filtrar, transformar, limitar
    auto processed = data
        | std::views::filter([](int x) { return x % 3 == 0; })  // Múltiplos de 3
        | std::views::transform([](int x) { return x * x; })    // Cuadrados
        | std::views::take(4);                                   // Solo 4
    
    std::cout << "Resultado: ";
    for (auto x : processed)
        std::cout << x << " ";  // Output: 9 36 81 144
    std::cout << std::endl;
    
    // Convertir resultado a DynList
    auto result_list = processed | to_dynlist_v;
    std::cout << "Tamaño: " << result_list.size() << std::endl;
    
    // Generar desde cero
    auto fibonacci_like = std::views::iota(1, 10)
        | std::views::transform([](int n) {
            int a = 0, b = 1;
            for (int i = 0; i < n; i++) {
                int temp = a + b;
                a = b;
                b = temp;
            }
            return a;
        })
        | to_dynarray_v;
    
    std::cout << "Fibonacci-like: ";
    for (auto x : fibonacci_like)
        std::cout << x << " ";  // Output: 1 1 2 3 5 8 13 21 34
    std::cout << std::endl;
    
    return 0;
}
```

## Limitaciones Conocidas

1. **Algoritmos de rangos con predicados**: Algunos algoritmos de `std::ranges::` (como `std::ranges::min_element`, `std::ranges::count_if`) requieren iteradores que satisfagan conceptos más estrictos. Use las versiones con iteradores explícitos cuando sea necesario:
   ```cpp
   // En lugar de:
   // auto min = std::ranges::min_element(container);
   
   // Use:
   auto min = std::min_element(container.begin(), container.end());
   ```

2. **Contenedores no secuenciales**: Los hash tables y heaps iteran en orden no determinístico.

3. **Evaluación lazy**: Las vistas son lazy; no materialice grandes rangos sin necesidad.

## Compatibilidad con Funciones Aleph Existentes

Las funciones de `ahFunctional.H` siguen funcionando y usan ranges internamente cuando es posible:

```cpp
DynList<int> nums = {1, 2, 3, 4, 5};

// API funcional de Aleph (optimizada con ranges internamente)
auto sum = nums.foldl(0, [](int acc, int x) { return acc + x; });
auto doubled = nums.maps<int>([](int x) { return x * 2; });
auto evens = nums.filter([](int x) { return x % 2 == 0; });
```

## Compilación

```bash
g++ -std=c++20 -I/path/to/Aleph-w my_program.cc -o my_program
```

---

**Versión**: Aleph-w con C++20 Ranges  
**Fecha**: 2026

