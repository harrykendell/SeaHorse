#pragma once

#include "src/Physics/Vectors.hpp"

#pragma GCC diagnostic push
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuse-after-free"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#include <libs/eigen/Eigen/Dense>
#include <libs/eigen/Eigen/SparseCore>
#pragma GCC diagnostic pop

class HilbertSpace {
private:
    // Spatial grid
    const RVec m_x;
    const double m_dx = 0;
    const int m_dim = 0;

    // Kinetic energy operator
    Eigen::SparseMatrix<double> m_T;

public:
    // Constructors
    HilbertSpace(int dim, double xmin, double xmax);
    HilbertSpace(int dim, double xlim);

    RVec x() const;
    double dx() const;
    int dim() const;
    Eigen::SparseMatrix<double> T() const; // Kinetic operator
};
