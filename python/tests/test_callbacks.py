"""
Python-callback rule tests.

The generic engines accept any Python callable matching
`(state, neighbours) -> new_state`. We reimplement Game of Life in pure
Python and verify it produces the same trajectory as the C++ engine
over a few steps — that's the strongest end-to-end check possible.
"""

from __future__ import annotations

import numpy as np

import aleph_ca


def _gol_rule(state: int, neighbours) -> int:
    live = sum(1 for n in neighbours if n != 0)
    if state != 0:
        return 1 if live in (2, 3) else 0
    return 1 if live == 3 else 0


def test_python_rule_matches_cxx_game_of_life():
    rows, cols = 6, 6
    seed = np.zeros((rows, cols), dtype=np.int32)
    # Glider.
    seed[1, 2] = 1
    seed[2, 3] = 1
    seed[3, 1] = 1
    seed[3, 2] = 1
    seed[3, 3] = 1

    cxx = aleph_ca.make_game_of_life_engine(rows, cols)
    cxx.set_frame(seed)

    py_eng = aleph_ca.make_generic_engine_2d(rows, cols, _gol_rule)
    py_eng.set_frame(seed)

    for _ in range(4):
        cxx.step()
        py_eng.step()
        assert np.array_equal(cxx.frame(), py_eng.frame())


def test_python_rule_can_use_floats():
    rows, cols = 4, 4
    # Diffusion-like rule: cell becomes the local average.
    def diffuse(state: float, neighbours) -> float:
        return (state + sum(neighbours)) / (1 + len(neighbours))

    eng = aleph_ca.make_generic_engine_2d(rows, cols, diffuse, dtype="float")
    seed = np.zeros((rows, cols), dtype=np.float32)
    seed[2, 2] = 1.0
    eng.set_frame(seed)

    eng.run(3)
    frame = eng.frame()
    # Mass is bounded (diffusion can leak through the toroidal border
    # but should not amplify the initial spike).
    assert frame[2, 2] < 1.0
    assert frame.sum() > 0.0