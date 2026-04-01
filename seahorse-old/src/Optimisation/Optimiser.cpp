#include "include/Optimisation/Optimiser.hpp"

// Constructor
Optimiser::Optimiser(Stopper stopper, Cost cost, SaveFn saver)
    : stopper(stopper)
    , cost(cost)
    , saver(saver)
{
    num_iterations = 0;
    steps_since_improvement = 0;
}

Optimiser::~Optimiser() {}

void Optimiser::updateBest(EvaluatedControl& newBest)
{
    if (newBest < bestControl) {
        bestControl = newBest;
        steps_since_improvement = 0;
    }
}
