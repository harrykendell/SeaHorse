#pragma once

#include "potential.hpp"

#include <unsupported/Eigen/FFT>

struct PropagationResult {
    CVec final_state;
    CMat trajectory;
};

class SplitStep1D {
public:
    SplitStep1D(const Grid1D& grid, Potential1D potential, double dt, bool use_absorbing_boundary = true);

    void reset(const CVec& psi0);
    void step(double control);
    PropagationResult propagate(const CVec& psi0, const RVec& control, bool store_path = false);

    const CVec& state() const noexcept { return psi_; }
    double dt() const noexcept { return dt_; }

private:
    Grid1D grid_;
    Potential1D potential_;
    double dt_ = 0.0;

    CVec kinetic_half_;
    CVec kinetic_full_;
    RVec absorbing_;

    Eigen::FFT<double> fft_;
    CVec psi_;
    CVec momentum_buffer_;
};

