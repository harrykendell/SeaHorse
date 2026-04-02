"""Typed specifications for the Python-facing Seahorse API."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

import numpy as np


def _load_core():
    """Import the compiled extension or raise a helpful runtime error."""

    try:
        from . import _core
    except ImportError as exc:  # pragma: no cover - depends on local build state
        raise RuntimeError(
            "The Seahorse native extension is not available. Build the project "
            "with `pip install -e .` inside `seahorse/` first."
        ) from exc
    return _core


class PotentialMode(str, Enum):
    """How a sampled potential should respond to the scalar control signal."""

    STATIC = "static"
    AMPLITUDE_SCALED = "amplitude_scaled"
    SHIFTED = "shifted"


@dataclass(frozen=True)
class GridSpec:
    """Specification for a uniform one-dimensional spatial grid."""

    dim: int
    xmin: float
    xmax: float

    def to_core(self):
        """Build the native ``Grid1D`` object for this grid specification.

        Returns
        -------
        seahorse._core.Grid1D
            Native grid built from the stored dimension and bounds.
        """

        core = _load_core()
        return core.Grid1D(int(self.dim), float(self.xmin), float(self.xmax))


@dataclass(frozen=True)
class TimeSpec:
    """Uniform time discretisation for propagation and optimisation."""

    dt: float
    num_steps: int

    def array(self) -> np.ndarray:
        """Return the inclusive time grid with ``num_steps + 1`` samples.

        Returns
        -------
        numpy.ndarray
            Float64 array containing the full time grid from ``0`` to
            ``dt * num_steps``.
        """

        return np.linspace(0.0, self.dt * self.num_steps, self.num_steps + 1)

    def control_times(self) -> np.ndarray:
        """Return the sample times associated with the control amplitudes.

        Returns
        -------
        numpy.ndarray
            Float64 array containing one sample time per control value.
        """

        if self.num_steps <= 0:
            return np.zeros(0, dtype=np.float64)
        return np.linspace(0.0, self.dt * self.num_steps, self.num_steps)


@dataclass(frozen=True)
class PotentialSpec:
    """Sampled potential values and the rule used to apply the control signal."""

    values: np.ndarray
    mode: PotentialMode = PotentialMode.STATIC

    def to_core(self, grid):
        """Build the native ``Potential1D`` object bound to ``grid``.

        Parameters
        ----------
        grid:
            Native grid object on which the potential samples are defined.

        Returns
        -------
        seahorse._core.Potential1D
            Native potential configured with the stored values and mode.
        """

        core = _load_core()
        values = np.asarray(self.values, dtype=np.float64)
        if self.mode is PotentialMode.STATIC:
            return core.Potential1D.constant(grid, values)
        if self.mode is PotentialMode.AMPLITUDE_SCALED:
            return core.Potential1D.amplitude_scaled(grid, values)
        if self.mode is PotentialMode.SHIFTED:
            return core.Potential1D.shifted(grid, values)
        raise ValueError(f"Unsupported potential mode: {self.mode!r}")


@dataclass(frozen=True)
class TransferTask:
    """Complete propagation task tying together grid, time, and potential."""

    grid: GridSpec
    time: TimeSpec
    potential: PotentialSpec
    use_absorbing_boundary: bool = True

    def build_core(self):
        """Build the native objects used by the C++ core.

        Returns
        -------
        tuple[seahorse._core.Grid1D, seahorse._core.Potential1D]
            Native grid and potential objects derived from the task
            specification.
        """

        grid = self.grid.to_core()
        potential = self.potential.to_core(grid)
        return grid, potential
