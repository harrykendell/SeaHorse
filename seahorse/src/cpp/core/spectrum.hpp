#pragma once

#include "potential.hpp"

#include <limits>

struct SpectrumResult {
    RVec eigenvalues;
    RMat eigenvectors;
};

SpectrumResult solve_spectrum(
    const Grid1D& grid,
    const Potential1D& potential,
    int num_states,
    double shift_guess = std::numeric_limits<double>::quiet_NaN());
