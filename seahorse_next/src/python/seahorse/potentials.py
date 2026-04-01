from __future__ import annotations

import numpy as np


def box_window(x, xmin: float, xmax: float) -> np.ndarray:
    x_arr = np.asarray(x, dtype=np.float64)
    return ((x_arr < xmax).astype(float) + (x_arr > xmin).astype(float)) - 1.0


def planck_taper(values, taper_ratio: float = 1.0 / 6.0) -> np.ndarray:
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


def cosine_lattice(x, depth: float, wave_number: float) -> np.ndarray:
    x_arr = np.asarray(x, dtype=np.float64)
    return depth - 0.5 * depth * (np.cos(2.0 * wave_number * x_arr) + 1.0)

