/*
    NOTE: Because we need to use this as both declaration and definitions everything must
    be static/inline to prevent the symbols being duplicated when linking againts libseahorse.
*/

#pragma once
#include "src/Utils/Globals.hpp"
#include <complex>
// We have to specifically suppress these warnings because of the Eigen library
#pragma GCC diagnostic push
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
#endif
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wenum-compare"
#include <libs/eigen/Eigen/Core>
#pragma GCC diagnostic pop

using RVec = Eigen::VectorXd;
using CVec = Eigen::VectorXcd;
using RMat = Eigen::MatrixXd;
using CMat = Eigen::MatrixXcd;

// Allow maths style notation on real/complex Vecs + Scalars
// We promote the type to complex only where needed
// This is done by casting to the Vector's type when adding doubles but casting the vectors to complex when adding complex
template <typename Derived>
inline auto operator+(const Eigen::MatrixBase<Derived>& v, double d) { return (v.array() + static_cast<typename Derived::Scalar>(d)).matrix(); }
template <typename Derived>
inline auto operator+(double d, const Eigen::MatrixBase<Derived>& v) { return (static_cast<typename Derived::Scalar>(d) + v.array()).matrix(); }
template <typename Derived>
inline auto operator-(double d, const Eigen::MatrixBase<Derived>& v) { return (static_cast<typename Derived::Scalar>(d) - v.array()).matrix(); }
template <typename Derived>
inline auto operator-(const Eigen::MatrixBase<Derived>& v, double d) { return (v.array() - static_cast<typename Derived::Scalar>(d)).matrix(); }

template <typename Derived>
inline auto operator+(const Eigen::MatrixBase<Derived>& v, std::complex<double> d) { return (v.template cast<std::complex<double>>().array() + d).matrix(); }
template <typename Derived>
inline auto operator+(std::complex<double> d, const Eigen::MatrixBase<Derived>& v) { return (d + v.template cast<std::complex<double>>().array()).matrix(); }
template <typename Derived>
inline auto operator-(std::complex<double> d, const Eigen::MatrixBase<Derived>& v) { return (d - v.template cast<std::complex<double>>().array()).matrix(); }
template <typename Derived>
inline auto operator-(const Eigen::MatrixBase<Derived>& v, std::complex<double> d) { return (v.template cast<std::complex<double>>().array() - d).matrix(); }

inline auto pow(const RVec& v, const double exponent) { return v.cwisePow(exponent); }
inline auto exp(const RVec& v) { return v.array().exp(); }
inline auto exp(const CVec& v) { return v.array().exp(); }
inline auto abs2(const RVec& v) { return v.array().abs2(); }
inline auto abs2(const CVec& v) { return v.array().abs2(); }

template <typename Derived, typename Derived2>
inline auto
fidelity(const Eigen::MatrixBase<Derived>& psi_1,
    const Eigen::MatrixBase<Derived2>& psi_2)
{
    return std::norm(psi_1.conjugate().dot(psi_2));
}

template <typename Derived, typename Derived2>
inline auto
overlap(const Eigen::MatrixBase<Derived>& psi_1,
    const Eigen::MatrixBase<Derived2>& psi_2)
{
    return psi_1.conjugate().dot(psi_2);
}

// These only really make sense on the Reals
inline auto cos(const RVec& v) { return v.array().cos(); }
inline auto sin(const RVec& v) { return v.array().sin(); }
inline auto box(const RVec& x, double min, double max)
{
    return (x.array().cwiseLess(max).cast<double>() + x.array().cwiseGreater(min).cast<double>()) - 1;
}
inline auto planck_taper(const RVec& v, double taper_ratio = 1.0 / 6.0)
{
    auto N = v.size();
    RVec ret = RVec::Ones(N);

    for (int i = 0; i < taper_ratio * N / 2; i++) {
        ret(i) = 0.5 * (1.0 - cos(2 * PI * i / (taper_ratio * N)));
        ret(N - i - 1) = ret(i);
    }

    return RVec(v.array() * ret.array());
}