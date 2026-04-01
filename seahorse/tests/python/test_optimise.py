from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from seahorse.objective import ControlEvaluation
from seahorse.optimise import DCRABConfig, DCRABOptimizer


class _FakeBasis:
    def __init__(self, tag: int = 0) -> None:
        self.tag = tag
        self.num_coeffs = 2
        self.generated = 0

    def sample_coefficients(self, rng: np.random.Generator | None = None) -> np.ndarray:
        generator = rng or np.random.default_rng()
        return generator.uniform(-1.0, 1.0, size=self.num_coeffs)

    def control(self, coeffs) -> np.ndarray:
        coeffs_arr = np.asarray(coeffs, dtype=np.float64)
        return coeffs_arr + self.tag

    def generate_new_basis(self) -> "_FakeBasis":
        self.generated += 1
        return _FakeBasis(tag=self.tag + 10)


class _ConstantObjective:
    def __init__(self) -> None:
        self.num_propagations = 0

    def evaluate(self, control) -> ControlEvaluation:
        self.num_propagations += 1
        control_arr = np.asarray(control, dtype=np.float64)
        return ControlEvaluation(control=control_arr, cost=1.0, fid=0.0, norm=1.0)


class _ZeroObjective:
    def __init__(self) -> None:
        self.num_propagations = 0

    def evaluate(self, control) -> ControlEvaluation:
        self.num_propagations += 1
        control_arr = np.asarray(control, dtype=np.float64)
        fid = 1.0 if np.allclose(control_arr, 0.0) else 0.25
        return ControlEvaluation(control=control_arr, cost=-fid, fid=fid, norm=1.0)


def test_control_from_coefficients_adds_dressed_terms() -> None:
    optimizer = DCRABOptimizer(
        objective=_ConstantObjective(),
        basis=_FakeBasis(tag=1),
        config=DCRABConfig(max_iterations=1),
    )
    optimizer._dressed_terms = [(np.array([1.0, 2.0]), _FakeBasis(tag=5))]

    control = optimizer.control_from_coefficients(np.array([3.0, 4.0]))

    np.testing.assert_allclose(control, np.array([3.0, 4.0]) + 1 + np.array([1.0, 2.0]) + 5)


def test_optimise_returns_immediately_when_target_fidelity_is_met() -> None:
    callbacks: list[tuple[int, float]] = []
    optimizer = DCRABOptimizer(
        objective=_ZeroObjective(),
        basis=_FakeBasis(),
        config=DCRABConfig(target_fidelity=0.99, max_iterations=5, dressings=0, seed=1),
        on_iteration=lambda opt: callbacks.append((opt.num_iterations, opt.best.fid if opt.best else -1.0)),
    )

    result = optimizer.optimise()

    assert result.best.fid == 1.0
    assert result.num_iterations == 0
    assert result.num_propagations >= 1
    assert callbacks


def test_optimise_dresses_basis_when_landscape_is_flat() -> None:
    basis = _FakeBasis()
    optimizer = DCRABOptimizer(
        objective=_ConstantObjective(),
        basis=basis,
        config=DCRABConfig(
            target_fidelity=0.99,
            max_iterations=5,
            stall_steps=0,
            dressings=1,
            seed=2,
        ),
    )

    result = optimizer.optimise()

    assert result.dressings_completed == 1
    assert basis.generated == 1
    assert result.best.cost == 1.0
    assert result.num_propagations >= basis.num_coeffs + 1

