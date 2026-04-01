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
    m.doc() = "Seahorse native core";

    nb::enum_<PotentialKind>(m, "PotentialKind")
        .value("STATIC", PotentialKind::Static)
        .value("AMPLITUDE_SCALED", PotentialKind::AmplitudeScaled)
        .value("SHIFTED", PotentialKind::Shifted);

    nb::class_<Grid1D>(m, "Grid1D")
        .def(nb::init<int, double, double>(), "dim"_a, "xmin"_a, "xmax"_a)
        .def_prop_ro("dim", &Grid1D::dim)
        .def_prop_ro("xmin", &Grid1D::xmin)
        .def_prop_ro("xmax", &Grid1D::xmax)
        .def_prop_ro("dx", &Grid1D::dx)
        .def_prop_ro("x", &Grid1D::x);

    nb::class_<Potential1D>(m, "Potential1D")
        .def_static("constant", &Potential1D::constant, "grid"_a, "values"_a)
        .def_static("amplitude_scaled", &Potential1D::amplitude_scaled, "grid"_a, "values"_a)
        .def_static("shifted", &Potential1D::shifted, "grid"_a, "values"_a)
        .def_prop_ro("kind", &Potential1D::kind)
        .def_prop_ro("x", &Potential1D::x)
        .def_prop_ro("base", &Potential1D::base)
        .def("sample", &Potential1D::sample, "control"_a);

    nb::class_<SpectrumResult>(m, "SpectrumResult")
        .def(nb::init<>())
        .def_rw("eigenvalues", &SpectrumResult::eigenvalues)
        .def_rw("eigenvectors", &SpectrumResult::eigenvectors);

    nb::class_<PropagationResult>(m, "PropagationResult")
        .def(nb::init<>())
        .def_rw("final_state", &PropagationResult::final_state)
        .def_rw("trajectory", &PropagationResult::trajectory);

    nb::class_<ControlEvaluation>(m, "ControlEvaluation")
        .def(nb::init<>())
        .def_rw("control", &ControlEvaluation::control)
        .def_rw("cost", &ControlEvaluation::cost)
        .def_rw("fid", &ControlEvaluation::fid)
        .def_rw("norm", &ControlEvaluation::norm);

    nb::class_<SplitStep1D>(m, "SplitStep1D")
        .def(
            nb::init<const Grid1D&, Potential1D, double, bool>(),
            "grid"_a,
            "potential"_a,
            "dt"_a,
            "use_absorbing_boundary"_a = true)
        .def("reset", &SplitStep1D::reset, "psi0"_a)
        .def("step", &SplitStep1D::step, "control"_a)
        .def("propagate", &SplitStep1D::propagate, "psi0"_a, "control"_a, "store_path"_a = false)
        .def_prop_ro("state", &SplitStep1D::state)
        .def_prop_ro("dt", &SplitStep1D::dt);

    m.def(
        "solve_spectrum",
        &solve_spectrum,
        "grid"_a,
        "potential"_a,
        "num_states"_a,
        "shift_guess"_a = std::numeric_limits<double>::quiet_NaN());

    m.def(
        "evaluate_control",
        &evaluate_control,
        "grid"_a,
        "potential"_a,
        "dt"_a,
        "psi0"_a,
        "psit"_a,
        "control"_a,
        "use_absorbing_boundary"_a = true);
}
