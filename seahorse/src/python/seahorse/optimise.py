from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from .basis import TrigBasis
from .objective import ControlEvaluation, Objective


@dataclass(frozen=True)
class DCRABConfig:
    target_fidelity: float = 0.99
    max_iterations: int = 100
    stall_steps: int = 20
    dressings: int = 5
    simplex_size_epsilon: float = 1e-3
    flat_cost_epsilon: float = 1e-5
    alpha: float = 1.0
    gamma: float = 2.0
    rho: float = 0.5
    sigma: float = 0.5
    seed: int | None = None


@dataclass
class DCRABResult:
    best: ControlEvaluation
    num_iterations: int
    steps_since_improvement: int
    num_propagations: int
    dressings_completed: int
    history: list[ControlEvaluation] = field(default_factory=list)


@dataclass
class _SimplexPoint:
    coeffs: np.ndarray
    evaluation: ControlEvaluation

    @property
    def cost(self) -> float:
        return self.evaluation.cost


class DCRABOptimizer:
    """Python-side dressed CRAB optimiser using a coarse native objective."""

    def __init__(
        self,
        objective: Objective,
        basis: TrigBasis,
        config: DCRABConfig,
        on_iteration=None,
    ) -> None:
        self.objective = objective
        self.basis = basis
        self.config = config
        self.on_iteration = on_iteration

        self.rng = np.random.default_rng(config.seed)
        self.num_iterations = 0
        self.steps_since_improvement = 0
        self.best: ControlEvaluation | None = None
        self.history: list[ControlEvaluation] = []
        self._dressed_terms: list[tuple[np.ndarray, TrigBasis]] = []
        self._simplex: list[_SimplexPoint] = []

    def sample_coefficients(self) -> np.ndarray:
        return self.basis.sample_coefficients(self.rng)

    def control_from_coefficients(self, coeffs) -> np.ndarray:
        control = self.basis.control(coeffs)
        for dressed_coeffs, dressed_basis in self._dressed_terms:
            control = control + dressed_basis.control(dressed_coeffs)
        return control

    def evaluate_coefficients(self, coeffs) -> ControlEvaluation:
        control = self.control_from_coefficients(coeffs)
        evaluation = self.objective.evaluate(control)
        if self.best is None or evaluation < self.best:
            self.best = evaluation
            self.steps_since_improvement = 0
        return evaluation

    def _generate_simplex(self) -> None:
        self._simplex = []

        zero = np.zeros(self.basis.num_coeffs, dtype=np.float64)
        self._simplex.append(_SimplexPoint(zero, self.evaluate_coefficients(zero)))

        for _ in range(self.basis.num_coeffs):
            coeffs = self.sample_coefficients()
            self._simplex.append(_SimplexPoint(coeffs, self.evaluate_coefficients(coeffs)))

        self._simplex.sort(key=lambda point: point.cost)

    def _simplex_size(self) -> float:
        coeffs = np.stack([point.coeffs for point in self._simplex[:-1]], axis=0)
        centroid = coeffs.mean(axis=0)
        distances = np.linalg.norm(coeffs - centroid, axis=1)
        return float(np.mean(distances))

    def _flat_cost(self) -> bool:
        return (self._simplex[-1].cost - self._simplex[0].cost) < self.config.flat_cost_epsilon

    def _step(self) -> None:
        self.num_iterations += 1
        self.steps_since_improvement += 1

        self._simplex.sort(key=lambda point: point.cost)
        centroid = np.mean([point.coeffs for point in self._simplex[:-1]], axis=0)
        worst = self._simplex[-1]
        second_worst = self._simplex[-2]
        best_point = self._simplex[0]

        reflection_coeffs = centroid + self.config.alpha * (centroid - worst.coeffs)
        reflected_eval = self.evaluate_coefficients(reflection_coeffs)
        reflected = _SimplexPoint(reflection_coeffs, reflected_eval)

        if best_point.cost < reflected.cost < second_worst.cost:
            self._simplex[-1] = reflected
            return

        if reflected.cost < best_point.cost:
            expansion_coeffs = centroid + self.config.gamma * (reflection_coeffs - centroid)
            expanded_eval = self.evaluate_coefficients(expansion_coeffs)
            expanded = _SimplexPoint(expansion_coeffs, expanded_eval)
            self._simplex[-1] = expanded if expanded.cost < reflected.cost else reflected
            return

        if reflected.cost < worst.cost:
            contraction_coeffs = centroid + self.config.rho * (reflection_coeffs - centroid)
            contracted_eval = self.evaluate_coefficients(contraction_coeffs)
            contracted = _SimplexPoint(contraction_coeffs, contracted_eval)
            if contracted.cost < reflected.cost:
                self._simplex[-1] = contracted
                return
        else:
            contraction_coeffs = centroid + self.config.rho * (worst.coeffs - centroid)
            contracted_eval = self.evaluate_coefficients(contraction_coeffs)
            contracted = _SimplexPoint(contraction_coeffs, contracted_eval)
            if contracted.cost < worst.cost:
                self._simplex[-1] = contracted
                return

        base_coeffs = self._simplex[0].coeffs
        for i in range(1, len(self._simplex)):
            new_coeffs = base_coeffs + self.config.sigma * (self._simplex[i].coeffs - base_coeffs)
            self._simplex[i] = _SimplexPoint(new_coeffs, self.evaluate_coefficients(new_coeffs))

    def optimise(self) -> DCRABResult:
        dressings_completed = 0

        for dressing_index in range(self.config.dressings + 1):
            self._generate_simplex()

            while True:
                self._simplex.sort(key=lambda point: point.cost)

                if self.best is not None:
                    self.history.append(self.best)
                    if self.on_iteration is not None:
                        self.on_iteration(self)

                if self.best is not None and self.best.fid >= self.config.target_fidelity:
                    return DCRABResult(
                        best=self.best,
                        num_iterations=self.num_iterations,
                        steps_since_improvement=self.steps_since_improvement,
                        num_propagations=self.objective.num_propagations,
                        dressings_completed=dressings_completed,
                        history=self.history.copy(),
                    )

                if self.num_iterations >= self.config.max_iterations:
                    if self.best is None:
                        raise RuntimeError("DCRABOptimizer reached max_iterations without any evaluations")
                    return DCRABResult(
                        best=self.best,
                        num_iterations=self.num_iterations,
                        steps_since_improvement=self.steps_since_improvement,
                        num_propagations=self.objective.num_propagations,
                        dressings_completed=dressings_completed,
                        history=self.history.copy(),
                    )

                if self.steps_since_improvement > self.config.stall_steps:
                    break
                if self._simplex_size() < self.config.simplex_size_epsilon:
                    break
                if self._flat_cost():
                    break

                self._step()

            if dressing_index >= self.config.dressings:
                break

            self._simplex.sort(key=lambda point: point.cost)
            self._dressed_terms.append((self._simplex[0].coeffs.copy(), self.basis))
            self.basis = self.basis.generate_new_basis()
            self.steps_since_improvement = 0
            dressings_completed += 1

        if self.best is None:
            raise RuntimeError("DCRABOptimizer finished without producing a best control")

        return DCRABResult(
            best=self.best,
            num_iterations=self.num_iterations,
            steps_since_improvement=self.steps_since_improvement,
            num_propagations=self.objective.num_propagations,
            dressings_completed=dressings_completed,
            history=self.history.copy(),
        )


DCRABSketchOptimizer = DCRABOptimizer
