#pragma once
#include <iostream>
#include <functional>

class Optimiser;

class StopComponent {
private:
    friend class Stopper;
    // Description of the stopping criteria
    std::string text;
    // The actual stopping criteria
    std::function<bool(const Optimiser&)> criteria;

    // Evaluate if we want to stop
    bool operator()(Optimiser& opt);

public:
    // Constructor
    inline StopComponent(std::string text, std::function<bool(const Optimiser&)> criteria)
        : text(text)
        , criteria(criteria) {};
};

StopComponent FidStopper(double fid);
StopComponent IterStopper(int maxIter);
StopComponent StallStopper(int stepsSinceImprovement);