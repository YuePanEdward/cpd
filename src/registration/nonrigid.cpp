/******************************************************************************
* Coherent Point Drift
* Copyright (C) 2014 Pete Gadomski <pete.gadomski@gmail.com>
* 
* Derived from the Coherent Point Drift Matlab package
* Copyright (C) 2008-2009 Andriy Myronenko <myron@csee.ogi.edu>
* https://sites.google.com/site/myronenko/research/cpd
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
******************************************************************************/

#include <cpd/registration/nonrigid.hpp>

#include <cpd/affinity_matrix.hpp>
#include <cpd/sigma2.hpp>


namespace cpd
{
namespace registration
{


Nonrigid::Nonrigid(double tol, int max_it, double outliers, bool use_fgt,
                   double beta, double lambda)
    : Base(tol, max_it, outliers, use_fgt)
    , m_beta(beta)
    , m_lambda(lambda)
{}


SpResult Nonrigid::execute(const arma::mat& X, const arma::mat& Y) const
{
    const arma::uword N = X.n_rows;
    const arma::uword M = Y.n_rows;
    const arma::uword D = Y.n_cols;

    double sigma2 = get_sigma2(X, Y);
    const double sigma2_init = sigma2;

    arma::mat T = Y;
    arma::mat W = arma::zeros<arma::mat>(M, D);
    
    int iter = 0;
    double ntol = get_tol() + 10;
    double L = 0;

    arma::mat G;
    construct_affinity_matrix(Y, Y, get_beta(), G);

    double L_old, Np;
    arma::vec P1(M), Pt1(M);
    arma::mat PX(M, D);

    while (iter < get_max_it() &&
            ntol > get_tol() &&
            sigma2 > 10 * std::numeric_limits<double>::epsilon())
    {
        L_old = L;
        L = find_P(X, T, sigma2, P1, Pt1, PX);

        L = L + get_lambda() / 2 * arma::trace(W.t() * G * W);
        ntol = std::abs((L - L_old) / L);

        // TODO logging
        //std::cerr << "dL= " << ntol << ", iter= " << iter << ", sigma2= " << sigma2 << std::endl;

        arma::sp_mat dP(M, M);
        for (arma::uword i = 0; i < M; ++i)
        {
            dP(i, i) = P1(i);
        }
        W = arma::solve(dP * G + get_lambda() * sigma2 * arma::eye<arma::mat>(M, M), PX - dP * Y);

        T = Y + G * W;

        Np = arma::sum(P1);
        sigma2 = std::abs((arma::accu(arma::pow(X, 2) % arma::repmat(Pt1, 1, D)) +
                           arma::accu(arma::pow(T, 2) % arma::repmat(P1, 1, D)) -
                           2 * arma::trace(PX.t() * T)) /
                (Np * D));

        ++iter;
    }

    SpResult result(new Result());
    result->Y = T;
    return result;
}


}
}