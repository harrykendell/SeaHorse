#pragma once

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <complex>
#include <stdexcept>

using RVec = Eigen::VectorXd;
using CVec = Eigen::VectorXcd;
using RMat = Eigen::MatrixXd;
using CMat = Eigen::MatrixXcd;
using SparseMat = Eigen::SparseMatrix<double>;

inline constexpr double kPi = 3.141592653589793238462643383279502884;

inline std::complex<double> overlap(const CVec& lhs, const CVec& rhs)
{
    return lhs.conjugate().dot(rhs);
}

inline double fidelity(const CVec& lhs, const CVec& rhs)
{
    return std::norm(overlap(lhs, rhs));
}

inline RVec planck_taper(const RVec& values, double taper_ratio = 1.0 / 6.0)
{
    RVec taper = RVec::Ones(values.size());
    const int width = static_cast<int>(taper_ratio * values.size() / 2.0);
    if (width <= 0) {
        return values;
    }

    for (int i = 0; i < width; ++i) {
        taper(i) = 0.5 * (1.0 - std::cos(2.0 * kPi * i / (taper_ratio * values.size())));
        taper(values.size() - i - 1) = taper(i);
    }
    return values.array() * taper.array();
}

