#pragma once

#include "types.hpp"

class Grid1D {
public:
    Grid1D(int dim, double xmin, double xmax);

    int dim() const noexcept { return dim_; }
    double xmin() const noexcept { return xmin_; }
    double xmax() const noexcept { return xmax_; }
    double dx() const noexcept { return dx_; }
    const RVec& x() const noexcept { return x_; }
    const SparseMat& kinetic() const noexcept { return kinetic_; }

private:
    RVec x_;
    double xmin_ = 0.0;
    double xmax_ = 0.0;
    double dx_ = 0.0;
    int dim_ = 0;
    SparseMat kinetic_;
};

