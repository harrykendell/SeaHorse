#pragma once

#include "grid.hpp"

enum class PotentialKind {
    Static,
    AmplitudeScaled,
    Shifted,
};

class Potential1D {
public:
    static Potential1D constant(const Grid1D& grid, const RVec& values);
    static Potential1D amplitude_scaled(const Grid1D& grid, const RVec& values);
    static Potential1D shifted(const Grid1D& grid, const RVec& values);

    PotentialKind kind() const noexcept { return kind_; }
    const RVec& x() const noexcept { return x_; }
    const RVec& base() const noexcept { return base_; }
    double dx() const noexcept { return x_.size() > 1 ? x_(1) - x_(0) : 0.0; }

    RVec sample(double control) const;

private:
    Potential1D(RVec x, RVec base, PotentialKind kind);

    RVec sample_shifted(double shift) const;

    RVec x_;
    RVec base_;
    PotentialKind kind_;
};

