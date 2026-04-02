from __future__ import annotations

import inspect

import pytest

import seahorse


def _assert_numpy_style(
    doc: str | None,
    *,
    has_parameters: bool = True,
    has_returns: bool = True,
) -> None:
    assert doc is not None
    if has_parameters:
        assert "Parameters\n----------" in doc
    if has_returns:
        assert "Returns\n-------" in doc


def test_public_api_symbols_have_custom_docstrings() -> None:
    missing = []

    for name in seahorse.__all__:
        obj = getattr(seahorse, name)
        doc = inspect.getdoc(obj)
        if doc is None or doc.startswith(f"{name}("):
            missing.append(name)

    assert not missing, f"Missing custom docstrings for: {', '.join(sorted(missing))}"


def test_selected_public_methods_have_docstrings() -> None:
    targets = {
        "GridSpec.to_core": seahorse.GridSpec.to_core,
        "TimeSpec.array": seahorse.TimeSpec.array,
        "PotentialSpec.to_core": seahorse.PotentialSpec.to_core,
        "TransferTask.build_core": seahorse.TransferTask.build_core,
        "TrigBasis.control": seahorse.TrigBasis.control,
        "ControlPenalty.scaled": seahorse.ControlPenalty.scaled,
        "Objective.evaluate": seahorse.Objective.evaluate,
        "DCRABOptimizer.optimise": seahorse.DCRABOptimizer.optimise,
    }

    missing = [name for name, obj in targets.items() if inspect.getdoc(obj) is None]
    assert not missing, f"Missing method docstrings for: {', '.join(missing)}"


def test_selected_public_callables_use_numpy_style_sections() -> None:
    targets = {
        "solve_spectrum": (seahorse.solve_spectrum, True, True),
        "make_propagator": (seahorse.make_propagator, True, True),
        "propagate": (seahorse.propagate, True, True),
        "evaluate_control": (seahorse.evaluate_control, True, True),
        "box_window": (seahorse.box_window, True, True),
        "planck_taper": (seahorse.planck_taper, True, True),
        "cosine_lattice": (seahorse.cosine_lattice, True, True),
        "GridSpec.to_core": (seahorse.GridSpec.to_core, False, True),
        "PotentialSpec.to_core": (seahorse.PotentialSpec.to_core, True, True),
        "TransferTask.build_core": (seahorse.TransferTask.build_core, False, True),
        "TrigBasis.control": (seahorse.TrigBasis.control, True, True),
        "ControlPenalty.scaled": (seahorse.ControlPenalty.scaled, True, True),
        "boundary_penalty": (seahorse.boundary_penalty, True, True),
        "Objective.evaluate": (seahorse.Objective.evaluate, True, True),
        "DCRABOptimizer.optimise": (seahorse.DCRABOptimizer.optimise, False, True),
    }

    for obj, has_parameters, has_returns in targets.values():
        _assert_numpy_style(
            inspect.getdoc(obj),
            has_parameters=has_parameters,
            has_returns=has_returns,
        )


@pytest.mark.native
def test_native_core_docstrings_are_descriptive(native_core) -> None:
    targets = {
        "Grid1D": (native_core.Grid1D, "Uniform one-dimensional spatial grid"),
        "Potential1D.constant": (
            native_core.Potential1D.constant,
            "Create a control-independent potential",
        ),
        "SplitStep1D.propagate": (
            native_core.SplitStep1D.propagate,
            "Propagate psi0 through the full control sequence",
        ),
        "solve_spectrum": (native_core.solve_spectrum, "lowest-energy eigenpairs"),
        "evaluate_control": (native_core.evaluate_control, "measure fidelity"),
    }

    missing = []
    for name, (obj, snippet) in targets.items():
        doc = inspect.getdoc(obj)
        normalized_doc = None if doc is None else doc.replace("`", "")
        normalized_snippet = snippet.replace("`", "")
        if normalized_doc is None or normalized_snippet not in normalized_doc:
            missing.append(name)

    assert not missing, f"Native callables missing descriptive docstrings: {', '.join(missing)}"


@pytest.mark.native
def test_selected_native_callables_use_numpy_style_sections(native_core) -> None:
    targets = {
        "Potential1D.constant": (native_core.Potential1D.constant, True, True),
        "Potential1D.sample": (native_core.Potential1D.sample, True, True),
        "SplitStep1D.reset": (native_core.SplitStep1D.reset, True, False),
        "SplitStep1D.step": (native_core.SplitStep1D.step, True, False),
        "SplitStep1D.propagate": (native_core.SplitStep1D.propagate, True, True),
        "solve_spectrum": (native_core.solve_spectrum, True, True),
        "evaluate_control": (native_core.evaluate_control, True, True),
    }

    for obj, has_parameters, has_returns in targets.values():
        _assert_numpy_style(
            inspect.getdoc(obj),
            has_parameters=has_parameters,
            has_returns=has_returns,
        )
