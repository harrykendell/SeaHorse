#include "spectrum.hpp"

#include <Eigen/Eigenvalues>

#include <Spectra/MatOp/SparseSymShiftSolve.h>
#include <Spectra/SymEigsShiftSolver.h>

#include <algorithm>
#include <cmath>

namespace {

SparseMat build_hamiltonian(const Grid1D& grid, const RVec& potential_values)
{
    SparseMat h = grid.kinetic();
    h.diagonal() += potential_values;
    return h;
}

void canonicalise_eigenvectors(RMat& eigenvectors)
{
    for (Eigen::Index i = 0; i < eigenvectors.cols(); ++i) {
        const double norm = eigenvectors.col(i).norm();
        if (norm != 0.0) {
            eigenvectors.col(i) /= norm;
        }

        const Eigen::Index half = std::max<Eigen::Index>(1, eigenvectors.rows() / 2);
        const double bias = eigenvectors.col(i).head(half).mean();
        const double parity = (i % 2 == 0) ? 1.0 : -1.0;
        if (bias * parity < 0.0) {
            eigenvectors.col(i) *= -1.0;
        }
    }
}

} // namespace

SpectrumResult solve_spectrum(
    const Grid1D& grid,
    const Potential1D& potential,
    int num_states,
    double shift_guess)
{
    if (num_states <= 0) {
        throw std::invalid_argument("num_states must be positive");
    }

    const RVec potential_values = potential.sample(0.0);
    SparseMat h = build_hamiltonian(grid, potential_values);
    const int dim = grid.dim();

    if (num_states >= dim - 1) {
        const RMat dense_h = RMat(h);
        Eigen::SelfAdjointEigenSolver<RMat> solver(dense_h);
        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Full eigensolve failed");
        }

        SpectrumResult result;
        result.eigenvalues = solver.eigenvalues().head(std::min(num_states, dim));
        result.eigenvectors = solver.eigenvectors().leftCols(std::min(num_states, dim));
        canonicalise_eigenvectors(result.eigenvectors);
        return result;
    }

    const int nev = std::min(std::max(num_states, 1), dim - 1);
    const int ncv = std::min(std::max(nev * 2 + 1, 20), dim);

    if (std::isnan(shift_guess)) {
        shift_guess = std::min(0.0, potential_values.minCoeff());
    }

    Spectra::SparseSymShiftSolve<double> op(h);
    Spectra::SymEigsShiftSolver<Spectra::SparseSymShiftSolve<double>> eigs(op, nev, ncv, shift_guess);

    eigs.init();
    const int nconv = eigs.compute(Spectra::SortRule::LargestAlge);

    if (nconv <= 0) {
        throw std::runtime_error("Shift-invert eigensolve did not converge");
    }

    SpectrumResult result;
    result.eigenvalues = eigs.eigenvalues().reverse();
    result.eigenvectors = eigs.eigenvectors().rowwise().reverse();
    canonicalise_eigenvectors(result.eigenvectors);
    return result;
}
