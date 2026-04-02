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

- Python build tooling for `nanobind`, `scikit-build-core`, and `cmake`

The current repository already vendors `Eigen` and `Spectra`, so the new project
reuses those from the parent repo instead of introducing another dependency chain.

## Local Workflow

Use the repo itself as the live package during development:

```bash
./scripts/install_editable.sh
```

That installs Seahorse in editable mode and builds the native `_core` module in
place. Python changes are then live from this checkout immediately.

After changing C++ or `CMakeLists.txt`, rebuild the native side with:

```bash
./scripts/rebuild_native.sh
```

Run the test suite with:

```bash
./scripts/test.sh
```

Build a regular wheel for installation into another environment or machine with:

```bash
./scripts/build_wheel.sh
```

The scripts use the active environment by default. To target a specific Python
interpreter, prefix the command with `PYTHON=/path/to/python`, for example:

```bash
PYTHON=.venv/bin/python ./scripts/install_editable.sh
```

Editable mode is configured as an in-place build, so:

- Python imports resolve from `src/python`
- the compiled `seahorse._core` extension is rebuilt into `src/python/seahorse`
- static wheel builds stay inside `build/` and `dist/` instead of polluting the
  source package

After reinstalling, restart any already-running Python REPL, notebook kernel,
or service process before checking C++ changes. The compiled `_core` extension
stays loaded for the lifetime of the process, so reinstalling does not hot-swap
an already-imported binary.

Recommended loop:

```bash
./scripts/install_editable.sh
# edit Python freely
./scripts/rebuild_native.sh   # only when C++ changes
./scripts/test.sh
```

When editing only Python, you usually do not need to reinstall anything.
When editing C++, rerun `rebuild_native.sh`, then restart the Python process
you use for testing.

## CI-built Wheels

`.github/workflows/seahorse-wheels.yml` runs the test suite on pull requests and
pushes, and on pushes it also builds a wheel artifact, installs that wheel into
a clean environment, and reruns the tests.

That gives us a path to use a prebuilt binary instead of compiling locally:

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
