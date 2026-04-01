from __future__ import annotations

import seahorse


def test_public_symbols_are_exported() -> None:
    expected = {
        "DCRABConfig",
        "DCRABOptimizer",
        "DCRABResult",
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
    }
    assert expected.issubset(set(seahorse.__all__))


def test_version_string_is_present() -> None:
    assert isinstance(seahorse.__version__, str)
    assert seahorse.__version__

