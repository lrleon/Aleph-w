# aleph-ca — Python bindings for Aleph-w cellular automata

Brings the core `Aleph::CA` engines to Python so the broader
scientific-computing community (Jupyter, NumPy, Matplotlib) can drive the
C++ engines without writing C++.

## Install

From a clone of the repository:

```bash
# editable install (requires CMake >= 3.18 and a C++20 compiler)
pip install --editable python/

# build a wheel only
python -m build python/
```

System dependencies (matching the C++ library):

- `libgmp-dev libmpfr-dev libgsl-dev`
- A modern C++ compiler with C++20 support
- Python 3.10+ with development headers

The build pulls `pybind11` automatically via `FetchContent` if it is not
already installed in the environment.

## Quick start

```python
import aleph_ca

eng = aleph_ca.make_game_of_life_engine(rows=32, cols=32)

# seed a "blinker" oscillator with NumPy
import numpy as np
seed = np.zeros((32, 32), dtype=np.int32)
seed[10, 9:12] = 1
eng.set_frame(seed)

eng.run(2)
print(eng.frame()[10, 9:12])         # period-2 oscillator returns to seed
```

## What is exposed

| Object                        | Purpose                                         |
|-------------------------------|-------------------------------------------------|
| `GameOfLifeEngine`            | Conway's Game of Life (toroidal int lattice)    |
| `WolframEngine`               | 1D elementary Wolfram rules `[0, 255]`          |
| `ForestFireEngine`            | Drossel & Schwabl 1992 (stochastic 2D)          |
| `GenericEngine2DInt/Float`    | Python-callable rule (slow path, for prototyping) |
| `save_checkpoint` / `load`    | Atomic + DEFLATE-compressed snapshot I/O        |
| `write_png_bytes` / `_svg_`   | In-memory image export (good for notebooks)     |

NumPy frames come in two flavours:

- `engine.frame()` — owning copy, always safe.
- `engine.frame_view()` — zero-copy view; becomes stale on the next `step()`
  because the engine swaps double buffers internally.

## Limitations (Phase 18 scope)

- Only `int` (combinatorial) and `float` (continuous) cell types are
  exposed. Multi-field engines and graph-automaton frames are out of
  scope until Phase 19/22.
- Neighbourhood is fixed to Moore radius-1 in 2D. Custom neighbourhoods
  require a small native shim.
- Python rules pay one GIL acquisition + list build per cell. Promote
  to C++ for production workloads.

## Tests

```bash
pip install --editable 'python/[test]'
pytest python/tests/ -v
```