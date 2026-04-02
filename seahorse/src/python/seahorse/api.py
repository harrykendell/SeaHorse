"""High-level Python wrappers around the native Seahorse extension."""

from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np

if TYPE_CHECKING:
    from .types import TransferTask


def _load_core():
    """Import the compiled extension or raise a helpful runtime error."""

    try:
        from . import _core
    except ImportError as exc:  # pragma: no cover - depends on local build state
        raise RuntimeError(
            "The Seahorse native extension is not available. Build the project "
            "with `pip install -e .` inside `seahorse/` first."
        ) from exc
    return _core


def solve_spectrum(task: "TransferTask", num_states: int):
    """Solve for the lowest-energy eigenstates defined by ``task``.

    Parameters
    ----------
    task:
        Grid, potential, and time configuration describing the physical system.
    num_states:
        Number of eigenpairs to request from the native solver.

    Returns
    -------
    seahorse._core.SpectrumResult
        Native result containing the eigenvalues and eigenvectors.
    """

    core = _load_core()
    grid, potential = task.build_core()
    return core.solve_spectrum(grid, potential, int(num_states))


def make_propagator(task: "TransferTask"):
    """Create a native split-step propagator for ``task``.

    Parameters
    ----------
    task:
        Grid, time, potential, and boundary configuration for the simulation.

    Returns
    -------
    seahorse._core.SplitStep1D
        Native propagator configured from the task specification.
    """

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
    """Propagate ``psi0`` under ``control`` using the native split-step solver.

    Parameters
    ----------
    task:
        Simulation configuration for the propagation.
    psi0:
        Initial state vector. It is coerced to ``complex128`` before crossing the
        Python/C++ boundary.
    control:
        One scalar control value per propagation step. Values are coerced to
        ``float64``.
    store_path:
        When ``True``, the native result includes the full state trajectory.

    Returns
    -------
    seahorse._core.PropagationResult
        Native propagation result containing the final state and, optionally,
        the full trajectory.
    """

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
    """Propagate ``psi0`` and score the result against ``psit``.

    Parameters
    ----------
    task:
        Simulation configuration used for the propagation.
    psi0:
        Initial state vector. It is coerced to ``complex128`` before crossing
        the Python/C++ boundary.
    psit:
        Target state vector used to compute the achieved fidelity. It is
        coerced to ``complex128``.
    control:
        One scalar control value per propagation step. Values are coerced to
        ``float64``.

    Returns
    -------
    seahorse._core.ControlEvaluation
        Native evaluation result containing the control waveform, the
        fidelity-derived cost, the achieved fidelity, and the final-state norm.
    """

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
