"""Objective and penalty helpers for control optimisation."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .api import propagate
from .types import TransferTask


@dataclass
class StateTransfer:
    """Normalised initial and target states for a state-transfer problem."""

    psi0: np.ndarray
    psit: np.ndarray

    def __post_init__(self) -> None:
        self.psi0 = np.asarray(self.psi0, dtype=np.complex128)
        self.psit = np.asarray(self.psit, dtype=np.complex128)

        if self.psi0.ndim != 1 or self.psit.ndim != 1:
            raise ValueError("StateTransfer expects 1D state vectors")
        if self.psi0.shape != self.psit.shape:
            raise ValueError("psi0 and psit must have the same shape")

        self.psi0 = self.psi0 / np.linalg.norm(self.psi0)
        self.psit = self.psit / np.linalg.norm(self.psit)


@dataclass
class ControlEvaluation:
    """Result of scoring a control waveform against the optimisation objective."""

    control: np.ndarray
    cost: float
    fid: float
    norm: float

    def __lt__(self, other: "ControlEvaluation") -> bool:
        return self.cost < other.cost


class ControlPenalty:
    """Callable penalty term applied to a control waveform."""

    def __init__(self, func, weight: float = 1.0, name: str = "penalty") -> None:
        self.func = func
        self.weight = float(weight)
        self.name = name

    def __call__(self, control) -> float:
        """Evaluate the weighted penalty on ``control``.

        Parameters
        ----------
        control:
            Control waveform to score.

        Returns
        -------
        float
            Weighted scalar penalty value.
        """

        control_arr = np.asarray(control, dtype=np.float64)
        return self.weight * float(self.func(control_arr))

    def scaled(self, factor: float) -> "ControlPenalty":
        """Return a copy of this penalty with its weight scaled by ``factor``.

        Parameters
        ----------
        factor:
            Multiplicative scale applied to the current penalty weight.

        Returns
        -------
        ControlPenalty
            New penalty with the same callable and name.
        """

        return ControlPenalty(self.func, self.weight * factor, self.name)

    def __rmul__(self, factor: float) -> "ControlPenalty":
        return self.scaled(float(factor))


def regularization_penalty() -> ControlPenalty:
    """Create an L2-style penalty for large control amplitudes.

    Returns
    -------
    ControlPenalty
        Penalty proportional to the mean squared control amplitude.
    """

    return ControlPenalty(lambda control: np.mean(np.abs(control) ** 2), name="regularization")


def boundary_penalty(min_bound: float, max_bound: float | None = None) -> ControlPenalty:
    """Create a penalty for control samples outside the provided bounds.

    Parameters
    ----------
    min_bound:
        Lower admissible bound. If ``max_bound`` is omitted, this value is
        treated as a symmetric magnitude bound.
    max_bound:
        Optional upper admissible bound.

    Returns
    -------
    ControlPenalty
        Penalty that grows with the mean squared bound violation.
    """

    if max_bound is None:
        max_bound = abs(min_bound)
        min_bound = -abs(min_bound)

    def _penalty(control: np.ndarray) -> float:
        upper = np.maximum(control - max_bound, 0.0)
        lower = np.minimum(control - min_bound, 0.0)
        return np.mean(np.abs(upper + lower) ** 2)

    return ControlPenalty(_penalty, name="boundary")


class Objective:
    """Aggregate state transfers and penalties into a scalar optimisation target."""

    def __init__(
        self,
        task: TransferTask,
        transfers: list[StateTransfer],
        penalties: list[ControlPenalty] | None = None,
    ) -> None:
        if not transfers:
            raise ValueError("Objective expects at least one state transfer")
        self.task = task
        self.transfers = transfers
        self.penalties = penalties or []
        self.num_propagations = 0

    def evaluate(self, control) -> ControlEvaluation:
        """Evaluate ``control`` across every transfer and penalty term.

        Parameters
        ----------
        control:
            Control waveform to propagate and score.

        Returns
        -------
        ControlEvaluation
            Python-side evaluation containing the copied control waveform, the
            total cost, the achieved fidelity, and the minimum final-state norm.
        """

        control_arr = np.asarray(control, dtype=np.float64)

        pseudofids: list[complex] = []
        min_norm = float("inf")

        for transfer in self.transfers:
            result = propagate(self.task, transfer.psi0, control_arr, store_path=False)
            final_state = np.asarray(result.final_state, dtype=np.complex128)
            pseudofids.append(np.vdot(transfer.psit, final_state))
            min_norm = min(min_norm, float(np.linalg.norm(final_state)))
            self.num_propagations += 1

        fid = abs(sum(pseudofids)) ** 2 / (len(pseudofids) ** 2)
        cost = -fid

        for penalty in self.penalties:
            cost += penalty(control_arr)

        return ControlEvaluation(
            control=control_arr.copy(),
            cost=float(cost),
            fid=float(fid),
            norm=float(min_norm),
        )
