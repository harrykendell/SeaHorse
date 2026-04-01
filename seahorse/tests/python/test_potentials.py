from __future__ import annotations

import numpy as np

from seahorse.potentials import box_window, cosine_lattice, planck_taper


def test_box_window_is_zero_on_boundaries_and_one_inside() -> None:
    x = np.array([-2.0, -1.0, 0.0, 1.0, 2.0])
    window = box_window(x, -1.0, 1.0)
    np.testing.assert_allclose(window, np.array([0.0, 0.0, 1.0, 0.0, 0.0]))


def test_planck_taper_is_symmetric() -> None:
    values = np.ones(12)
    tapered = planck_taper(values, taper_ratio=0.5)
    np.testing.assert_allclose(tapered, tapered[::-1])
    assert tapered[0] == 0.0
    assert tapered[5] > tapered[0]


def test_planck_taper_returns_copy_when_width_is_zero() -> None:
    values = np.arange(5.0)
    tapered = planck_taper(values, taper_ratio=0.0)
    np.testing.assert_allclose(tapered, values)
    assert tapered is not values


def test_cosine_lattice_stays_between_zero_and_depth() -> None:
    x = np.linspace(-np.pi, np.pi, 17)
    lattice = cosine_lattice(x, depth=3.0, wave_number=1.0)
    assert np.min(lattice) >= -1e-12
    assert np.max(lattice) <= 3.0 + 1e-12

