from __future__ import annotations

import numpy as np
import pytest

from seahorse import TrigBasis, TrigMode, boundary_penalty, regularization_penalty


def test_trig_basis_returns_expected_shape() -> None:
    t = np.linspace(0.0, 1.0, 17)
    basis = TrigBasis(t=t, max_frequency=3.0, basis_size=4)

    coeffs = np.zeros(basis.num_coeffs)
    control = basis.control(coeffs)

    assert control.shape == t.shape


@pytest.mark.parametrize(
    ("mode", "per_basis"),
    [
        (TrigMode.AMPLITUDE, 1),
        (TrigMode.AMP_FREQ, 2),
        (TrigMode.AMP_PHASE, 2),
        (TrigMode.AMP_FREQ_PHASE, 3),
    ],
)
def test_num_coeffs_tracks_mode(mode: TrigMode, per_basis: int) -> None:
    basis = TrigBasis(
        t=np.linspace(0.0, 1.0, 9),
        max_frequency=4.0,
        mode=mode,
        basis_size=3,
    )
    assert basis.num_coeffs_per_basis == per_basis
    assert basis.num_coeffs == 1 + 3 * per_basis


def test_generate_new_basis_preserves_configuration_but_resamples() -> None:
    rng = np.random.default_rng(123)
    basis = TrigBasis(
        t=np.linspace(0.0, 1.0, 9),
        max_frequency=4.0,
        mode=TrigMode.AMPLITUDE,
        basis_size=3,
        rng=rng,
    )

    new_basis = basis.generate_new_basis()

    assert new_basis is not basis
    assert new_basis.mode is basis.mode
    assert new_basis.basis_size == basis.basis_size
    assert new_basis.max_frequency == basis.max_frequency
    assert not np.array_equal(new_basis._fixed_frequencies, basis._fixed_frequencies)


def test_control_is_scaled_to_requested_max_amplitude() -> None:
    basis = TrigBasis(
        t=np.linspace(0.0, 1.0, 32),
        max_frequency=3.0,
        mode=TrigMode.AMP_FREQ_PHASE,
        basis_size=2,
        max_amplitude=0.75,
    )
    coeffs = np.ones(basis.num_coeffs)

    control = basis.control(coeffs)

    assert np.max(np.abs(control)) <= 0.75 + 1e-12


def test_control_raises_for_wrong_shape() -> None:
    basis = TrigBasis(t=np.linspace(0.0, 1.0, 8), max_frequency=2.0)

    with pytest.raises(ValueError, match="Expected coefficient vector"):
        basis.control(np.zeros(basis.num_coeffs + 1))


def test_penalties_are_callable_and_non_negative() -> None:
    control = np.array([-2.0, -0.5, 0.25, 3.0])
    boundary = boundary_penalty(-1.0, 1.0)
    regularization = regularization_penalty()

    assert boundary(control) >= 0.0
    assert regularization(control) >= 0.0

