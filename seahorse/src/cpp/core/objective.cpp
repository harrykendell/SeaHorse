#include "objective.hpp"

ControlEvaluation evaluate_control(
    const Grid1D& grid,
    const Potential1D& potential,
    double dt,
    const CVec& psi0,
    const CVec& psit,
    const RVec& control,
    bool use_absorbing_boundary)
{
    if (psit.size() != grid.dim()) {
        throw std::invalid_argument("Target state size must match the grid dimension");
    }

    SplitStep1D propagator(grid, potential, dt, use_absorbing_boundary);
    const PropagationResult result = propagator.propagate(psi0, control, false);

    const CVec target = psit / psit.norm();
    const double fid = fidelity(target, result.final_state);

    return ControlEvaluation {
        control,
        -fid,
        fid,
        result.final_state.norm(),
    };
}

