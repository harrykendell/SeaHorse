"""Python-first spatial quantum simulation scaffold."""

__version__ = "0.1.0"

from .api import evaluate_control, make_propagator, propagate, solve_spectrum
from .basis import TrigBasis
from .optimise import DCRABConfig, DCRABSketchOptimizer
from .potentials import box_window, cosine_lattice, planck_taper
from .types import GridSpec, PotentialMode, PotentialSpec, TimeSpec, TransferTask

__all__ = [
    "DCRABConfig",
    "DCRABSketchOptimizer",
    "GridSpec",
    "PotentialMode",
    "PotentialSpec",
    "TimeSpec",
    "TransferTask",
    "TrigBasis",
    "box_window",
    "cosine_lattice",
    "evaluate_control",
    "make_propagator",
    "planck_taper",
    "propagate",
    "solve_spectrum",
]

