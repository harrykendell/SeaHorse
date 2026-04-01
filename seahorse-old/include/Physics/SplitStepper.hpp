#pragma once

#include "include/Physics/Stepper.hpp"
#include <iostream>

class SplitStepper : public Stepper {
private:
    // V is defined on real space, and is the actual potential
    // T is defined on shifted frequency space and is the kinetic energy operator
    std::function<RVec(double)> m_V;
    RVec imagPot;
    CVec m_T_exp;
    CVec m_T_exp_2;

    // FFT and buffers
    Eigen::FFT<double> m_fft;
    CVec m_mombuf;

protected:
    // Implementing the clone pattern within derived classes
    virtual Stepper* clone_impl() const override { return new SplitStepper(*this); }

public:
    // Constructor
    SplitStepper(double dt, HamiltonianFn& H, bool use_imag_pot = true);
    SplitStepper();

    // Discard any internal state changed to date
    void reset(const CVec& psi_0) override;

    // Evolution of state
    void step(double u) override;
    // Optimised steps but can't provide intermediate step wavefunctions
    // This combines T/2 ifft fft T/2 between steps to save computation.
    void evolve(const CVec& psi_0, const RVec& control) override;
};