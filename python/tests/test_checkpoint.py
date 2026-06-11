"""
Checkpoint round-trip tests.

The on-disk format is Phase-17 v2 (atomic rename + optional DEFLATE).
We verify that:

  * uncompressed and compressed snapshots both round-trip byte-for-byte;
  * the step counter is preserved across save/load;
  * mismatched extents are rejected by the loader.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
import pytest

import aleph_ca


def _make_seeded_engine() -> aleph_ca.GameOfLifeEngine:
    eng = aleph_ca.make_game_of_life_engine(rows=8, cols=8)
    seed = np.zeros((8, 8), dtype=np.int32)
    # Glider in the top-left corner.
    seed[1, 2] = 1
    seed[2, 3] = 1
    seed[3, 1] = 1
    seed[3, 2] = 1
    seed[3, 3] = 1
    eng.set_frame(seed)
    eng.run(5)
    return eng


def test_uncompressed_round_trip(tmp_path: Path):
    eng = _make_seeded_engine()
    original_frame = eng.frame()
    original_steps = eng.steps_run()

    path = tmp_path / "snapshot.ckpt"
    aleph_ca.save_checkpoint(eng, str(path))
    assert path.exists()

    restored = aleph_ca.make_game_of_life_engine(rows=8, cols=8)
    aleph_ca.load_checkpoint_into(restored, str(path))
    assert restored.steps_run() == original_steps
    assert np.array_equal(restored.frame(), original_frame)


def test_compressed_round_trip(tmp_path: Path):
    eng = _make_seeded_engine()
    original_frame = eng.frame()

    opts = aleph_ca.CheckpointOptions()
    opts.compress = True
    opts.level = 6

    path = tmp_path / "snapshot.ckpt.gz"
    aleph_ca.save_checkpoint(eng, str(path), opts)
    assert path.exists()

    restored = aleph_ca.make_game_of_life_engine(rows=8, cols=8)
    aleph_ca.load_checkpoint_into(restored, str(path))
    assert np.array_equal(restored.frame(), original_frame)


def test_extents_mismatch_rejected(tmp_path: Path):
    eng = _make_seeded_engine()
    path = tmp_path / "snapshot.ckpt"
    aleph_ca.save_checkpoint(eng, str(path))

    wrong = aleph_ca.make_game_of_life_engine(rows=4, cols=4)
    with pytest.raises(RuntimeError):
        aleph_ca.load_checkpoint_into(wrong, str(path))


def test_unsupported_engine_type_rejected(tmp_path: Path):
    """save_checkpoint dispatches on engine type; passing something
    unknown should fail loudly rather than silently."""

    class FakeEngine:
        pass

    with pytest.raises(TypeError):
        aleph_ca.save_checkpoint(FakeEngine(), str(tmp_path / "x.ckpt"))