#pragma once

#include "include/Optimisation/Cost/Cost.hpp"
#include "include/Optimisation/Stopper/Stopper.hpp"

class Optimiser;
typedef std::function<void(const Optimiser&)> SaveFn;

class Optimiser {
private:
    // Stopper
    Stopper stopper;
    // Cost
    Cost cost;
    // Saver
    SaveFn saver;

public:
    friend class dCRAB;

    EvaluatedControl bestControl {.control=RVec::Zero(0), .cost=std::numeric_limits<double>::infinity(), .fid=0.0, .norm=0.0};
    // Num iterations
    int num_iterations;
    // steps since improvement
    int steps_since_improvement;
    // Full path propagations
    int fpp = 0;

    // Constructor
    Optimiser(Stopper stopper, Cost cost, SaveFn saver);

    virtual ~Optimiser() = 0;

    // perform any initialisation that we may wish to reset
    virtual void init() = 0;

    // Run optimisation
    virtual void optimise() = 0;
    virtual void step() = 0;

private:
    void updateBest(EvaluatedControl&);
};
