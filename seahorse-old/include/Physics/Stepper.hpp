#pragma once

#include <iostream>

#include "include/Physics/Hamiltonian.hpp"

#include <libs/eigen/Eigen/Core>
#include <libs/eigen/unsupported/Eigen/FFT>

// General class to evolve wavefunctions: either by a single `step(u)` or multiple `evolve(control)`.
class Stepper {

protected:
    // Implementing the clone pattern within derived classes
    virtual Stepper* clone_impl() const = 0;
    double m_dt = 0;
    double m_dx = 0;
    CVec m_psi_f; // current state

public:
    // Constructor
    Stepper();
    Stepper(double dt, HamiltonianFn& H);

    virtual ~Stepper() {};
    // implementing the clone pattern
    std::unique_ptr<Stepper> clone() const;

    // Reset any internal state
    virtual void reset(const CVec& psi_0) = 0;

    // Evolve by a step or a number of steps
    virtual void step(double u) = 0;
    virtual void evolve(const CVec& psi_0, const RVec& control) = 0;

    CVec state() const;
    double dt() const;
    double dx() const;
};