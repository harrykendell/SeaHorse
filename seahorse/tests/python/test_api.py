from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import pytest

import seahorse.api as api


class _FakeCore:
    def __init__(self) -> None:
        self.solve_calls = []
        self.evaluate_calls = []
        self.propagator_calls = []

    class SplitStep1D:
        def __init__(self, grid, potential, dt, use_absorbing_boundary) -> None:
            self.grid = grid
            self.potential = potential
            self.dt = dt
            self.use_absorbing_boundary = use_absorbing_boundary
            self.calls = []

        def propagate(self, psi0, control, store_path):
            self.calls.append((psi0, control, store_path))
            return {
                "psi0_dtype": psi0.dtype,
                "control_dtype": control.dtype,
                "store_path": store_path,
            }

    def solve_spectrum(self, grid, potential, num_states):
        self.solve_calls.append((grid, potential, num_states))
        return {"grid": grid, "potential": potential, "num_states": num_states}

    def evaluate_control(self, grid, potential, dt, psi0, psit, control, absorbing):
        self.evaluate_calls.append((grid, potential, dt, psi0, psit, control, absorbing))
        return {
            "dt": dt,
            "psi0_dtype": psi0.dtype,
            "psit_dtype": psit.dtype,
            "control_dtype": control.dtype,
            "absorbing": absorbing,
        }


@dataclass
class _FakeTask:
    grid: object = "grid"
    potential: object = "potential"
    time: object = type("Time", (), {"dt": 0.25})()
    use_absorbing_boundary: bool = True

    def build_core(self):
        return self.grid, self.potential


def test_make_propagator_constructs_native_object(monkeypatch: pytest.MonkeyPatch) -> None:
    fake_core = _FakeCore()
    monkeypatch.setattr(api, "_load_core", lambda: fake_core)

    propagator = api.make_propagator(_FakeTask(use_absorbing_boundary=False))

    assert isinstance(propagator, _FakeCore.SplitStep1D)
    assert propagator.dt == 0.25
    assert propagator.use_absorbing_boundary is False


def test_propagate_coerces_numpy_dtypes(monkeypatch: pytest.MonkeyPatch) -> None:
    fake_core = _FakeCore()
    monkeypatch.setattr(api, "_load_core", lambda: fake_core)

    result = api.propagate(
        _FakeTask(),
        psi0=[1.0, 0.0],
        control=[0, 1, 2],
        store_path=True,
    )

    assert result["psi0_dtype"] == np.complex128
    assert result["control_dtype"] == np.float64
    assert result["store_path"] is True


def test_solve_spectrum_forwards_integer_num_states(monkeypatch: pytest.MonkeyPatch) -> None:
    fake_core = _FakeCore()
    monkeypatch.setattr(api, "_load_core", lambda: fake_core)

    result = api.solve_spectrum(_FakeTask(), num_states=np.int64(4))

    assert result["num_states"] == 4


def test_evaluate_control_forwards_native_arguments(monkeypatch: pytest.MonkeyPatch) -> None:
    fake_core = _FakeCore()
    monkeypatch.setattr(api, "_load_core", lambda: fake_core)

    result = api.evaluate_control(
        _FakeTask(use_absorbing_boundary=False),
        psi0=[1.0, 0.0],
        psit=[0.0, 1.0],
        control=[0.0, 1.0],
    )

    assert result["dt"] == 0.25
    assert result["psi0_dtype"] == np.complex128
    assert result["psit_dtype"] == np.complex128
    assert result["control_dtype"] == np.float64
    assert result["absorbing"] is False


def test_load_core_raises_helpful_error_when_extension_is_missing(monkeypatch: pytest.MonkeyPatch) -> None:
    import builtins

    real_import = builtins.__import__

    def _failing_import(name, globals=None, locals=None, fromlist=(), level=0):
        if level == 1 and fromlist == ("_core",):
            raise ImportError("simulated missing extension")
        return real_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(builtins, "__import__", _failing_import)

    with pytest.raises(RuntimeError, match="native extension is not available"):
        api._load_core()

