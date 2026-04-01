#include "include/Optimisation/Cost/StateTransferCost.hpp"
#include "src/Physics/Vectors.hpp"

double StateTransfer::operator()(const RVec& u)
{
    stepper->evolve(psi_0, u);
    this->pseudofid = overlap(psi_t, stepper->state());

    double fid = fidelity(psi_t, stepper->state());
    // We want to maximise the fidelity, so the cost is the negative
    eval = { .control = u, .cost = -fid, .fid = fid, .norm = stepper->state().norm() };

    return eval.fid;
}

StateTransfer& StateTransfer::operator=(const StateTransfer& other)
{
    eval = other.eval;
    psi_0 = other.psi_0;
    psi_t = other.psi_t;

    stepper = other.stepper->clone();

    return *this;
}