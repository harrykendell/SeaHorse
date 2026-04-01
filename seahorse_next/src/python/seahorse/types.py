from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

import numpy as np


def _load_core():
    try:
        from . import _core
    except ImportError as exc:  # pragma: no cover - depends on local build state
        raise RuntimeError(
            "The Seahorse native extension is not available. Build the project "
            "with `pip install -e .` inside `seahorse_next/` first."
        ) from exc
    return _core


class PotentialMode(str, Enum):
    STATIC = "static"
    AMPLITUDE_SCALED = "amplitude_scaled"
    SHIFTED = "shifted"


@dataclass(frozen=True)
class GridSpec:
    dim: int
    xmin: float
    xmax: float

    def to_core(self):
        core = _load_core()
        return core.Grid1D(int(self.dim), float(self.xmin), float(self.xmax))


@dataclass(frozen=True)
class TimeSpec:
    dt: float
    num_steps: int

    def array(self) -> np.ndarray:
        return np.linspace(0.0, self.dt * self.num_steps, self.num_steps + 1)


@dataclass(frozen=True)
class PotentialSpec:
    values: np.ndarray
    mode: PotentialMode = PotentialMode.STATIC

    def to_core(self, grid):
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
    grid: GridSpec
    time: TimeSpec
    potential: PotentialSpec
    use_absorbing_boundary: bool = True

    def build_core(self):
        grid = self.grid.to_core()
        potential = self.potential.to_core(grid)
        return grid, potential

