# Performance-regression gate (cellular automata)

This directory implements roadmap **Phase 20**: catching performance
regressions between commits with the same seriousness as broken tests.

## Layout

| Path | Purpose |
|---|---|
| `ca/bench_support.H` | Shared timing harness + structured-JSON emitter. |
| `ca/bench_*.cc` | The six anchor benchmarks. |
| `../scripts/ca_perf_regression.rb` | Runs the anchors, compares against the baseline, gates the PR. |
| `perf_config.json` | Per-benchmark regression thresholds (survives baseline regen). |
| `baseline.json` | Committed reference numbers (regenerated on the CI runner). |
| `history.jsonl` | Append-only trend samples (one line per baseline refresh). |
| `../scripts/ca_perf_trend.rb` | Renders the 90-day trend as a dependency-free HTML/SVG page. |

The Ruby tooling lives under `scripts/` next to the project's other CI
scripts (`fft_perf_regression.rb`, `polynomial_perf_regression.rb`, …); the
data files it reads and writes stay here in `benchmarks/`.

## Anchor benchmarks

| Benchmark id | Kernel | Size |
|---|---|---|
| `gol_1024` | Dense Game of Life, toroidal | 1024² × 100 steps |
| `wolfram_30_1d` | Elementary Wolfram rule 30 | 1 M cells × 1000 steps |
| `gray_scott_512` | Gray-Scott reaction-diffusion | 512² × 1000 steps |
| `parallel_gol_2048` | Parallel Game of Life | 2048² × 100 steps |
| `hashlife_breeder` | Hashlife exponential advance | 2²⁰ generations |
| `multi_field_lv_512` | Lotka-Volterra, SoA layout | 512² × 500 steps |

Each binary times its kernel with one untimed warmup pass followed by the
median of several timed passes (3 by default; override with the
`CA_BENCH_REPEATS` environment variable). It prints exactly one line of JSON:

```json
{"name":"gol_1024","wall_ns":3377500712,"cells_per_sec":31045914.995,"compiler":"gcc 13.3.0","cpu":"Intel(R) Core(TM) Ultra 7 265KF"}
```

### Note on `hashlife_breeder`

The roadmap asks for a quadratically growing *breeder* so Hashlife's node
table keeps expanding and the run cannot be fully memoised — that is what makes
the wall time large and stable enough to gate on. A periodic emitter (glider
gun) would compress almost perfectly and collapse to a sub-millisecond,
jitter-dominated measurement. To get the same effect with a pattern that is
guaranteed valid and fully deterministic (no fragile hand-typed RLE), this
anchor seeds a fixed-seed chaotic soup whose long transient forces Hashlife to
canonicalise a large, ever-changing set of nodes, then performs a single
`advance(20)` of 2²⁰ generations.

## Running locally

```bash
cmake -S . -B build-benchmarks -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON \
  -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_REPRODUCTIONS=OFF
cmake --build build-benchmarks --target ca_benchmarks

# Compare against the committed baseline (exit code 1 on regression):
ruby scripts/ca_perf_regression.rb --build-dir build-benchmarks

# Regenerate the baseline on this machine:
ruby scripts/ca_perf_regression.rb --build-dir build-benchmarks --update-baseline
```

## How the gate decides

The gate keys on `wall_ns` (lower is better). A benchmark **regresses** when
its median wall time grows by more than its threshold percentage (default 10 %,
overridable per benchmark in `perf_config.json`) relative to `baseline.json`,
and that fails the PR.

`baseline.json` is **hardware-specific**: absolute times only mean something on
the machine that recorded them. The committed file is therefore regenerated on
the canonical CI runner by the `perf-update` workflow. Until a
matching-hardware baseline exists, `perf-gate` runs with
`--soft-on-cpu-mismatch`, so a baseline seeded on different hardware produces an
advisory comparison instead of a spurious failure.

## CI workflows

- **`.github/workflows/perf-gate.yml`** — on every PR: builds the anchors, pins
  them to a fixed CPU set with `taskset`, compares against the baseline, posts a
  single updated comment with the comparison table, and fails when a benchmark
  regresses past its threshold.
- **`.github/workflows/perf-update.yml`** — manually or on a `master` push whose
  commit message contains `[perf-update]`: regenerates `baseline.json`, appends
  a sample to `history.jsonl`, rebuilds the 90-day trend page, and commits the
  result back via the bot account. The trend page is also uploaded as an
  artifact and can be published to GitHub Pages.