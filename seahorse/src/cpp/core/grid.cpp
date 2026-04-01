#include "grid.hpp"

#include <Eigen/Sparse>

#include <vector>

Grid1D::Grid1D(int dim, double xmin, double xmax)
    : x_(RVec::LinSpaced(dim, xmin, xmax))
    , xmin_(xmin)
    , xmax_(xmax)
    , dx_(dim > 1 ? x_(1) - x_(0) : 0.0)
    , dim_(dim)
{
    if (dim < 5 || dim % 2 != 0) {
        throw std::invalid_argument("Grid1D expects an even dimension >= 5");
    }

    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(static_cast<std::size_t>(dim) * 5U);

    auto wrap = [dim](int idx) {
        int wrapped = idx % dim;
        if (wrapped < 0) {
            wrapped += dim;
        }
        return wrapped;
    };

    for (int row = 0; row < dim; ++row) {
        triplets.emplace_back(row, wrap(row - 2), -1.0);
        triplets.emplace_back(row, wrap(row - 1), 16.0);
        triplets.emplace_back(row, row, -30.0);
        triplets.emplace_back(row, wrap(row + 1), 16.0);
        triplets.emplace_back(row, wrap(row + 2), -1.0);
    }

    kinetic_.resize(dim, dim);
    kinetic_.setFromTriplets(triplets.begin(), triplets.end());
    kinetic_ /= ((dx_ * dx_) * 12.0);
    kinetic_ *= -0.5;
}

