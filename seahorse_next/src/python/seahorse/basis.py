from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .potentials import planck_taper


@dataclass
class TrigBasis:
    t: np.ndarray
    max_frequency: float
    basis_size: int = 10
    max_amplitude: float = 1.0
    frequency_power: int = 5

    @property
    def num_coeffs_per_basis(self) -> int:
        return 3

    @property
    def num_coeffs(self) -> int:
        return 1 + self.basis_size * self.num_coeffs_per_basis

    def sample_coefficients(self, rng: np.random.Generator | None = None) -> np.ndarray:
        generator = rng or np.random.default_rng()
        return generator.uniform(-1.0, 1.0, size=self.num_coeffs)

    def control(self, coeffs) -> np.ndarray:
        coeffs_arr = np.asarray(coeffs, dtype=np.float64)
        if coeffs_arr.shape != (self.num_coeffs,):
            raise ValueError(
                f"Expected coefficient vector of shape {(self.num_coeffs,)}, "
                f"got {coeffs_arr.shape}"
            )

        t = np.asarray(self.t, dtype=np.float64)
        control = np.zeros_like(t)

        for i in range(self.basis_size):
            offset = 1 + i * self.num_coeffs_per_basis
            amp, raw_freq, phase = coeffs_arr[offset : offset + self.num_coeffs_per_basis]
            freq = self.max_frequency * np.power(abs(raw_freq), self.frequency_power)
            control += planck_taper(amp * np.cos(2.0 * np.pi * (freq * t + phase)))

        max_abs = np.max(np.abs(control))
        if max_abs == 0.0:
            return control
        return self.max_amplitude * coeffs_arr[0] * control / max_abs

