from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .api import evaluate_control
from .basis import TrigBasis
from .types import TransferTask


@dataclass(frozen=True)
class DCRABConfig:
    basis_size: int = 10
    max_frequency: float = 5.0
    max_amplitude: float = 1.0
    random_starts: int = 8
    seed: int | None = None


class DCRABSketchOptimizer:
    """Minimal Python-side scaffold for the future dCRAB implementation."""

    def __init__(
        self,
        task: TransferTask,
        psi0,
        psit,
        config: DCRABConfig,
    ) -> None:
        self.task = task
        self.psi0 = np.asarray(psi0, dtype=np.complex128)
        self.psit = np.asarray(psit, dtype=np.complex128)
        self.config = config
        self.rng = np.random.default_rng(config.seed)
        self.basis = TrigBasis(
            t=task.time.array(),
            max_frequency=config.max_frequency,
            basis_size=config.basis_size,
            max_amplitude=config.max_amplitude,
        )

    def sample_coefficients(self) -> np.ndarray:
        return self.basis.sample_coefficients(self.rng)

    def control_from_coefficients(self, coeffs) -> np.ndarray:
        return self.basis.control(coeffs)

    def evaluate_coefficients(self, coeffs):
        control = self.control_from_coefficients(coeffs)
        return evaluate_control(self.task, self.psi0, self.psit, control)

    def random_search(self):
        best = None
        for _ in range(self.config.random_starts):
            coeffs = self.sample_coefficients()
            trial = self.evaluate_coefficients(coeffs)
            if best is None or trial.cost < best.cost:
                best = trial
        return best

