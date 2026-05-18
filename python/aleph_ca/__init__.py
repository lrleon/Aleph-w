"""
aleph_ca — Python bindings for the Aleph-w cellular-automata module.

Phase 18 of `cell-automata-industrial-roadmap.md`. The C++ side lives in
the compiled extension `aleph_ca._core`; this façade re-exports the
canonical surface so users only need:

    >>> import aleph_ca
    >>> eng = aleph_ca.GameOfLifeEngine(8, 8)
    >>> eng.step()
    >>> eng.frame().shape
    (8, 8)

The bindings cover Game of Life, elementary Wolfram rules, the Drossel
& Schwabl forest-fire model, a Python-callback engine for prototyping,
checkpoint save/load, and PNG/SVG export. Anything else exposed in the
underlying C++ library still requires a small native shim.
"""

from __future__ import annotations

from typing import Callable, Sequence

from . import _core
from ._core import (
    CheckpointOptions,
    ForestFireEngine,
    GameOfLifeEngine,
    GenericEngine2DFloat,
    GenericEngine2DInt,
    RGB8,
    WolframEngine,
    make_forest_fire_engine,
    make_game_of_life_engine,
    make_generic_engine_2d_float,
    make_generic_engine_2d_int,
    make_wolfram_engine,
)

__version__ = "2.0.0b0"

__all__ = [
    "CheckpointOptions",
    "ForestFireEngine",
    "GameOfLifeEngine",
    "GenericEngine2DFloat",
    "GenericEngine2DInt",
    "RGB8",
    "WolframEngine",
    "__version__",
    "load_checkpoint_into",
    "make_forest_fire_engine",
    "make_game_of_life_engine",
    "make_generic_engine_2d",
    "make_wolfram_engine",
    "save_checkpoint",
    "write_png_bytes",
    "write_svg_bytes",
]


# ---------------------------------------------------------------------------
# Engine-type dispatch tables.
#
# The C++ side exports one save/load function per engine instantiation
# because templates cannot be specialised at runtime. We hide that detail
# behind dict lookups so users see a clean `save_checkpoint(engine, path)`
# call regardless of the underlying engine type.
# ---------------------------------------------------------------------------

_SAVE_DISPATCH = {
    GameOfLifeEngine: _core.save_checkpoint_gol,
    WolframEngine: _core.save_checkpoint_wolfram,
    ForestFireEngine: _core.save_checkpoint_forest_fire,
}

_LOAD_DISPATCH = {
    GameOfLifeEngine: _core.load_checkpoint_into_gol,
    WolframEngine: _core.load_checkpoint_into_wolfram,
    ForestFireEngine: _core.load_checkpoint_into_forest_fire,
}

_PNG_DISPATCH = {
    GameOfLifeEngine: _core.write_png_bytes_gol,
    ForestFireEngine: _core.write_png_bytes_forest_fire,
    GenericEngine2DInt: _core.write_png_bytes_generic_int,
}

_SVG_DISPATCH = {
    GameOfLifeEngine: _core.write_svg_bytes_gol,
    ForestFireEngine: _core.write_svg_bytes_forest_fire,
}


def save_checkpoint(engine, path: str, options: CheckpointOptions | None = None) -> None:
    """Persist `engine` to `path` atomically (Phase 17 format v2).

    The format is binary, self-describing and supports optional DEFLATE
    compression via :class:`CheckpointOptions`.
    """
    fn = _SAVE_DISPATCH.get(type(engine))
    if fn is None:
        raise TypeError(f"save_checkpoint: unsupported engine type {type(engine).__name__}")
    fn(engine, str(path), options if options is not None else CheckpointOptions())


def load_checkpoint_into(engine, path: str) -> None:
    """Restore an engine's state from a checkpoint in place.

    The engine must have been built with extents matching the snapshot;
    failure to do so raises ``RuntimeError`` from the C++ layer.
    """
    fn = _LOAD_DISPATCH.get(type(engine))
    if fn is None:
        raise TypeError(f"load_checkpoint_into: unsupported engine type {type(engine).__name__}")
    fn(engine, str(path))


def write_png_bytes(engine) -> bytes:
    """Render `engine`'s current frame as a PNG bytestring."""
    fn = _PNG_DISPATCH.get(type(engine))
    if fn is None:
        raise TypeError(f"write_png_bytes: unsupported engine type {type(engine).__name__}")
    return fn(engine)


def write_svg_bytes(engine) -> bytes:
    """Render `engine`'s current frame as an SVG bytestring."""
    fn = _SVG_DISPATCH.get(type(engine))
    if fn is None:
        raise TypeError(f"write_svg_bytes: unsupported engine type {type(engine).__name__}")
    return fn(engine)


def make_generic_engine_2d(
    rows: int,
    cols: int,
    rule: Callable[[int, Sequence[int]], int],
    *,
    dtype: str = "int",
):
    """Build a 2D engine driven by a Python rule callable.

    The callable receives the current cell value and a Python list of the
    eight Moore neighbours, and must return the next cell value. Pick
    ``dtype="int"`` for combinatorial CAs (Game of Life-like) or
    ``dtype="float"`` for continuous numerics.
    """
    if dtype == "int":
        return make_generic_engine_2d_int(rows, cols, rule)
    if dtype == "float":
        return make_generic_engine_2d_float(rows, cols, rule)
    raise ValueError(f"unsupported dtype: {dtype!r} (expected 'int' or 'float')")