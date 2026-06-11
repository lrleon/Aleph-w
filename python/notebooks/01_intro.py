# ---
# jupyter:
#   jupytext:
#     formats: py:percent,ipynb
#     text_representation:
#       extension: .py
#       format_name: percent
#       format_version: '1.3'
#       jupytext_version: 1.16.0
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# %% [markdown]
# # Aleph-w `aleph_ca` — Introductory tour
#
# This notebook walks through the three canonical entry points exposed by
# Phase 18 of the Aleph-w cellular-automata roadmap:
#
# 1. **Conway's Game of Life** — period-2 blinker in six cells.
# 2. **Wolfram elementary rule 30** — 1D space-time diagram.
# 3. **Drossel & Schwabl forest-fire** — stochastic 2D dynamics with a
#    deterministic per-cell RNG.
#
# The library lives in C++ (`namespace Aleph::CA`); the Python façade
# `aleph_ca` is a thin pybind11 wrapper. Frames come back as
# `numpy.ndarray`, so anything you can do in NumPy applies directly.
#
# This file is the canonical source for the notebook. Pair it with
# Jupytext to round-trip to `.ipynb`:
#
# ```bash
# pip install jupytext
# jupytext --to ipynb 01_intro.py
# ```

# %%
import aleph_ca
import numpy as np
import matplotlib.pyplot as plt

print("aleph_ca version:", aleph_ca.__version__)

# %% [markdown]
# ## 1. Game of Life — period-2 blinker
#
# The blinker is the simplest non-trivial oscillator. A 1×3 row of live
# cells flips to a 3×1 column after one step, and back to the row after
# two. We use this as a smoke test that the engine, the toroidal
# boundary and the Moore neighbourhood are all wired up correctly.

# %%
rows, cols = 6, 6
eng = aleph_ca.make_game_of_life_engine(rows, cols)

seed = np.zeros((rows, cols), dtype=np.int32)
seed[2, 1:4] = 1
eng.set_frame(seed)

fig, axes = plt.subplots(1, 3, figsize=(9, 3))
for ax, step in zip(axes, range(3)):
    ax.imshow(eng.frame(), cmap="gray_r", vmin=0, vmax=1)
    ax.set_title(f"step {step}")
    ax.axis("off")
    if step < 2:
        eng.step()
plt.tight_layout()
plt.show()

# %% [markdown]
# ## 2. Wolfram elementary rule 30 — 1D space-time diagram
#
# Rule 30 produces apparent randomness from a single seed cell. We stack
# each step as a row of an image to recover the classic Wolfram triangle.

# %%
width = 201
steps = 100

eng = aleph_ca.make_wolfram_engine(rule_no=30, width=width)
seed = np.zeros(width, dtype=np.int32)
seed[width // 2] = 1
eng.set_frame(seed)

history_rows = [eng.frame().copy()]
for _ in range(steps):
    eng.step()
    history_rows.append(eng.frame().copy())
history = np.vstack(history_rows)

plt.figure(figsize=(8, 5))
plt.imshow(history, cmap="binary", interpolation="nearest", aspect="auto")
plt.title("Wolfram rule 30 — 100 steps, single-cell seed")
plt.xlabel("cell")
plt.ylabel("step")
plt.show()

# %% [markdown]
# ## 3. Forest-fire (Drossel & Schwabl 1992)
#
# Cell states: `0 = empty`, `1 = tree`, `2 = burning`. We seed a partly
# populated grid and watch a few decades of dynamics. The trajectory is
# reproducible because every per-cell RNG stream is derived from
# `(master_seed, step, coord)`.

# %%
rows, cols = 64, 64
rng = np.random.default_rng(seed=7)
seed = (rng.random((rows, cols)) < 0.6).astype(np.int32)  # ~60% forested

eng = aleph_ca.make_forest_fire_engine(
    rows,
    cols,
    p_growth=0.02,
    p_lightning=0.0005,
    seed=12345,
)
eng.set_frame(seed)

frames = []
for _ in range(20):
    eng.step()
    frames.append(eng.frame().copy())

fig, axes = plt.subplots(2, 5, figsize=(12, 5))
for ax, step in zip(axes.ravel(), range(2, 20, 2)):
    ax.imshow(frames[step], cmap="YlOrBr", vmin=0, vmax=2)
    ax.set_title(f"step {step + 1}")
    ax.axis("off")
plt.tight_layout()
plt.show()

# %% [markdown]
# ## Where to go next
#
# - `GenericEngine2D` lets you prototype a new rule directly in Python
#   (slow but expressive).
# - `aleph_ca.save_checkpoint` / `load_checkpoint_into` give you
#   crash-safe snapshots — useful for long-running explorations.
# - For production performance, register a C++ rule in `tpl_ca_rule.H`
#   and expose it through `python/src/bindings.cc`.
#
# See `docs/cellular_automata.md` for the full design of the CA module.