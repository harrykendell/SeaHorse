#pragma once

#include "include/Optimisation/Basis/Basis.hpp"
#include "include/Optimisation/Optimiser.hpp"

// Implementation of a dressed CRAB optimiser
class dCRAB : public Optimiser {
private:
    // Nelder-mead meta-parameters
    // static constexpr double stepSize = 0.2;
    static constexpr double alpha = 1.0;
    static constexpr double gamma = 2.0;
    static constexpr double rho = 0.5;
    static constexpr double sigma = 0.5;

    struct SimplexPoint {
        RVec coeffs;
        double cost;

        bool operator<(const SimplexPoint& other) const
        {
            return cost < other.cost;
        }
    };

    // Holds the simplex of control coefficients
    std::vector<SimplexPoint> simplex;

    // Holds the basis states as columns
    std::unique_ptr<Basis> basis;
    // The basis states we are done with
    std::vector<std::tuple<RVec, std::unique_ptr<Basis>>> dressedBases;

    void generateSimplex();

    // our own internal stop criteria
    // If all points have the same cost then its flat and we should probably stop
    bool flatCost(double costEpsilon) const;
    // this is the average distance from the centroid to each point
    bool simplexSize(double sizeEpsilon) const;

    // Dress the basis
    void regenerateBasis();

    EvaluatedControl evaluate(RVec coeffs);

public:
    dCRAB(Basis& basis, Stopper& stopper, Cost& cost, SaveFn saver);
    // No Saver
    dCRAB(Basis& basis, Stopper& stopper, Cost& cost);

    // Run optimisation - Basis size inferred from basis
    void optimise() override;
    void optimise(int dressings);

    void init() override;
    void step() override;
};