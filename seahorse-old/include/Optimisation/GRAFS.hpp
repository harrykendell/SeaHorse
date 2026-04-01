#pragma once

#include "include/Optimisation/Optimiser.hpp"

/*
TODO
        - step size
                https://en.wikipedia.org/wiki/Backtracking_line_search
        - parameterised basis
                https://journals.aps.org/pra/pdf/10.1103/PhysRevA.97.062346
                Maybe a Python example from qutip:
                        https://qutip.org/docs/4.0.2/modules/qutip/control/pulseoptim.html#optimize_pulse
                        /Users/User/anaconda3/lib/python3.11/site-packages/qutip_qtrl/grape.py
                Another example from goat? they calculate gradients for fourier
series at least
*/

class GRAFS : public Optimiser {
private:
    RVec control;

  public:
      GRAFS(RVec control, Stopper& stopper, Cost& cost, SaveFn saver);
      // No Saver
      GRAFS(RVec control, Stopper &stopper, Cost &cost);
      
      void optimise() override;

      void init() override;
      void step() override;
};
