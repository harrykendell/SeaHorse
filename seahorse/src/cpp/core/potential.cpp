#include "potential.hpp"

#include <cmath>
#include <utility>

namespace {

void validate_values(const Grid1D& grid, const RVec& values)
{
    if (values.size() != grid.dim()) {
        throw std::invalid_argument("Potential values must match the grid dimension");
    }
}

} // namespace

Potential1D::Potential1D(RVec x, RVec base, PotentialKind kind)
    : x_(std::move(x))
    , base_(std::move(base))
    , kind_(kind)
{
}

Potential1D Potential1D::constant(const Grid1D& grid, const RVec& values)
{
    validate_values(grid, values);
    return Potential1D(grid.x(), values, PotentialKind::Static);
}

Potential1D Potential1D::amplitude_scaled(const Grid1D& grid, const RVec& values)
{
    validate_values(grid, values);
    return Potential1D(grid.x(), values, PotentialKind::AmplitudeScaled);
}

Potential1D Potential1D::shifted(const Grid1D& grid, const RVec& values)
{
    validate_values(grid, values);
    return Potential1D(grid.x(), values, PotentialKind::Shifted);
}

RVec Potential1D::sample(double control) const
{
    switch (kind_) {
    case PotentialKind::Static:
        return base_;
    case PotentialKind::AmplitudeScaled:
        return base_ * control;
    case PotentialKind::Shifted:
        return sample_shifted(control);
    }
    throw std::runtime_error("Unhandled PotentialKind");
}

RVec Potential1D::sample_shifted(double shift) const
{
    const int n = static_cast<int>(x_.size());
    if (n <= 1) {
        return base_;
    }

    const double dx_value = dx();
    const double span = (x_(n - 1) - x_(0)) + dx_value;
    RVec shifted = RVec::Zero(n);

    for (int i = 0; i < n; ++i) {
        const double source_x = x_(i) + shift;
        double wrapped = std::fmod(source_x - x_(0), span);
        if (wrapped < 0.0) {
            wrapped += span;
        }

        const double scaled = wrapped / dx_value;
        const int idx0 = static_cast<int>(std::floor(scaled)) % n;
        const int idx1 = (idx0 + 1) % n;
        const double frac = scaled - std::floor(scaled);

        shifted(i) = (1.0 - frac) * base_(idx0) + frac * base_(idx1);
    }

    return shifted;
}
