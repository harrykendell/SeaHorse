/*
 * spline.h
 *
 * simple cubic spline interpolation library without external
 * dependencies
 *
 * ---------------------------------------------------------------------
 * Copyright (C) 2011, 2014, 2016, 2021 Tino Kluge (ttk448 at gmail.com)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------
 *
 */

#pragma once

#include "src/Physics/Vectors.hpp"
#include <assert.h>

namespace tk {

// spline interpolation
class spline {
public:
protected:
    std::vector<double> m_x, m_y; // x,y coordinates of points
    std::vector<double> m_b, m_c, m_d; // spline coefficients
    RVec eigen_b, eigen_c, eigen_d, eigen_x, eigen_y;
    double m_c0; // for left extrapolation
    size_t find_closest(double x) const; // closest idx so that m_x[idx]<=x

public:
    // default constructor: set boundary condition to be zero curvature
    // at both ends, i.e. natural splines
    spline()
    {
        ;
    }
    spline(const std::vector<double>& X, const std::vector<double>& Y)
    {
        this->set_points(X, Y);
    }

    // set all data points
    void set_points(const std::vector<double>& x, const std::vector<double>& y);

    // evaluates the spline at point x
    double operator()(double x) const;
    // evaluates at evenly translated (by x0) points from original x
    RVec resample_shifted(double x0) const;
};

namespace internal {

    // band matrix solver
    class band_matrix {
    private:
        std::vector<std::vector<double>> m_upper; // upper band
        std::vector<std::vector<double>> m_lower; // lower band
    public:
        band_matrix() {}; // constructor
        band_matrix(int dim, int n_u, int n_l); // constructor
        ~band_matrix() {}; // destructor
        void resize(int dim, int n_u, int n_l); // init with dim,n_u,n_l
        int dim() const; // matrix dimension
        int num_upper() const
        {
            return (int)m_upper.size() - 1;
        }
        int num_lower() const
        {
            return (int)m_lower.size() - 1;
        }
        // access operator
        double& operator()(int i, int j); // write
        double operator()(int i, int j) const; // read
        // we can store an additional diagonal (in m_lower)
        double& saved_diag(int i);
        double saved_diag(int i) const;
        void lu_decompose();
        std::vector<double> r_solve(const std::vector<double>& b) const;
        std::vector<double> l_solve(const std::vector<double>& b) const;
        std::vector<double> lu_solve(const std::vector<double>& b,
            bool is_lu_decomposed = false);
    };

    double get_eps();

} // namespace internal
} // namespace tk
