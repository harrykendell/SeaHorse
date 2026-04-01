# Seahorse Monorepo

This repository currently contains two projects:

- [`seahorse/`](/home/ae19663/Desktop/seahorse/seahorse): the current Python-first library with a nanobind-powered C++ core
- [`seahorse-old/`](/home/ae19663/Desktop/seahorse/seahorse-old): the legacy C++ application and GUI, kept operational for reference and migration work

Shared third-party numerical dependencies remain in [`libs/`](/home/ae19663/Desktop/seahorse/libs) because both projects use them.
Legacy GUI-specific dependencies now live inside [`seahorse-old/libs/`](/home/ae19663/Desktop/seahorse/seahorse-old/libs).

## Main Project

The active package is in [`seahorse/`](/home/ae19663/Desktop/seahorse/seahorse).

Typical development flow:

```bash
cd seahorse
./scripts/build_and_install_local.sh
python -m pytest tests/python
```

## Legacy Project

The old application has been moved under [`seahorse-old/`](/home/ae19663/Desktop/seahorse/seahorse-old).

Typical legacy build flow:

```bash
cd seahorse-old
make all -j8
make gui -j8
```
