#include "propagator.hpp"

#include <complex>
#include <utility>

namespace {

RVec momentum_grid(const Grid1D& grid)
{
    const int n = grid.dim();
    const double dp = 2.0 * kPi / grid.dx() / static_cast<double>(n);

    RVec p = RVec::Zero(n);
    for (int i = 0; i < n / 2; ++i) {
        p(i) = dp * static_cast<double>(i);
    }
    for (int i = n / 2; i < n; ++i) {
        p(i) = dp * static_cast<double>(-n + i);
    }
    return p;
}

} // namespace

SplitStep1D::SplitStep1D(const Grid1D& grid, Potential1D potential, double dt, bool use_absorbing_boundary)
    : grid_(grid)
    , potential_(std::move(potential))
    , dt_(dt)
    , psi_(CVec::Zero(grid.dim()))
    , momentum_buffer_(CVec::Zero(grid.dim()))
{
    if (dt_ <= 0.0) {
        throw std::invalid_argument("SplitStep1D expects dt > 0");
    }

    const RVec p = momentum_grid(grid_);
    const RVec kinetic = p.array().square() / 2.0;

    const std::complex<double> half_phase(0.0, -0.5 * dt_);
    const std::complex<double> full_phase(0.0, -dt_);
    kinetic_half_ = (half_phase * kinetic.array()).exp().matrix();
    kinetic_full_ = (full_phase * kinetic.array()).exp().matrix();

    if (use_absorbing_boundary) {
        const RVec strength = 100.0 * (1.0 - planck_taper(RVec::Ones(grid_.dim()), 1.0 / 8.0).array());
        absorbing_ = (-dt_ * strength.array()).exp().matrix();
    } else {
        absorbing_ = RVec::Ones(grid_.dim());
    }
}

void SplitStep1D::reset(const CVec& psi0)
{
    if (psi0.size() != grid_.dim()) {
        throw std::invalid_argument("Initial state size must match the grid dimension");
    }

    const double norm = psi0.norm();
    if (norm == 0.0) {
        throw std::invalid_argument("Initial state must have non-zero norm");
    }

    psi_ = psi0 / norm;
}

void SplitStep1D::step(double control)
{
    fft_.fwd(momentum_buffer_, psi_);
    momentum_buffer_ = (momentum_buffer_.array() * kinetic_half_.array()).matrix();
    fft_.inv(psi_, momentum_buffer_);

    const RVec potential_values = potential_.sample(control);
    const CVec potential_phase = (std::complex<double>(0.0, -dt_) * potential_values.array()).exp().matrix();
    psi_ = (psi_.array() * potential_phase.array() * absorbing_.array()).matrix();

    fft_.fwd(momentum_buffer_, psi_);
    momentum_buffer_ = (momentum_buffer_.array() * kinetic_half_.array()).matrix();
    fft_.inv(psi_, momentum_buffer_);
}

PropagationResult SplitStep1D::propagate(const CVec& psi0, const RVec& control, bool store_path)
{
    if (store_path) {
        reset(psi0);
        CMat trajectory(grid_.dim(), control.size() + 1);
        trajectory.col(0) = psi_;
        for (int i = 0; i < control.size(); ++i) {
            step(control(i));
            trajectory.col(i + 1) = psi_;
        }
        return PropagationResult { psi_, trajectory };
    }

    if (control.size() == 0) {
        reset(psi0);
        return PropagationResult { psi_, CMat() };
    }

    reset(psi0);

    fft_.fwd(momentum_buffer_, psi_);
    momentum_buffer_ = (momentum_buffer_.array() * kinetic_half_.array()).matrix();

    for (int i = 0; i < control.size() - 1; ++i) {
        fft_.inv(psi_, momentum_buffer_);
        const RVec potential_values = potential_.sample(control(i));
        const CVec potential_phase = (std::complex<double>(0.0, -dt_) * potential_values.array()).exp().matrix();
        psi_ = (psi_.array() * potential_phase.array() * absorbing_.array()).matrix();

        fft_.fwd(momentum_buffer_, psi_);
        momentum_buffer_ = (momentum_buffer_.array() * kinetic_full_.array()).matrix();
    }

    fft_.inv(psi_, momentum_buffer_);
    const RVec potential_values = potential_.sample(control(control.size() - 1));
    const CVec potential_phase = (std::complex<double>(0.0, -dt_) * potential_values.array()).exp().matrix();
    psi_ = (psi_.array() * potential_phase.array() * absorbing_.array()).matrix();
    fft_.fwd(momentum_buffer_, psi_);
    momentum_buffer_ = (momentum_buffer_.array() * kinetic_half_.array()).matrix();
    fft_.inv(psi_, momentum_buffer_);

    return PropagationResult { psi_, CMat() };
}
