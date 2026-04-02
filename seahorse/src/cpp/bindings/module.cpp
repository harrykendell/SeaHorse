#include <nanobind/nanobind.h>

#include <nanobind/eigen/dense.h>
#include <nanobind/stl/string.h>

#include <limits>

#include "grid.hpp"
#include "objective.hpp"
#include "potential.hpp"
#include "propagator.hpp"
#include "spectrum.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_core, m)
{
    m.doc() = "Nanobind-backed C++ kernels for Seahorse spatial simulations.";

    nb::enum_<PotentialKind>(m, "PotentialKind", "How a native potential responds to the scalar control.")
        .value("STATIC", PotentialKind::Static)
        .value("AMPLITUDE_SCALED", PotentialKind::AmplitudeScaled)
        .value("SHIFTED", PotentialKind::Shifted);

    nb::class_<Grid1D>(
        m,
        "Grid1D",
        "Uniform one-dimensional spatial grid with periodic finite-difference kinetics.")
        .def(
            nb::init<int, double, double>(),
            "dim"_a,
            "xmin"_a,
            "xmax"_a,
            R"doc(
Create a uniform one-dimensional grid.

Parameters
----------
dim:
    Number of grid points. Must be even and at least five.
xmin, xmax:
    Inclusive spatial bounds of the grid.
)doc")
        .def_prop_ro("dim", &Grid1D::dim, "Number of grid points.")
        .def_prop_ro("xmin", &Grid1D::xmin, "Lower bound of the spatial interval.")
        .def_prop_ro("xmax", &Grid1D::xmax, "Upper bound of the spatial interval.")
        .def_prop_ro("dx", &Grid1D::dx, "Uniform spacing between neighbouring grid points.")
        .def_prop_ro("x", &Grid1D::x, "One-dimensional array containing the spatial coordinates.");

    nb::class_<Potential1D>(
        m,
        "Potential1D",
        "One-dimensional potential sampled on a Grid1D and driven by a scalar control.")
        .def_static(
            "constant",
            &Potential1D::constant,
            "grid"_a,
            "values"_a,
            R"doc(
Create a control-independent potential from samples defined on ``grid``.

Parameters
----------
grid:
    Spatial grid on which the potential samples are defined.
values:
    Sampled potential values. The array length must match ``grid.dim``.

Returns
-------
Potential1D
    Potential that ignores the control signal and always returns ``values``.
)doc")
        .def_static(
            "amplitude_scaled",
            &Potential1D::amplitude_scaled,
            "grid"_a,
            "values"_a,
            R"doc(
Create a potential whose sampled values are multiplied by the scalar control.

Parameters
----------
grid:
    Spatial grid on which the potential samples are defined.
values:
    Base sampled potential values. The array length must match ``grid.dim``.

Returns
-------
Potential1D
    Potential that scales ``values`` by the supplied control amplitude.
)doc")
        .def_static(
            "shifted",
            &Potential1D::shifted,
            "grid"_a,
            "values"_a,
            R"doc(
Create a potential that is spatially shifted according to the scalar control.

Parameters
----------
grid:
    Spatial grid on which the potential samples are defined.
values:
    Base sampled potential values. The array length must match ``grid.dim``.

Returns
-------
Potential1D
    Potential that wraps and interpolates the sampled values as the control
    signal shifts the potential in space.
)doc")
        .def_prop_ro("kind", &Potential1D::kind, "Potential control mode.")
        .def_prop_ro("x", &Potential1D::x, "Spatial coordinates used by the sampled potential.")
        .def_prop_ro("base", &Potential1D::base, "Base sampled potential values before control is applied.")
        .def(
            "sample",
            &Potential1D::sample,
            "control"_a,
            R"doc(
Sample the potential values produced by ``control`` on the underlying grid.

Parameters
----------
control:
    Scalar control amplitude applied to the potential model.

Returns
-------
numpy.ndarray
    Sampled potential values on the underlying grid.
)doc");

    nb::class_<SpectrumResult>(m, "SpectrumResult", "Eigenpairs returned by ``solve_spectrum``.")
        .def(nb::init<>())
        .def_rw("eigenvalues", &SpectrumResult::eigenvalues, "Lowest eigenvalues returned by the solver.")
        .def_rw(
            "eigenvectors",
            &SpectrumResult::eigenvectors,
            "Matrix whose columns are the corresponding eigenvectors.");

    nb::class_<PropagationResult>(m, "PropagationResult", "Result of a split-step propagation.")
        .def(nb::init<>())
        .def_rw("final_state", &PropagationResult::final_state, "Final propagated state.")
        .def_rw(
            "trajectory",
            &PropagationResult::trajectory,
            "Stored state trajectory with shape ``(dim, len(control) + 1)`` when requested.");

    nb::class_<ControlEvaluation>(m, "ControlEvaluation", "Fidelity-based score computed for a control waveform.")
        .def(nb::init<>())
        .def_rw("control", &ControlEvaluation::control, "Control waveform that was evaluated.")
        .def_rw("cost", &ControlEvaluation::cost, "Scalar objective value derived from the fidelity.")
        .def_rw("fid", &ControlEvaluation::fid, "Achieved state-transfer fidelity.")
        .def_rw("norm", &ControlEvaluation::norm, "Norm of the propagated final state.");

    nb::class_<SplitStep1D>(
        m,
        "SplitStep1D",
        "One-dimensional split-step propagator for time-dependent scalar control sequences.")
        .def(
            nb::init<const Grid1D&, Potential1D, double, bool>(),
            "grid"_a,
            "potential"_a,
            "dt"_a,
            "use_absorbing_boundary"_a = true,
            R"doc(
Create a split-step propagator.

Parameters
----------
grid:
    Spatial grid used by the kinetic and potential operators.
potential:
    Potential model evaluated at each control step.
dt:
    Propagation timestep.
use_absorbing_boundary:
    Whether to damp the wavefunction near the boundaries during propagation.
)doc")
        .def(
            "reset",
            &SplitStep1D::reset,
            "psi0"_a,
            R"doc(
Normalise ``psi0`` and install it as the propagator's current state.

Parameters
----------
psi0:
    Initial state vector. Its length must match the grid dimension.
)doc")
        .def(
            "step",
            &SplitStep1D::step,
            "control"_a,
            R"doc(
Advance the propagator by one control step.

Parameters
----------
control:
    Scalar control amplitude applied during the timestep.
)doc")
        .def(
            "propagate",
            &SplitStep1D::propagate,
            "psi0"_a,
            "control"_a,
            "store_path"_a = false,
            R"doc(
Propagate ``psi0`` through the full control sequence and optionally store every state.

Parameters
----------
psi0:
    Initial state vector. Its length must match the grid dimension.
control:
    One scalar control value per propagation step.
store_path:
    When ``True``, record every propagated state in the returned trajectory.

Returns
-------
PropagationResult
    Final state and, when requested, the full state trajectory.
)doc")
        .def_prop_ro("state", &SplitStep1D::state, "Current propagated state.")
        .def_prop_ro("dt", &SplitStep1D::dt, "Propagation timestep.");

    m.def(
        "solve_spectrum",
        &solve_spectrum,
        "grid"_a,
        "potential"_a,
        "num_states"_a,
        "shift_guess"_a = std::numeric_limits<double>::quiet_NaN(),
        R"doc(
Solve for the lowest-energy eigenpairs of the Hamiltonian defined by ``grid`` and ``potential``.

Parameters
----------
grid:
    Spatial grid defining the kinetic operator.
potential:
    Potential sampled on ``grid``.
num_states:
    Number of low-lying eigenpairs to compute.
shift_guess:
    Optional shift used by the sparse eigensolver. If omitted, a shift is
    inferred from the sampled potential values.

Returns
-------
SpectrumResult
    Eigenvalues and eigenvectors returned by the native solver.

)doc");

    m.def(
        "evaluate_control",
        &evaluate_control,
        "grid"_a,
        "potential"_a,
        "dt"_a,
        "psi0"_a,
        "psit"_a,
        "control"_a,
        "use_absorbing_boundary"_a = true,
        R"doc(
Propagate ``psi0`` under ``control`` and measure fidelity against ``psit``.

Parameters
----------
grid:
    Spatial grid used by the propagator.
potential:
    Potential model sampled on ``grid``.
dt:
    Propagation timestep.
psi0:
    Initial state vector.
psit:
    Target state vector used to compute the achieved fidelity.
control:
    One scalar control value per propagation step.
use_absorbing_boundary:
    Whether to damp the wavefunction near the boundaries during propagation.

Returns
-------
ControlEvaluation
    Control waveform together with its cost, fidelity, and final-state norm.
)doc");
}
