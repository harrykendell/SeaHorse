# Seahorse Next

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

## CI-built Wheels

`.github/workflows/seahorse-next-wheels.yml` is intended to build wheel artifacts
for the native extension in GitHub Actions.

- Pull requests and pushes upload wheels as workflow artifacts
- Version tags like `v0.1.0` can also publish those wheels as release assets

That gives us a path to use prebuilt binaries instead of compiling locally:

```bash
pip install seahorse_next-0.1.0-...whl
```
