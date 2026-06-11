"""
Smoke tests for the core engine bindings.

The blinker pattern is the textbook period-2 Game of Life oscillator,
so a round trip after two steps is the cleanest sanity check that the
toroidal lattice + Moore neighbourhood + Game-of-Life rule are all
wired up correctly through pybind11.
"""

from __future__ import annotations

import numpy as np

import aleph_ca


def test_module_metadata():
    assert aleph_ca.__version__ == "4.0.0b0"
    # _core re-exports a build_id we set from C++; tells us the wheel
    # is the one this branch shipped (not a stale install).
    assert aleph_ca._core.__build_id__ == "phase-18"


def test_blinker_period_two():
    eng = aleph_ca.make_game_of_life_engine(rows=8, cols=8)
    seed = np.zeros((8, 8), dtype=np.int32)
    seed[3, 2:5] = 1
    eng.set_frame(seed)

    eng.step()
    frame_1 = eng.frame()
    # After one step the horizontal blinker becomes vertical.
    assert frame_1[2:5, 3].tolist() == [1, 1, 1]
    assert frame_1[3, 2] == 0
    assert frame_1[3, 4] == 0

    eng.step()
    frame_2 = eng.frame()
    assert np.array_equal(frame_2, seed)
    assert eng.steps_run() == 2


def test_wolfram_rule_30_produces_chaos():
    eng = aleph_ca.make_wolfram_engine(rule_no=30, width=15)
    seed = np.zeros(15, dtype=np.int32)
    seed[7] = 1
    eng.set_frame(seed)

    eng.step()
    row1 = eng.frame()
    # Rule 30: ...XXX... after one step from a single live cell.
    assert row1[6:9].tolist() == [1, 1, 1]


def test_engine_extents():
    eng = aleph_ca.make_game_of_life_engine(rows=4, cols=6)
    assert eng.extents() == (4, 6)
    assert eng.frame().shape == (4, 6)


def test_invalid_wolfram_rule_rejected():
    import pytest

    with pytest.raises(ValueError):
        aleph_ca.make_wolfram_engine(rule_no=999, width=10)