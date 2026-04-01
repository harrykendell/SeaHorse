# Seahorse

Greenfield rewrite of SeaHorse as a Python-first library with a small C++ core.

## Goals

- Keep numerically heavy kernels in C++
- Expose a NumPy-friendly Python API
- Use `nanobind` for a thin binding layer
- Keep build dependencies small and optional
- Avoid callback-driven Python/C++ boundaries in hot loops

## Initial Scope

- 1D spatial grids
- Typed potentials: static, amplitude-scaled, shifted
- Sparse low-lying eigenspectrum solves
- Split-step propagation
- State-transfer objective evaluation
- Python-side basis generation and optimisation scaffolding

## Build Notes

This scaffold expects:

- `nanobind`
- `scikit-build-core`
- an available `cmake`

The current repository already vendors `Eigen` and `Spectra`, so the new project
reuses those from the parent repo instead of introducing another dependency chain.

For local development, the default flow is an editable install:

```bash
./scripts/build_and_install_local.sh
```

If you already have a Conda environment activated, the blank form above
installs into that active environment automatically.

If you want to validate the actual wheel artifact locally instead, use:

```bash
./scripts/build_and_install_local.sh --wheel
```

You can target a specific interpreter with:

```bash
./scripts/build_and_install_local.sh --python .venv/bin/python
```

Or install straight into a Conda environment without activating it first:

```bash
./scripts/build_and_install_local.sh --conda-env my-env
```

## CI-built Wheels

`.github/workflows/seahorse-wheels.yml` is intended to build wheel artifacts
for the native extension in GitHub Actions.

- Pull requests and pushes upload wheels as workflow artifacts
- Version tags like `v0.1.0` can also publish those wheels as release assets

That gives us a path to use prebuilt binaries instead of compiling locally:

```bash
pip install seahorse-0.1.0-...whl
```

After installing a wheel, an end-to-end sanity check is:

```bash
python examples/wheel_demo.py
```

For a more interactive walkthrough, open:

```bash
jupyter lab examples/quick_demo.ipynb
```
