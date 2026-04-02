"""Control-basis primitives used by the Python-side optimisation layer."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

import numpy as np

from .potentials import planck_taper


class TrigMode(str, Enum):
    """Coefficient layout used by :class:`TrigBasis`."""

    AMPLITUDE = "amplitude"
    AMP_FREQ = "amp_freq"
    AMP_PHASE = "amp_phase"
    AMP_FREQ_PHASE = "amp_freq_phase"


@dataclass
class TrigBasis:
    """Randomised trigonometric basis for generating bounded control waveforms."""

    t: np.ndarray
    max_frequency: float
    mode: TrigMode = TrigMode.AMP_FREQ_PHASE
    basis_size: int = 10
    max_amplitude: float = 1.0
    frequency_power: int = 5
    taper_ratio: float = 1.0 / 6.0
    rng: np.random.Generator | None = None
    _fixed_frequencies: np.ndarray = field(init=False, repr=False)
    _fixed_phases: np.ndarray = field(init=False, repr=False)

    def __post_init__(self) -> None:
        self.t = np.asarray(self.t, dtype=np.float64)
        if self.t.ndim != 1:
            raise ValueError("TrigBasis expects a 1D time grid")
        self.rng = self.rng or np.random.default_rng()
        self._resample_static_parameters()

    def _resample_static_parameters(self) -> None:
        generator = self.rng or np.random.default_rng()
        self._fixed_frequencies = np.power(
            generator.uniform(-1.0, 1.0, size=self.basis_size),
            self.frequency_power,
        ) * self.max_frequency
        self._fixed_phases = generator.uniform(-1.0, 1.0, size=self.basis_size)

    @property
    def num_coeffs_per_basis(self) -> int:
        if self.mode is TrigMode.AMPLITUDE:
            return 1
        if self.mode in (TrigMode.AMP_FREQ, TrigMode.AMP_PHASE):
            return 2
        return 3

    @property
    def num_coeffs(self) -> int:
        return 1 + self.basis_size * self.num_coeffs_per_basis

    def sample_coefficients(self, rng: np.random.Generator | None = None) -> np.ndarray:
        """Draw a coefficient vector in ``[-1, 1]`` matching this basis layout.

        Parameters
        ----------
        rng:
            Optional random number generator used to sample the coefficients.

        Returns
        -------
        numpy.ndarray
            Float64 vector with shape ``(num_coeffs,)``.
        """

        generator = rng or np.random.default_rng()
        return generator.uniform(-1.0, 1.0, size=self.num_coeffs)

    def generate_new_basis(self) -> "TrigBasis":
        """Clone this basis configuration while resampling its fixed terms.

        Returns
        -------
        TrigBasis
            New basis object with the same configuration and newly sampled
            fixed frequencies and phases.
        """

        return TrigBasis(
            t=self.t.copy(),
            max_frequency=self.max_frequency,
            mode=self.mode,
            basis_size=self.basis_size,
            max_amplitude=self.max_amplitude,
            frequency_power=self.frequency_power,
            taper_ratio=self.taper_ratio,
            rng=self.rng,
        )

    def control(self, coeffs) -> np.ndarray:
        """Map a coefficient vector onto a tapered control waveform on ``t``.

        Parameters
        ----------
        coeffs:
            Coefficient vector with shape ``(num_coeffs,)``.

        Returns
        -------
        numpy.ndarray
            Float64 control waveform sampled on ``t`` and scaled to respect
            ``max_amplitude``.

        Raises
        ------
        ValueError
            If ``coeffs`` does not match the expected coefficient shape.
        """

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
            local = coeffs_arr[offset : offset + self.num_coeffs_per_basis]
            amp = local[0]

            if self.mode is TrigMode.AMPLITUDE:
                freq = self._fixed_frequencies[i]
                phase = self._fixed_phases[i]
            elif self.mode is TrigMode.AMP_FREQ:
                freq = self.max_frequency * np.power(local[1], self.frequency_power)
                phase = self._fixed_phases[i]
            elif self.mode is TrigMode.AMP_PHASE:
                freq = self._fixed_frequencies[i]
                phase = local[1]
            else:
                freq = self.max_frequency * np.power(local[1], self.frequency_power)
                phase = local[2]

            control += planck_taper(
                amp * np.cos(2.0 * np.pi * (freq * t + phase)),
                taper_ratio=self.taper_ratio,
            )

        max_abs = np.max(np.abs(control))
        if max_abs == 0.0:
            return control
        return self.max_amplitude * coeffs_arr[0] * control / max_abs
