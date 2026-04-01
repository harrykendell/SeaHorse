from __future__ import annotations

import numpy as np

from seahorse import (
    GridSpec,
    PotentialMode,
    PotentialSpec,
    TimeSpec,
    TransferTask,
    box_window,
    cosine_lattice,
    solve_spectrum,
)


def main() -> None:
    grid = GridSpec(dim=1 << 9, xmin=-4.0, xmax=4.0)
    time = TimeSpec(dt=1e-3, num_steps=2000)

    x = np.linspace(grid.xmin, grid.xmax, grid.dim)
    base = cosine_lattice(x, depth=20.0, wave_number=np.sqrt(2.0)) * box_window(x, -2.0, 2.0)

    task = TransferTask(
        grid=grid,
        time=time,
        potential=PotentialSpec(base, mode=PotentialMode.SHIFTED),
    )

    spectrum = solve_spectrum(task, num_states=4)
    print(spectrum.eigenvalues)


if __name__ == "__main__":
    main()

