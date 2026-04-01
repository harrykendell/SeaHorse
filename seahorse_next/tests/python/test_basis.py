from __future__ import annotations

import numpy as np

from seahorse.basis import TrigBasis


def test_trig_basis_returns_expected_shape() -> None:
    t = np.linspace(0.0, 1.0, 17)
    basis = TrigBasis(t=t, max_frequency=3.0, basis_size=4)

    coeffs = np.zeros(basis.num_coeffs)
    control = basis.control(coeffs)

    assert control.shape == t.shape

