#include "include/Optimisation/Optimiser.hpp"
#include <string>

bool StopComponent::operator()(Optimiser& opt) { return criteria(opt); }

StopComponent FidStopper(double fid)
{
    std::function<bool(const Optimiser&)> criteria = [=](const Optimiser& opt) {
        return opt.bestControl.fid > fid;
    };
    std::string text = "Fidelity Criterion Reached (" + std::to_string(fid) + ")";
    return StopComponent(text, std::move(criteria));
}

StopComponent IterStopper(int iter)
{
    std::function<bool(const Optimiser&)> criteria = [=](const Optimiser& opt) {
        return opt.num_iterations >= iter;
    };
    std::string text = "Max Iterations Reached (" + std::to_string(iter) + ")";
    return StopComponent(text, std::move(criteria));
}

StopComponent StallStopper(int stepsSinceImprovement)
{
    std::function<bool(const Optimiser&)> criteria = [=](const Optimiser& opt) {
        return opt.steps_since_improvement > stepsSinceImprovement;
    };
    std::string text = "No Improvement for " + std::to_string(stepsSinceImprovement) + " steps";
    return StopComponent(text, std::move(criteria));
}
