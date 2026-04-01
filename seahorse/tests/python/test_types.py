from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import pytest

from seahorse.types import GridSpec, PotentialMode, PotentialSpec, TimeSpec, TransferTask
import seahorse.types as types_module


class _FakePotentialFactory:
    @staticmethod
    def constant(grid, values):
        return ("constant", grid, values.copy())

    @staticmethod
    def amplitude_scaled(grid, values):
        return ("amplitude_scaled", grid, values.copy())

    @staticmethod
    def shifted(grid, values):
        return ("shifted", grid, values.copy())


class _FakeCore:
    Potential1D = _FakePotentialFactory

    class Grid1D:
        def __init__(self, dim: int, xmin: float, xmax: float) -> None:
            self.dim = dim
            self.xmin = xmin
            self.xmax = xmax


def test_time_spec_arrays_have_expected_shapes() -> None:
    spec = TimeSpec(dt=0.25, num_steps=4)
    np.testing.assert_allclose(spec.array(), np.array([0.0, 0.25, 0.5, 0.75, 1.0]))
    np.testing.assert_allclose(spec.control_times(), np.array([0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0]))


def test_time_spec_control_times_handles_zero_steps() -> None:
    spec = TimeSpec(dt=0.5, num_steps=0)
    assert spec.control_times().shape == (0,)


def test_grid_spec_to_core_uses_loader(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(types_module, "_load_core", lambda: _FakeCore)
    grid = GridSpec(dim=16, xmin=-2.0, xmax=3.0).to_core()
    assert (grid.dim, grid.xmin, grid.xmax) == (16, -2.0, 3.0)


@pytest.mark.parametrize(
    ("mode", "expected_name"),
    [
        (PotentialMode.STATIC, "constant"),
        (PotentialMode.AMPLITUDE_SCALED, "amplitude_scaled"),
        (PotentialMode.SHIFTED, "shifted"),
    ],
)
def test_potential_spec_dispatches_to_expected_constructor(
    monkeypatch: pytest.MonkeyPatch,
    mode: PotentialMode,
    expected_name: str,
) -> None:
    monkeypatch.setattr(types_module, "_load_core", lambda: _FakeCore)
    grid = _FakeCore.Grid1D(8, -1.0, 1.0)
    values = np.arange(8.0)

    kind, returned_grid, returned_values = PotentialSpec(values, mode=mode).to_core(grid)

    assert kind == expected_name
    assert returned_grid is grid
    np.testing.assert_allclose(returned_values, values)


def test_potential_spec_rejects_unknown_mode(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(types_module, "_load_core", lambda: _FakeCore)
    grid = _FakeCore.Grid1D(8, -1.0, 1.0)
    spec = PotentialSpec(np.arange(8.0), mode="bogus")  # type: ignore[arg-type]

    with pytest.raises(ValueError, match="Unsupported potential mode"):
        spec.to_core(grid)


def test_transfer_task_build_core_wires_grid_and_potential(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(types_module, "_load_core", lambda: _FakeCore)

    task = TransferTask(
        grid=GridSpec(dim=8, xmin=-1.0, xmax=1.0),
        time=TimeSpec(dt=0.1, num_steps=5),
        potential=PotentialSpec(np.ones(8), mode=PotentialMode.SHIFTED),
        use_absorbing_boundary=False,
    )

    grid, potential = task.build_core()

    assert isinstance(grid, _FakeCore.Grid1D)
    assert potential[0] == "shifted"

