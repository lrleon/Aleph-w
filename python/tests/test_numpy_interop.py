"""
NumPy interop tests.

Two contracts that must hold for the bindings to feel native:

  * `frame()` returns an owning copy — mutating it must NOT affect the engine.
  * `frame_view()` is a zero-copy view on the engine's current buffer.
"""

from __future__ import annotations

import numpy as np
import pytest

import aleph_ca


def test_frame_returns_independent_copy():
    eng = aleph_ca.make_game_of_life_engine(rows=4, cols=4)
    arr = eng.frame()
    arr[0, 0] = 99
    # The mutation must not bleed back into the engine.
    again = eng.frame()
    assert again[0, 0] == 0


def test_frame_view_is_zero_copy_before_step():
    eng = aleph_ca.make_game_of_life_engine(rows=8, cols=8)
    seed = np.zeros((8, 8), dtype=np.int32)
    seed[1, 1] = 1
    eng.set_frame(seed)

    view = eng.frame_view()
    # Same shape and dtype as the engine's lattice.
    assert view.shape == (8, 8)
    assert view.dtype == np.int32
    # And it sees the same content as the eager copy.
    assert view[1, 1] == 1


def test_set_frame_validates_shape():
    eng = aleph_ca.make_game_of_life_engine(rows=4, cols=4)
    wrong_shape = np.zeros((5, 5), dtype=np.int32)
    with pytest.raises(ValueError):
        eng.set_frame(wrong_shape)


def test_set_frame_accepts_non_contiguous_input():
    eng = aleph_ca.make_game_of_life_engine(rows=4, cols=4)
    big = np.zeros((8, 8), dtype=np.int32)
    big[2, 2] = 1
    # The forcecast flag on the binding lets us hand in a strided slice.
    eng.set_frame(big[2:6, 2:6])
    frame = eng.frame()
    assert frame[0, 0] == 1
    assert frame.sum() == 1


def test_wolfram_frame_is_1d():
    eng = aleph_ca.make_wolfram_engine(rule_no=110, width=20)
    assert eng.frame().shape == (20,)
    assert eng.frame_view().shape == (20,)