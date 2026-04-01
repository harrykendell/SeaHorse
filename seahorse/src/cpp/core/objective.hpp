#pragma once

#include "propagator.hpp"

struct ControlEvaluation {
    RVec control;
    double cost = 0.0;
    double fid = 0.0;
    double norm = 0.0;
};

ControlEvaluation evaluate_control(
    const Grid1D& grid,
    const Potential1D& potential,
    double dt,
    const CVec& psi0,
    const CVec& psit,
    const RVec& control,
    bool use_absorbing_boundary = true);

