# SeaHorse
Quantum Simulation and Optimal Control.

## Included libraries

<b>These are included as submodules. </b>
* <ins>Eigen:</ins>
  
	Used for fast linear algebra calculations. The MKL backend allows it to go as fast as possible :)

* <ins>Spectra:</ins>

	Used for generating eigenvectors.
	This is substantially faster than Eigen's default implementation as it allows us to request only a specific number instead of the full specturm.

* <ins>Raylib + Raygui:</ins>

	Used for the graphical interface.
	This is only included for the gui target and now lives under `seahorse-old/libs/`.

## Required libraries

<b>These should live on your path somewhere. </b>
* <ins>MKL:</ins>
  
	Used for fast maths. We could support not needing this in the future?

## Installing
On first run you need to initialise and update the submodules:

`git submodule update --init --recursive`

## Building
The legacy project now lives entirely inside `seahorse-old/`.

Build from this directory:

```bash
cd seahorse-old
```

We use the make build system - ideally this should separate the seahorse parts and user parts to reduce compile times but that needs some thoughful design

Any *.cpp in the projects file can be built using:

`make * -j8`

or all at once with

`make all -j8`

which will create executables in `seahorse-old/bin`.

The legacy project expects:

* shared numerical dependencies in `../libs/`
* GUI dependencies in `./libs/`


## On the BC4 Cluster
* Load modules:
        - tools/cmake/3.20.0
        - languages/gcc/10.4.0
- we need to ensure configuration with these properties
- we need to ensure the default cmake/g++ are the ones we just loaded
