# `aleph_ca` notebooks

Tutorial notebooks for the Python bindings.

The canonical source is the `.py` file in *jupytext percent* format. To
work with the notebook in Jupyter, regenerate the `.ipynb` first:

```bash
pip install jupytext
jupytext --to ipynb 01_intro.py
jupyter lab 01_intro.ipynb
```

Edits made in the `.ipynb` can be paired back to the `.py`:

```bash
jupytext --sync 01_intro.ipynb
```

We keep the `.py` flavour as the version-controlled source because it
diffs cleanly and replays bit-for-bit in CI; the `.ipynb` is a
build-time artefact you regenerate locally.

## Available notebooks

| File           | Topics covered                                              |
|----------------|-------------------------------------------------------------|
| `01_intro.py`  | Game of Life blinker, Wolfram rule 30, forest-fire dynamics |