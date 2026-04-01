"""Python-first spatial quantum simulation scaffold."""

__version__ = "0.1.0"

from .api import evaluate_control, make_propagator, propagate, solve_spectrum
from .basis import TrigBasis, TrigMode
from .objective import (
    ControlEvaluation,
    ControlPenalty,
    Objective,
    StateTransfer,
    boundary_penalty,
    regularization_penalty,
)
from .optimise import DCRABConfig, DCRABOptimizer, DCRABResult, DCRABSketchOptimizer
from .potentials import box_window, cosine_lattice, planck_taper
from .types import GridSpec, PotentialMode, PotentialSpec, TimeSpec, TransferTask

__all__ = [
    "DCRABConfig",
    "DCRABOptimizer",
    "DCRABResult",
    "DCRABSketchOptimizer",
    "ControlEvaluation",
    "ControlPenalty",
    "GridSpec",
    "Objective",
    "PotentialMode",
    "PotentialSpec",
    "StateTransfer",
    "TimeSpec",
    "TransferTask",
    "TrigBasis",
    "TrigMode",
    "boundary_penalty",
    "box_window",
    "cosine_lattice",
    "evaluate_control",
    "make_propagator",
    "planck_taper",
    "propagate",
    "regularization_penalty",
    "solve_spectrum",
]
