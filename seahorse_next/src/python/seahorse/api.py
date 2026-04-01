from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np

if TYPE_CHECKING:
    from .types import TransferTask


def _load_core():
    try:
        from . import _core
    except ImportError as exc:  # pragma: no cover - depends on local build state
        raise RuntimeError(
            "The Seahorse native extension is not available. Build the project "
            "with `pip install -e .` inside `seahorse_next/` first."
        ) from exc
    return _core


def solve_spectrum(task: "TransferTask", num_states: int):
    core = _load_core()
    grid, potential = task.build_core()
    return core.solve_spectrum(grid, potential, int(num_states))


def make_propagator(task: "TransferTask"):
    core = _load_core()
    grid, potential = task.build_core()
    return core.SplitStep1D(
        grid,
        potential,
        float(task.time.dt),
        bool(task.use_absorbing_boundary),
    )


def propagate(
    task: "TransferTask",
    psi0,
    control,
    *,
    store_path: bool = False,
):
    propagator = make_propagator(task)
    psi0_arr = np.asarray(psi0, dtype=np.complex128)
    control_arr = np.asarray(control, dtype=np.float64)
    return propagator.propagate(psi0_arr, control_arr, bool(store_path))


def evaluate_control(
    task: "TransferTask",
    psi0,
    psit,
    control,
):
    core = _load_core()
    grid, potential = task.build_core()
    psi0_arr = np.asarray(psi0, dtype=np.complex128)
    psit_arr = np.asarray(psit, dtype=np.complex128)
    control_arr = np.asarray(control, dtype=np.float64)
    return core.evaluate_control(
        grid,
        potential,
        float(task.time.dt),
        psi0_arr,
        psit_arr,
        control_arr,
        bool(task.use_absorbing_boundary),
    )

