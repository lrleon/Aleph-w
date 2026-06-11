"""
Loose performance gate.

The acceptance criterion from the roadmap is "100x100 GoL 100 steps
finishes in < 5 s on a default GitHub runner". We use `pytest-timeout`
plus a wall-clock assertion so the regression is visible in CI logs
even when the timeout doesn't fire.
"""

from __future__ import annotations

import time

import numpy as np
import pytest

import aleph_ca


@pytest.mark.timeout(15)  # generous: CI shared runners are slow.
def test_gol_100x100_100steps_under_5s():
    rows, cols, steps = 100, 100, 100
    eng = aleph_ca.make_game_of_life_engine(rows, cols)

    # Random seed: ~25% density gets us a non-trivial workload.
    rng = np.random.default_rng(seed=42)
    seed = (rng.random((rows, cols)) < 0.25).astype(np.int32)
    eng.set_frame(seed)

    start = time.perf_counter()
    eng.run(steps)
    elapsed = time.perf_counter() - start

    assert eng.steps_run() == steps
    assert elapsed < 5.0, f"GoL 100x100x100 took {elapsed:.2f}s, expected < 5s"


@pytest.mark.timeout(30)
def test_wolfram_1d_one_million_cells_one_step():
    """The bigger 1D case mostly catches O(N²) regressions on the
    inner loop. Anything < 2s/step is good enough for our purposes."""
    eng = aleph_ca.make_wolfram_engine(rule_no=110, width=1_000_000)
    seed = np.zeros(1_000_000, dtype=np.int32)
    seed[500_000] = 1
    eng.set_frame(seed)

    start = time.perf_counter()
    eng.step()
    elapsed = time.perf_counter() - start

    assert elapsed < 2.0, f"Wolfram 1D 1M cells/step took {elapsed:.2f}s"