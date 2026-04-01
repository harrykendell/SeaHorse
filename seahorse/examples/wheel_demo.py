from __future__ import annotations

import numpy as np

from seahorse import (
    PotentialMode,
    PotentialSpec,
    GridSpec,
    TimeSpec,
    TransferTask,
    box_window,
    cosine_lattice,
    evaluate_control,
    propagate,
    solve_spectrum,
)


def make_task() -> TransferTask:
    grid = GridSpec(dim=1 << 7, xmin=-4.0, xmax=4.0)
    time = TimeSpec(dt=0.01, num_steps=160)

    x = np.linspace(grid.xmin, grid.xmax, grid.dim)
    base = cosine_lattice(x, depth=8.0, wave_number=1.2)
    base *= box_window(x, -2.5, 2.5)
    base += 0.05 * x**2

    return TransferTask(
        grid=grid,
        time=time,
        potential=PotentialSpec(base, mode=PotentialMode.SHIFTED),
        use_absorbing_boundary=False,
    )


def main() -> None:
    task = make_task()
    t = task.time.array()

    print("Solving the first four eigenstates...")
    spectrum = solve_spectrum(task, num_states=4)
    print("eigenvalues:", np.array2string(spectrum.eigenvalues, precision=6))

    psi0 = np.asarray(spectrum.eigenvectors[:, 0], dtype=np.complex128)
    psit = np.asarray(spectrum.eigenvectors[:, 1], dtype=np.complex128)

    control = 0.15 * np.sin(2.0 * np.pi * t[:-1] / t[-1])

    print("Propagating the ground state under a small drive...")
    result = propagate(task, psi0, control, store_path=True)

    final_overlap = np.vdot(psi0, result.final_state)
    print("final_state_norm:", f"{np.linalg.norm(result.final_state):.6f}")
    print("ground_state_overlap:", f"{abs(final_overlap):.6f}")
    print("trajectory_shape:", result.trajectory.shape)

    evaluation = evaluate_control(task, psi0, psit, control)
    print("transfer_fidelity:", f"{evaluation.fid:.6f}")
    print("transfer_cost:", f"{evaluation.cost:.6f}")


if __name__ == "__main__":
    main()
