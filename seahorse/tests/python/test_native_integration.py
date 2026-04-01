from __future__ import annotations

import numpy as np
import pytest

from seahorse import (
    GridSpec,
    PotentialMode,
    PotentialSpec,
    TimeSpec,
    TransferTask,
    evaluate_control,
    propagate,
    solve_spectrum,
)


@pytest.mark.native
def test_native_core_smoke(native_core) -> None:
    grid = native_core.Grid1D(8, -1.0, 1.0)
    assert grid.dim == 8
    assert grid.x.shape == (8,)


@pytest.mark.native
def test_spectrum_propagation_and_control_evaluation_round_trip(native_core) -> None:
    dim = 64
    grid = GridSpec(dim=dim, xmin=-4.0, xmax=4.0)
    time = TimeSpec(dt=0.01, num_steps=20)
    x = np.linspace(grid.xmin, grid.xmax, grid.dim)
    potential = 0.25 * x**2

    task = TransferTask(
        grid=grid,
        time=time,
        potential=PotentialSpec(potential, mode=PotentialMode.STATIC),
        use_absorbing_boundary=False,
    )

    spectrum = solve_spectrum(task, num_states=2)
    assert spectrum.eigenvalues.shape == (2,)
    assert spectrum.eigenvectors.shape == (dim, 2)
    assert np.all(np.diff(spectrum.eigenvalues) >= 0.0)

    psi0 = np.asarray(spectrum.eigenvectors[:, 0], dtype=np.complex128)
    control = np.zeros(time.num_steps, dtype=np.float64)

    propagation = propagate(task, psi0, control, store_path=True)
    assert propagation.final_state.shape == (dim,)
    assert propagation.trajectory.shape == (dim, time.num_steps + 1)
    assert np.linalg.norm(propagation.final_state) == pytest.approx(1.0, rel=1e-5, abs=1e-5)

    evaluation = evaluate_control(task, psi0, psi0, control)
    assert evaluation.control.shape == (time.num_steps,)
    assert evaluation.fid == pytest.approx(1.0, rel=1e-4, abs=1e-4)
    assert evaluation.norm == pytest.approx(1.0, rel=1e-5, abs=1e-5)
