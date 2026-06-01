# Cellular-Automata Reproductions

This directory contains deterministic, quantitative reproductions for the
cellular-automata module. They are intentionally separate from the fast PR
suite: `.github/workflows/reproductions.yml` builds and runs them weekly in
Release mode, then uploads CSV files and generated figures as workflow
artifacts.

Configure and build them with:

```bash
cmake -S . -B build-reproductions -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_REPRODUCTIONS=ON
cmake --build build-reproductions --target ca_reproductions
```

Run each executable from the repository root. Run the gnuplot scripts from
this directory after the quantitative programs have produced their CSV files.
For a short local forest-fire calibration pass, use
`./build-reproductions/reproductions/drossel_schwabl --quick`.

## Reproduction Matrix

| Reproduction | Paper | Parameters | Metric | Result | Expected |
| --- | --- | --- | --- | --- | --- |
| BTW sandpile | [Bak, Tang and Wiesenfeld, 1987](https://doi.org/10.1103/PhysRevLett.59.381) | 64x64 open grid, `Sequential_Update<RowMajor>`, 20K warm-up drops, 100K measured drops | Log-binned avalanche-size slope | `results/btw_summary.csv` | `[-1.2, -0.8]` |
| Schelling segregation | [Schelling, 1971](https://doi.org/10.1080/0022250X.1971.9989794); [Vinkovic and Kirman, 2006](https://doi.org/10.1073/pnas.0605290103) | 100x100 torus, threshold `0.5`, 200 steps | Final Moran's I | `results/schelling_summary.csv` | `> 0.7` |
| Drossel-Schwabl forest fire | [Drossel and Schwabl, 1992](https://doi.org/10.1103/PhysRevLett.69.1629) | 256x256 torus, `p_grow=0.005`, `p_lightning=0.0001`, 5K warm-up steps, 50K measured steps | Log-binned cluster-number slope `n(s)`, fit window `4..1024` | `results/drossel_schwabl_summary.csv` | `[-2.2, -1.8]` |
| Gray-Scott patterns | [Pearson, 1993](https://doi.org/10.1126/science.261.5118.189) | 128x128 Neumann grid, 4K steps, spots/stripes/mitosis presets | Mean absolute per-channel PNG difference | `results/gray_scott_summary.csv` | `< 0.01` |
| Game of Life benchmark | [Gosper Hashlife algorithm](https://golly.sourceforge.net/Help/Algorithms/HashLife.html) | 384x384 dense grid and `Hashlife_Engine`, Gosper gun, 1000 requested generations | Wall-clock seconds and final population | `results/game_of_life_bench.csv` | Informational, not a test gate |

## Output

`btw_sandpile` and `drossel_schwabl` emit exact discrete histograms and
summary CSV files. Their gnuplot scripts generate log-log figures under
`figures/generated/`. The forest-fire CSV reports both observed ignitions and
the `1/s`-weighted cluster-number distribution: random lightning samples a
cluster of size `s` with probability proportional to `s`, so the weighting is
required to recover `n(s)`. The fitted `4..1024` scaling window excludes the
finite-grid cutoff visible for larger clusters.

`gray_scott_patterns` generates three PNG files under `figures/generated/`
and compares them pixel-by-pixel against the versioned PNG files in
`figures/reference/`. To regenerate those references intentionally after a
reviewed visual-model change:

```bash
./build-reproductions/reproductions/gray_scott_patterns --update-golden
```

`game_of_life_bench` compares the dense synchronous engine with Aleph's
Golly-style Hashlife implementation. Timing is documented but not asserted,
because runner hardware and load materially affect benchmark ratios.

## PR Smoke Coverage

`Tests/ca_reproductions_smoke_test.cc` exercises reduced versions of all five
paths in the regular PR suite. It checks mechanics only; the weekly job owns
the publication-scale quantitative gates.
