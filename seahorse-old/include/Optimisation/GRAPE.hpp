#pragma once

#include "include/Optimisation/Optimiser.hpp"

/*
TODO
        - main paper
                https://arxiv.org/pdf/2307.08479.pdf
        -More basics
                https://michaelgoerz.net/research/grape_june_2010_slides.pdf

        - We could do newton or bfgs instead of gradient descent
                https://doi.org/10.1063/1.4949534
                      NB table 1 here is useful for gradients of extras costs
                https://doi.org/10.1103/PhysRevResearch.5.L012042

        - step size
                https://en.wikipedia.org/wiki/Backtracking_line_search
series at least
*/

class GRAPE : public Optimiser {
private:
    RVec control;

public:
    GRAPE(RVec control, Stopper& stopper, Cost& cost, SaveFn saver);
    // No Saver
    GRAPE(RVec control, Stopper& stopper, Cost& cost);

    void optimise() override;

    void init() override;
    void step() override;
};
