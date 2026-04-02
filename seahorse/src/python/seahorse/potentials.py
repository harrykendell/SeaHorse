"""Convenience helpers for constructing common one-dimensional potentials."""

from __future__ import annotations

import numpy as np


def box_window(x, xmin: float, xmax: float) -> np.ndarray:
    """Return a rectangular window on the open interval ``(xmin, xmax)``.

    Parameters
    ----------
    x:
        Coordinates at which to sample the window.
    xmin, xmax:
        Lower and upper interval bounds. Samples exactly at the bounds evaluate
        to zero.

    Returns
    -------
    numpy.ndarray
        Float64 array containing ones strictly inside the interval and zeros
        elsewhere.
    """

    x_arr = np.asarray(x, dtype=np.float64)
    return ((x_arr < xmax).astype(float) + (x_arr > xmin).astype(float)) - 1.0


def planck_taper(values, taper_ratio: float = 1.0 / 6.0) -> np.ndarray:
    """Smoothly taper a one-dimensional array to zero near both ends.

    Parameters
    ----------
    values:
        One-dimensional samples to taper.
    taper_ratio:
        Fraction of the array reserved for the rising and falling taper. A
        value of zero returns an unchanged copy.

    Returns
    -------
    numpy.ndarray
        Float64 array with the taper applied multiplicatively.
    """

    arr = np.asarray(values, dtype=np.float64)
    out = np.ones_like(arr)
    width = int(taper_ratio * arr.size / 2)
    if width <= 0:
        return arr.copy()
    idx = np.arange(width, dtype=np.float64)
    taper = 0.5 * (1.0 - np.cos(2.0 * np.pi * idx / (taper_ratio * arr.size)))
    out[:width] = taper
    out[-width:] = taper[::-1]
    return arr * out


def cosine_lattice(
    x, depth: float, wave_number: float, xlimit: tuple[float, float] = (-np.inf, np.inf)
) -> np.ndarray:
    """Return a cosine lattice whose values lie between ``0`` and ``depth``.

    Parameters
    ----------
    x:
        Coordinates at which to sample the lattice.
    depth:
        Maximum lattice depth.
    wave_number:
        Spatial wave number controlling the lattice period.
    xlimit:
        Tuple of lower and upper limits for the lattice domain.

    Returns
    -------
    numpy.ndarray
        Float64 array containing the sampled lattice values.
    """

    x_arr = np.asarray(x, dtype=np.float64)
    box = box_window(x_arr, *xlimit)
    return depth - 0.5 * depth * (np.cos(2.0 * wave_number * x_arr) + 1.0) * box
