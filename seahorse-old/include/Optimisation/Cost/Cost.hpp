#pragma once

#include "include/Optimisation/Cost/ControlCost.hpp"
#include "include/Optimisation/Cost/EvaluatedControl.hpp"
#include "include/Optimisation/Cost/StateTransferCost.hpp"
#include "src/Utils/Logger.hpp"

// Evaluates a control based on the state to state transfers and control penalties
class Cost {
public:
    EvaluatedControl operator()(const RVec&);
    int fpp = 0;

    Cost(StateTransfer transfer) { this->transfers.push_back(transfer); };
    Cost(ControlCost cc) { this->components.push_back(cc); };

    void inline addTransferCost(StateTransfer st)
    {
        // First we need to ensure the initial state is orthogonal to any we
        // already use
        for (auto& transfer : transfers) {
            if (fidelity(transfer.psi_0, st.psi_0) > 1e-4) {
                S_FATAL("Initial states for StateTransfers are not orthogonal. This indicates a non-unitary transfer operator. (fid = ", fidelity(transfer.psi_0, st.psi_0));
            }
            if (fidelity(transfer.psi_t, st.psi_t) > 1e-4) {
                S_FATAL("Final states for StateTransfers are not orthogonal. This indicates a non-unitary transfer operator. (fid = ", fidelity(transfer.psi_t, st.psi_t));
            }
        }

        transfers.push_back(st);
    }

    void addControlCost(ControlCost cc) { components.push_back(cc); }

    Cost operator+(Cost& other);
    Cost operator+(StateTransfer st);
    Cost operator+(ControlCost cc);
    // Cost operator+(RobustCost rc);

private:
    std::vector<StateTransfer> transfers;
    std::vector<ControlCost> components;
};

// self-self
Cost operator+(StateTransfer, StateTransfer);
Cost operator+(ControlCost, ControlCost);
// Cost operator+(RobustCost, RobustCost);

// Cost-self to mirror inside Cost Class
Cost operator+(StateTransfer, Cost);
Cost operator+(ControlCost, Cost);
// Cost operator+(RobustCost, Cost);

// allow for commutativity
Cost operator+(StateTransfer, ControlCost);
// Cost operator+(StateTransfer, RobustCost);
Cost operator+(ControlCost, StateTransfer);
// Cost operator+(ControlCost, RobustCost);
// Cost operator+(RobustCost, StateTransfer);
// Cost operator+(RobustCost, ControlCost);
