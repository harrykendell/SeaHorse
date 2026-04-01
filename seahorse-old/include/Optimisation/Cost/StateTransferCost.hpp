#pragma once

#include "include/Optimisation/Cost/EvaluatedControl.hpp"
#include "include/Physics/Stepper.hpp"

template <typename T>
concept Steppable = std::is_base_of<Stepper, T>::value;

// Represents a fidelity cost function from an initial state to the desired final state
class StateTransfer {
public:
    friend class Cost;

private:
    EvaluatedControl eval;

    std::unique_ptr<Stepper> stepper;
    CVec psi_0;
    CVec psi_t;

    std::complex<double> pseudofid = 0.0;

public:
    // NB: This is not a true fidelity as we just average the fidelities of each
    // transfer. But it's good enough for our purposes.
    double operator()(const RVec&);

    // Copy constructor
    StateTransfer(const StateTransfer& other)
        : eval(other.eval)
        , stepper(other.stepper->clone())
        , psi_0(other.psi_0)
        , psi_t(other.psi_t) {};

    // Copy assignment operator
    StateTransfer& operator=(const StateTransfer& other);

    // Explicitly defaulted Destructor
    ~StateTransfer() = default;
    // Templated constructor to allow for any type of stepper
    template <Steppable T>
    StateTransfer(T stepper, CVec psi_0, CVec psi_t)
    {
        this->stepper = std::make_unique<T>(stepper);
        this->psi_0 = psi_0.normalized();
        this->psi_t = psi_t.normalized();
    }
};
