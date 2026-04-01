#include "include/Physics/HilbertSpace.hpp"
#include "src/Utils/Logger.hpp"

// Constructors
HilbertSpace::HilbertSpace(int dim, double xmin, double xmax)
    : m_x(Eigen::VectorXd::LinSpaced(dim, xmin, xmax))
    , m_dx(m_x[1] - m_x[0])
    , m_dim(dim)
{
    if (dim % 2 != 0 || dim < 5)
        S_FATAL("Dimension of Hilbert Space should be a multiple of 2 but is ", dim);
    // 4th order central difference approx for d2/dx2
    Eigen::SparseMatrix<double> Laplace(dim, dim);
    Laplace.reserve(Eigen::VectorXi::Constant(dim, 5));

    RMat Lap = RMat::Zero(dim, dim);

    // Set diagonals with periodic boundary conditions
    Lap.diagonal(-2).setConstant(-1);
    Lap(0, dim - 2) = -1;
    Lap(1, dim - 1) = -1;

    Lap.diagonal(-1).setConstant(16);
    Lap(0, dim - 1) = 16;

    Lap.diagonal().setConstant(-30);

    Lap.diagonal(1).setConstant(16);
    Lap(dim - 1, 0) = 16;

    Lap.diagonal(2).setConstant(-1);
    Lap(dim - 1, 1) = -1;
    Lap(dim - 2, 0) = -1;

    Laplace = Lap.sparseView();

    Laplace /= ((m_dx * m_dx) * 12); // rescale derivative
    m_T = Laplace * (-0.5); // p^2/2m = -(d^2/dx^2)(h_bar)^2/2m  with m=1,h_bar=1
}

HilbertSpace::HilbertSpace(int dim, double xlim)
    : HilbertSpace(dim, -xlim, xlim) {}

RVec HilbertSpace::x() const { return m_x; }
double HilbertSpace::dx() const { return m_dx; }
int HilbertSpace::dim() const { return m_dim; }
Eigen::SparseMatrix<double> HilbertSpace::T() const { return m_T; } // Kinetic operator
