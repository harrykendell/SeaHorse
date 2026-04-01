from __future__ import annotations

from types import SimpleNamespace

import numpy as np
import pytest

import seahorse.objective as objective_module
from seahorse.objective import (
    ControlEvaluation,
    ControlPenalty,
    Objective,
    StateTransfer,
    boundary_penalty,
    regularization_penalty,
)
from seahorse.types import GridSpec, PotentialMode, PotentialSpec, TimeSpec, TransferTask


def _simple_task() -> TransferTask:
    return TransferTask(
        grid=GridSpec(dim=4, xmin=-1.0, xmax=1.0),
        time=TimeSpec(dt=0.1, num_steps=3),
        potential=PotentialSpec(np.zeros(4), mode=PotentialMode.STATIC),
    )


def test_state_transfer_normalizes_states() -> None:
    transfer = StateTransfer(np.array([2.0, 0.0]), np.array([0.0, 3.0]))
    np.testing.assert_allclose(transfer.psi0, np.array([1.0 + 0.0j, 0.0 + 0.0j]))
    np.testing.assert_allclose(transfer.psit, np.array([0.0 + 0.0j, 1.0 + 0.0j]))


def test_state_transfer_rejects_non_vector_inputs() -> None:
    with pytest.raises(ValueError, match="1D state vectors"):
        StateTransfer(np.eye(2), np.array([1.0, 0.0]))


def test_state_transfer_rejects_shape_mismatch() -> None:
    with pytest.raises(ValueError, match="same shape"):
        StateTransfer(np.array([1.0, 0.0]), np.array([1.0, 0.0, 0.0]))


def test_control_penalty_scaling_preserves_name() -> None:
    penalty = ControlPenalty(lambda control: np.sum(control), weight=2.0, name="sum")
    scaled = 3.0 * penalty
    assert scaled.name == "sum"
    assert scaled(np.array([1.0, 2.0])) == 18.0


def test_boundary_and_regularization_penalties_behave_as_expected() -> None:
    control = np.array([-2.0, -0.5, 0.5, 2.0])
    assert boundary_penalty(-1.0, 1.0)(control) == pytest.approx(0.5)
    assert regularization_penalty()(np.array([1.0, -1.0])) == pytest.approx(1.0)


def test_objective_requires_at_least_one_transfer() -> None:
    with pytest.raises(ValueError, match="at least one state transfer"):
        Objective(task=_simple_task(), transfers=[])


def test_objective_aggregates_transfers_and_penalties(monkeypatch: pytest.MonkeyPatch) -> None:
    task = _simple_task()
    transfer_a = StateTransfer(np.array([1.0, 0.0]), np.array([1.0, 0.0]))
    transfer_b = StateTransfer(np.array([0.0, 1.0]), np.array([0.0, 1.0]))

    responses = {
        tuple(transfer_a.psi0): np.array([1.0 + 0.0j, 0.0 + 0.0j]),
        tuple(transfer_b.psi0): np.array([0.0 + 0.0j, 1.0 + 0.0j]),
    }

    def fake_propagate(task_arg, psi0, control, *, store_path=False):
        assert task_arg is task
        assert store_path is False
        return SimpleNamespace(final_state=responses[tuple(np.asarray(psi0))])

    monkeypatch.setattr(objective_module, "propagate", fake_propagate)

    objective = Objective(
        task=task,
        transfers=[transfer_a, transfer_b],
        penalties=[0.5 * regularization_penalty()],
    )

    control = np.array([1.0, -1.0, 1.0])
    evaluation = objective.evaluate(control)

    assert isinstance(evaluation, ControlEvaluation)
    np.testing.assert_allclose(evaluation.control, control)
    assert evaluation.fid == pytest.approx(1.0)
    assert evaluation.cost == pytest.approx(-1.0 + 0.5)
    assert evaluation.norm == pytest.approx(1.0)
    assert objective.num_propagations == 2


def test_control_evaluation_orders_by_cost() -> None:
    better = ControlEvaluation(np.zeros(2), cost=-1.0, fid=1.0, norm=1.0)
    worse = ControlEvaluation(np.zeros(2), cost=0.5, fid=0.5, norm=1.0)
    assert better < worse

