from __future__ import annotations

import numpy as np

from seahorse import (
    DCRABConfig,
    DCRABOptimizer,
    GridSpec,
    Objective,
    PotentialMode,
    PotentialSpec,
    StateTransfer,
    TimeSpec,
    TransferTask,
    TrigBasis,
    TrigMode,
    boundary_penalty,
    box_window,
    regularization_penalty,
    solve_spectrum,
)


def main() -> None:
    dim = 1 << 11
    k = np.sqrt(2.0)
    xlim = np.pi / k / 2.0 * 4.0
    dt = 1e-3
    num_steps = 3000
    depth = 400.0

    grid = GridSpec(dim=dim, xmin=-xlim, xmax=xlim)
    time = TimeSpec(dt=dt, num_steps=num_steps)

    x = np.linspace(grid.xmin, grid.xmax, grid.dim)
    base = -0.5 * depth * (np.cos(2.0 * k * x) + 1.0) * box_window(x, -np.pi / k / 2.0, np.pi / k / 2.0)

    task = TransferTask(
        grid=grid,
        time=time,
        potential=PotentialSpec(base, mode=PotentialMode.SHIFTED),
    )

    spectrum = solve_spectrum(task, num_states=2)
    psi0 = np.asarray(spectrum.eigenvectors[:, 0], dtype=np.complex128)
    psi1 = np.asarray(spectrum.eigenvectors[:, 1], dtype=np.complex128)

    objective = Objective(
        task=task,
        transfers=[
            StateTransfer(psi0, psi1),
            StateTransfer(psi1, psi0),
        ],
        penalties=[
            1e3 * boundary_penalty(-1.0, 1.0),
            1e-6 * regularization_penalty(),
        ],
    )

    basis = TrigBasis(
        t=time.control_times(),
        max_frequency=8.5,
        mode=TrigMode.AMP_FREQ,
        basis_size=10,
    )

    optimiser = DCRABOptimizer(
        objective=objective,
        basis=basis,
        config=DCRABConfig(
            target_fidelity=0.99,
            max_iterations=100,
            stall_steps=20,
            dressings=5,
            seed=7,
        ),
        on_iteration=lambda opt: print(
            f"iter={opt.num_iterations:4d} "
            f"fid={opt.best.fid if opt.best else float('nan'):.6f} "
            f"cost={opt.best.cost if opt.best else float('nan'):.6f}"
        ),
    )

    result = optimiser.optimise()
    print()
    print("Best fidelity:", f"{result.best.fid:.6f}")
    print("Best cost:", f"{result.best.cost:.6f}")
    print("Best norm:", f"{result.best.norm:.6f}")
    print("Iterations:", result.num_iterations)
    print("Propagations:", result.num_propagations)
    print("Dressings completed:", result.dressings_completed)


if __name__ == "__main__":
    main()

