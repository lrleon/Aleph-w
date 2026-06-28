# 04 — Observability

Rendering shows you *what* a simulation looks like; **observers** tell you what
it is *doing* — densities, activity, entropy — without slowing the inner loop.

## Attaching an observer

An observer is a small object that the engine calls back on every step. Attach
it with `attach_observer`, then run normally:

```cpp
#include <ca-engine-utils.H>
#include <ca-observer.H>

using namespace Aleph;
using namespace Aleph::CA;

auto eng = make_gol_engine(64, 64);
// ... seed eng ...

Density_Observer<int> density(/*sample_every=*/1);
attach_observer(eng, density);

eng.run(500);

// 1 initial sample + one per step.
std::cout << "samples: " << density.size() << '\n';
std::cout << "final live density: "
          << density.density_at(density.size() - 1) << '\n';
```

`Density_Observer` records the live-cell count (and density) at each sample.
`counts()` returns the raw counts; `density_at(i)` the normalized fraction.

## The built-in observers

| Observer | Measures |
|----------|----------|
| `Density_Observer<State>` | live-cell count / density per sample |
| `Activity_Observer<Lattice>` | how many cells changed since the last step |
| `Entropy_Observer<Lattice>` | Shannon entropy of the state distribution |
| `Sampling_Observer` | periodically snapshots whole frames |

`Activity_Observer` is the cheapest way to detect a fixed point: when activity
drops to zero, the configuration has stabilized.

## Combining observers

`make_composite_observer` fans one step out to several observers at once:

```cpp
Density_Observer<int> dens(1);
Activity_Observer<decltype(eng.frame())> act;     // tracks changed cells

auto combo = make_composite_observer(dens, act);
attach_observer(eng, combo);
eng.run(200);
```

## One-off metrics

When you only need a number for the current frame (not a time series),
`ca-metrics.H` has free functions such as `state_histogram(frame)` (returns an
`Array` of per-state counts). These are handy inside an `on_post_step` callback
or after a run:

```cpp
#include <ca-metrics.H>

auto hist = state_histogram(eng.frame());   // Array<ca_size_t>
```

## Stationarity detection

Pair an `Activity_Observer` with a small loop to stop early once the lattice
stops changing — useful for relaxation rules (sandpiles, majority voting):

```cpp
Activity_Observer<decltype(eng.frame())> act;
attach_observer(eng, act);
while (eng.steps_run() < 10000)
{
  eng.step();
  if (act.size() > 0 and act.activity()[act.size() - 1] == 0)
    break;                                   // reached a fixed point
}
```

## Next

[Chapter 05 — Long-running simulations](05_long_running.md): checkpoint and
resume runs that take hours.