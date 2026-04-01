
#include "include/Physics/Spline.hpp"

namespace tk {

    // spline implementation
    // -----------------------

    void spline::set_points(const std::vector<double>& x,
        const std::vector<double>& y)
    {
        assert(x.size() == y.size());
        assert(x.size() >= 3);
        m_x = x;
        m_y = y;
        int n = (int)x.size();

        // classical cubic splines which are C^2 (twice cont differentiable)
        // this requires solving an equation system

        // setting up the matrix and right hand side of the equation system
        // for the parameters b[]
        internal::band_matrix A(n, 1, 1);
        std::vector<double> rhs(n);
        for (int i = 1; i < n - 1; i++) {
            A(i, i - 1) = 1.0 / 3.0 * (x[i] - x[i - 1]);
            A(i, i) = 2.0 / 3.0 * (x[i + 1] - x[i - 1]);
            A(i, i + 1) = 1.0 / 3.0 * (x[i + 1] - x[i]);
            rhs[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]);
        }
        // boundary conditions for second deriv
        // 2*c[0] = f''
        A(0, 0) = 2.0;
        A(0, 1) = 0.0;
        rhs[0] = 0.0;

        // 2*c[n-1] = f''
        A(n - 1, n - 1) = 2.0;
        A(n - 1, n - 2) = 0.0;
        rhs[n - 1] = 0.0;

        // solve the equation system to obtain the parameters c[]
        m_c = A.lu_solve(rhs);

        // calculate parameters b[] and d[] based on c[]
        m_d.resize(n);
        m_b.resize(n);
        for (int i = 0; i < n - 1; i++) {
            m_d[i] = 1.0 / 3.0 * (m_c[i + 1] - m_c[i]) / (x[i + 1] - x[i]);
            m_b[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i])
                - 1.0 / 3.0 * (2.0 * m_c[i] + m_c[i + 1]) * (x[i + 1] - x[i]);
        }
        // for the right extrapolation coefficients (zero cubic term)
        // f_{n-1}(x) = y_{n-1} + b*(x-x_{n-1}) + c*(x-x_{n-1})^2
        double h = x[n - 1] - x[n - 2];
        // m_c[n-1] is determined by the boundary condition
        m_d[n - 1] = 0.0;
        m_b[n - 1] = 3.0 * m_d[n - 2] * h * h + 2.0 * m_c[n - 2] * h + m_b[n - 2]; // = f'_{n-2}(x_{n-1})

        // Save Eigen vectors too
        eigen_d = RVec::Map(m_d.data(), m_d.size());
        eigen_b = RVec::Map(m_b.data(), m_b.size());
        eigen_c = RVec::Map(m_c.data(), m_c.size());
        eigen_y = RVec::Map(m_y.data(), m_y.size());
        eigen_x = RVec::Map(m_x.data(), m_x.size());
    }

    // return the closest idx so that m_x[idx] <= x (return 0 if x<m_x[0])
    size_t spline::find_closest(double x) const
    {
        std::vector<double>::const_iterator it;
        it = std::upper_bound(m_x.begin(), m_x.end(), x); // *it > x
        size_t idx = std::max(int(it - m_x.begin()) - 1, 0); // m_x[idx] <= x
        return idx;
    }

    double spline::operator()(double x) const
    {
        // polynomial evaluation using Horner's scheme
        // TODO: consider more numerically accurate algorithms, e.g.:
        //   - Clenshaw
        //   - Even-Odd method by A.C.R. Newbery
        //   - Compensated Horner Scheme
        size_t n = m_x.size();
        size_t idx = find_closest(x);

        double h = x - m_x[idx];

        double interpol;
        if (x < m_x[0]) {
            // extrapolation to the left
            interpol = (m_c[0] * h + m_b[0]) * h + m_y[0];
        } else if (x > m_x[n - 1]) {
            // extrapolation to the right
            interpol = (m_c[n - 1] * h + m_b[n - 1]) * h + m_y[n - 1];
        } else {
            // interpolation
            interpol = ((m_d[idx] * h + m_c[idx]) * h + m_b[idx]) * h + m_y[idx];
        }
        return interpol;
    }

#define seg(name) eigen_##name.segment(idx, n)
    RVec spline::resample_shifted(double x0) const
    {
        // we want to resample in parallel using eigen's efficient expressions
        // This assumes all points in x are evenly spaced

        static size_t n = eigen_x.size() / 3;
        // find where our minimum x-value translated to
        double h = eigen_x[n] - x0;
        size_t idx = find_closest(h);
        h -= m_x[idx];

        static auto fix_eps = [](double val) { return (abs(val) < 1e-16) ? 0. : val; };
        return RVec((h * (h * (h * seg(d) + seg(c)) + seg(b)) + seg(y)).unaryExpr(fix_eps));
    }
#undef seg
namespace internal {

    // band_matrix implementation
    // -------------------------

    band_matrix::band_matrix(int dim, int n_u, int n_l)
    {
        resize(dim, n_u, n_l);
    }
    void band_matrix::resize(int dim, int n_u, int n_l)
    {
        assert(dim > 0);
        assert(n_u >= 0);
        assert(n_l >= 0);
        m_upper.resize(n_u + 1);
        m_lower.resize(n_l + 1);
        for (size_t i = 0; i < m_upper.size(); i++) {
            m_upper[i].resize(dim);
        }
        for (size_t i = 0; i < m_lower.size(); i++) {
            m_lower[i].resize(dim);
        }
    }
    int band_matrix::dim() const
    {
        if (m_upper.size() > 0) {
            return m_upper[0].size();
        } else {
            return 0;
        }
    }

    // defines the new operator (), so that we can access the elements
    // by A(i,j), index going from i=0,...,dim()-1
    double& band_matrix::operator()(int i, int j)
    {
        int k = j - i; // what band is the entry
        assert((i >= 0) && (i < dim()) && (j >= 0) && (j < dim()));
        assert((-num_lower() <= k) && (k <= num_upper()));
        // k=0 -> diagonal, k<0 lower left part, k>0 upper right part
        if (k >= 0)
            return m_upper[k][i];
        else
            return m_lower[-k][i];
    }
    double band_matrix::operator()(int i, int j) const
    {
        int k = j - i; // what band is the entry
        assert((i >= 0) && (i < dim()) && (j >= 0) && (j < dim()));
        assert((-num_lower() <= k) && (k <= num_upper()));
        // k=0 -> diagonal, k<0 lower left part, k>0 upper right part
        if (k >= 0)
            return m_upper[k][i];
        else
            return m_lower[-k][i];
    }
    // second diag (used in LU decomposition), saved in m_lower
    double band_matrix::saved_diag(int i) const
    {
        assert((i >= 0) && (i < dim()));
        return m_lower[0][i];
    }
    double& band_matrix::saved_diag(int i)
    {
        assert((i >= 0) && (i < dim()));
        return m_lower[0][i];
    }

    // LR-Decomposition of a band matrix
    void band_matrix::lu_decompose()
    {
        int i_max, j_max;
        int j_min;
        double x;

        // preconditioning
        // normalize column i so that a_ii=1
        for (int i = 0; i < this->dim(); i++) {
            assert(this->operator()(i, i) != 0.0);
            this->saved_diag(i) = 1.0 / this->operator()(i, i);
            j_min = std::max(0, i - this->num_lower());
            j_max = std::min(this->dim() - 1, i + this->num_upper());
            for (int j = j_min; j <= j_max; j++) {
                this->operator()(i, j) *= this->saved_diag(i);
            }
            this->operator()(i, i) = 1.0; // prevents rounding errors
        }

        // Gauss LR-Decomposition
        for (int k = 0; k < this->dim(); k++) {
            i_max = std::min(this->dim() - 1, k + this->num_lower()); // num_lower not a mistake!
            for (int i = k + 1; i <= i_max; i++) {
                assert(this->operator()(k, k) != 0.0);
                x = -this->operator()(i, k) / this->operator()(k, k);
                this->operator()(i, k) = -x; // assembly part of L
                j_max = std::min(this->dim() - 1, k + this->num_upper());
                for (int j = k + 1; j <= j_max; j++) {
                    // assembly part of R
                    this->operator()(i, j) = this->operator()(i, j) + x * this->operator()(k, j);
                }
            }
        }
    }
    // solves Ly=b
    std::vector<double> band_matrix::l_solve(const std::vector<double>& b) const
    {
        assert(this->dim() == (int)b.size());
        std::vector<double> x(this->dim());
        int j_start;
        double sum;
        for (int i = 0; i < this->dim(); i++) {
            sum = 0;
            j_start = std::max(0, i - this->num_lower());
            for (int j = j_start; j < i; j++)
                sum += this->operator()(i, j) * x[j];
            x[i] = (b[i] * this->saved_diag(i)) - sum;
        }
        return x;
    }
    // solves Rx=y
    std::vector<double> band_matrix::r_solve(const std::vector<double>& b) const
    {
        assert(this->dim() == (int)b.size());
        std::vector<double> x(this->dim());
        int j_stop;
        double sum;
        for (int i = this->dim() - 1; i >= 0; i--) {
            sum = 0;
            j_stop = std::min(this->dim() - 1, i + this->num_upper());
            for (int j = i + 1; j <= j_stop; j++)
                sum += this->operator()(i, j) * x[j];
            x[i] = (b[i] - sum) / this->operator()(i, i);
        }
        return x;
    }

    std::vector<double> band_matrix::lu_solve(const std::vector<double>& b,
        bool is_lu_decomposed)
    {
        assert(this->dim() == (int)b.size());
        std::vector<double> x, y;
        if (is_lu_decomposed == false) {
            this->lu_decompose();
        }
        y = this->l_solve(b);
        x = this->r_solve(y);
        return x;
    }

    // machine precision of a double, i.e. the successor of 1 is 1+eps
    double get_eps()
    {
        return std::numeric_limits<double>::epsilon(); // __DBL_EPSILON__
        return 2.2204460492503131e-16; // 2^-52
    }

} // namespace internal

} // namespace tk
