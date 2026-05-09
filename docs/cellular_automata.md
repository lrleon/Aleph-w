# Módulo de autómatas celulares

`Aleph::CA` es el módulo de Aleph-w para construir, ejecutar, medir y exportar
autómatas celulares en C++20. La API pública vive en headers `.H` y mantiene la
misma filosofía del resto del proyecto: plantillas livianas, estructuras propias
de Aleph donde aportan valor, tests con GoogleTest y ejemplos portables.

## Capas principales

El diseño separa las responsabilidades en capas pequeñas:

```text
Storage -> Lattice -> Neighborhood -> Rule -> Engine -> Observers / Exporters
```

- **Storage**: `Dense_Cell_Storage<T, N>` y `Bit_Cell_Storage<N>` guardan los
  estados de celda de forma contigua.
- **Lattice**: `Lattice<Storage, Boundary>` aplica fronteras abiertas,
  toroidales, reflectivas o constantes; `Ghost_Lattice` mantiene halos para
  kernels con menos ramas.
- **Neighborhood**: Moore, Von Neumann, hexagonal, triangular y vecindarios
  personalizados enumeran vecinos en orden estable.
- **Rule**: reglas totalísticas, outer-totalistic, lookup, estocásticas,
  continuas y compuestas definen la transición local.
- **Engine**: motores síncronos secuenciales, paralelos y Hashlife aplican la
  regla con doble buffer.
- **Observers / Exporters**: métricas, detección de ciclos, hooks por paso,
  RLE/Life, CSV, JSON, PPM/PGM, TikZ, SVG, PNG, GIF, VTK, NPY, HTML, DOT,
  ASCII y sinks de frames.

## Quick start: Game of Life

```cpp
#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>

#include <cstddef>
#include <utility>

using namespace Aleph::CA;

int main()
{
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  Grid grid({ 8, 8 }, 0);
  grid.set({ 1, 2 }, 1);
  grid.set({ 2, 3 }, 1);
  grid.set({ 3, 1 }, 1);
  grid.set({ 3, 2 }, 1);
  grid.set({ 3, 3 }, 1);

  Engine engine(std::move(grid), make_game_of_life_rule(), Moore<2, 1>{});
  for (std::size_t step = 0; step < 5; ++step)
    engine.step();
}
```

## Headers por área

| Área | Headers principales |
|---|---|
| Concepts y traits | `tpl_ca_concepts.H`, `ca-traits.H` |
| Storage y lattices | `tpl_ca_storage.H`, `tpl_ca_bit_storage.H`, `tpl_ca_lattice.H`, `tpl_ca_ghost_lattice.H` |
| Topologías especiales | `tpl_ca_hex_lattice.H`, `tpl_ca_triangular_lattice.H`, `tpl_ca_graph_automaton.H` |
| Vecindarios y reglas | `tpl_ca_neighborhood.H`, `tpl_ca_rule.H`, `tpl_ca_stochastic_rules.H`, `tpl_ca_continuous_rules.H` |
| Motores | `tpl_ca_engine.H`, `tpl_ca_parallel_engine.H`, `tpl_ca_hashlife.H`, `ca-engine-utils.H` |
| Observabilidad | `ca-observer.H`, `ca-metrics.H`, `ca-rng.H`, `ca-kernels.H`, `ca-tiling.H`, `ca-bench.H` |
| IO y visualización | `ca-io.H`, `ca-tikz.H`, `ca-frame-stream.H`, `ca-png.H`, `ca-gif.H`, `ca-svg.H`, `ca-npy.H`, `ca-vtk.H`, `ca-html-player.H`, `ca-ffmpeg-sink.H`, `ca-dot.H`, `ca-ascii.H`, `ca-x11-viewer.H` |

## Ejemplos disponibles

Los ejemplos viven en `Examples/` y se registran automáticamente en CMake.
Los más útiles para empezar son:

- `ca_game_of_life_example.cc`: Game of Life toroidal con patrones clásicos.
- `ca_wolfram1d_example.cc`: reglas elementales de Wolfram.
- `ca_parallel_gol_example.cc`: Game of Life con motor paralelo.
- `ca_boundary_policies_example.cc`: comparación de políticas de frontera.
- `ca_sir_epidemic_example.cc`, `ca_forest_fire_example.cc`,
  `ca_ising_magnetisation_example.cc`: reglas estocásticas reproducibles.
- `ca_gray_scott_example.cc`: CA continuo de reacción-difusión.
- `ca_hashlife_example.cc`: evolución acelerada de patrones Life.
- `ca_export_example.cc` y `ca_visualization_gallery_example.cc`: formatos de
  exportación y visualización.

Compilación:

```bash
cmake -S . -B build -G Ninja -DBUILD_EXAMPLES=ON
cmake --build build
./build/Examples/ca_game_of_life_example glider 20 32 16
```

## Tests

Cada capa tiene tests dedicados bajo `Tests/`. Para ejecutar solo el módulo:

```bash
cmake --build build --target ca_engine_sync_test ca_io_test ca_visual_sinks_test
ctest --test-dir build -R 'CA|ca_|tpl_ca' --output-on-failure
```

La suite completa sigue el flujo normal del proyecto:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Notas de diseño

- Los accesos fuera de rango deben pasar por una política de frontera cuando la
  regla espera topología cerrada o reflectiva.
- Los motores síncronos usan doble buffer; una regla no debe mutar el frame
  actual durante `step()`.
- Las reglas estocásticas usan `Per_Thread_RNG` para obtener reproducibilidad
  independiente del número de hilos.
- Los exporters no deben depender de herramientas externas para los formatos
  base. El sink de ffmpeg es opcional y detecta el ejecutable en runtime.
- `ca-x11-viewer.H` requiere `DISPLAY` para uso interactivo; los tests o demos
  deben saltarse de forma explícita en entornos sin servidor X11.

## Relación con el plan

Este documento cubre la integración de la Fase 12 del plan
`cell-automata.md`. La arquitectura inicial está descrita con más detalle en
`docs/cellular_automata_module.md`.
