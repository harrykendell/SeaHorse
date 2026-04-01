#include "include/Optimisation/dCRAB.hpp"
#include "src/Utils/Logger.hpp"

/*
TODO
check we align with the paper
https://arxiv.org/pdf/1506.04601.pdf
*/

dCRAB::dCRAB(Basis& basis, Stopper& stopper, Cost& cost,
    SaveFn saver)
    : Optimiser(stopper, cost, saver)
    , basis(std::make_unique<Basis>(basis))
{
    dCRAB::init();
}

EvaluatedControl dCRAB::evaluate(RVec coeffs)
{
    RVec control = basis->control(coeffs);
    for (auto& [dressedCoeffs, dressedBasis] : dressedBases) {
        control += dressedBasis->control(dressedCoeffs);
    }
    return cost(control);
}

// No saver
dCRAB::dCRAB(Basis& basis, Stopper& stopper, Cost& cost)
    : dCRAB(basis, stopper, cost, [](const Optimiser& opt) { S_LOG(opt.num_iterations, "\tfid= ", opt.bestControl.fid, "\tcost= ", opt.bestControl.cost); })
{
}
void dCRAB::generateSimplex()
{
    // Ensure the simplex is empty
    simplex.clear();

    // Initial point for simplex
    RVec initialControl = basis->randomCoeffs() * 0;
    auto eval = evaluate(initialControl);
    simplex.push_back({ initialControl, eval.cost });
    updateBest(eval);

    // Rest of the simplex around the initial guess (n+1 points for n-dim basis)
    for (auto _ = 0; _ <= basis->num_coeffs(); _++) {
        RVec temp = basis->randomCoeffs();
        auto eval = evaluate(temp);
        simplex.push_back({ temp, eval.cost });
        updateBest(eval);
    }

    // order simplex by cost
    std::sort(
        simplex.begin(), simplex.end(),
        [](const auto& a, const auto& b) { return (a.cost < b.cost); });
}

void dCRAB::init()
{
    num_iterations = 0;
    steps_since_improvement = 0;

    // generate the simplex
    generateSimplex();
}

void dCRAB::step()
{
    num_iterations++;
    steps_since_improvement++;
    // centroid of simplex except worst point
    RVec centroid_pt = RVec::Zero(basis->num_coeffs());
    for (auto it_pt = simplex.begin(); it_pt != (simplex.end() - 1); it_pt++) {
        centroid_pt += (*it_pt).coeffs;
    }
    centroid_pt /= (simplex.size() - 1);

    // REFLECTION
    RVec reflection_pt = centroid_pt + alpha * (centroid_pt - simplex.back().coeffs);

    // evaluate cost at reflection point
    auto eval = evaluate(reflection_pt);
    SimplexPoint reflected = { reflection_pt, eval.cost };

    // reflection is better than second worst point but worse than best point
    if ((simplex.front() < reflected) && (reflected < simplex[simplex.size() - 2])) {
        // S_LOG("Reflection replaces worst point");
        simplex.pop_back();
        simplex.push_back(reflected);
        updateBest(eval);
        return;
    }

    // EXPANSION
    // reflection is better than best point
    if (reflected < simplex.front()) {
        // we need to save this as we are about to evolve again
        EvaluatedControl reflectedEC = eval;

        RVec expansion_pt = centroid_pt + gamma * (reflection_pt - centroid_pt);
        auto eval = evaluate(expansion_pt);
        SimplexPoint expanded = { expansion_pt, eval.cost };

        simplex.pop_back();
        // expansion is better than reflection
        if (expanded < reflected) {
            // S_LOG("Expansion replaces worst point");
            simplex.push_back(expanded);
            updateBest(eval);
        } else {
            // S_LOG("Reflection replaces worst point after checking expansion");
            simplex.push_back(reflected);
            updateBest(reflectedEC);
        }
        return;
    }

    // CONTRACTION
    // reflection is better than worst point
    if (reflected < simplex.back()) {
        RVec contraction_pt = centroid_pt + rho * (reflection_pt - centroid_pt);
        auto eval = evaluate(contraction_pt);
        SimplexPoint contracted = { contraction_pt, eval.cost };

        // contraction is better than reflection
        if (contracted < reflected) {
            // S_LOG("Contraction around reflection replaces worst point");
            simplex.pop_back();
            simplex.push_back(contracted);
            updateBest(eval);
            return;
        }
        // reflection worse than worst
    } else if (simplex.back() < reflected) {
        RVec contraction_pt = centroid_pt + rho * (simplex.back().coeffs - centroid_pt);
        auto eval = evaluate(contraction_pt);
        SimplexPoint contracted = { contraction_pt, eval.cost };

        // contraction is better than worst
        if (contracted < simplex.back()) {
            // S_LOG("Contraction around worst replaces worst point");
            simplex.pop_back();
            simplex.push_back(contracted);
            updateBest(eval);
            return;
        }
    }

    // SHRINK
    // shrink all points towards best point
    for (size_t i = 1; i < simplex.size(); i++) {
        RVec new_pt = simplex.front().coeffs + sigma * (simplex[i].coeffs - simplex.front().coeffs);
        auto eval = evaluate(new_pt);
        simplex[i] = { new_pt, eval.cost };
        updateBest(eval);
    }
    // S_LOG("Shrink");
    return;
}

bool dCRAB::simplexSize(double sizeEpsilon) const
{
    // find the middle of the simplex
    RVec centroid_pt = RVec::Zero(basis->num_coeffs());
    for (auto it_pt = simplex.begin(); it_pt != (simplex.end() - 1); it_pt++) {
        centroid_pt += (*it_pt).coeffs;
    }
    centroid_pt /= (simplex.size() - 1);
    // find the distance from the centroid to each point
    double sum = 0;
    for (auto it_pt = simplex.begin(); it_pt != (simplex.end() - 1); it_pt++) {
        sum += (centroid_pt - (*it_pt).coeffs).norm();
    }
    // find the average distance
    double avg = sum / (simplex.size() - 1);

    return (avg < sizeEpsilon);
}

bool dCRAB::flatCost(double costEpsilon) const
{
    return (simplex.back().cost - simplex.front().cost) < costEpsilon;
}

void dCRAB::optimise() { optimise(0); }
void dCRAB::optimise(int dressings)
{
    S_LOG("dCRAB optimise using a basis size of ", basis->num_basis_vectors(), " dressing ", dressings, " times");

    // Early checking of stopper and saving incase we initialise with a good
    // control

    while ((int)dressedBases.size() < dressings) {
        // order simplex by cost
        std::sort(
            simplex.begin(), simplex.end(),
            [](const auto& a, const auto& b) { return (a.cost < b.cost); });

        // update our number of full path propagations
        fpp = cost.fpp;

        // save data from control if we have a saver
        if (saver) {
            saver(*this);
        }

        // break if we satisfy contraints
        if (stopper(*this)) {
            this->regenerateBasis();
        }

        // if the simplex volume is too small we stop
        else if (simplexSize(1e-3)) {
            S_LOG("STOPPING: ", "Simplex too small");
            this->regenerateBasis();
        }

        // if the cost is flat we stop
        else if (flatCost(1e-5)) {
            S_LOG("STOPPING: ", "Cost landscape is flat");
            this->regenerateBasis();
        }

        step();
    }
    S_LOG("dCRAB finished with {", num_iterations, " iters, ", fpp, " fpps, ",
        bestControl.fid, " fid, ", bestControl.norm, " norm, ",
        bestControl.cost, " cost}");
}

void dCRAB::regenerateBasis()
{
    S_LOG("Dressing the basis (", dressedBases.size(), " times)");
    // save the current basis
    // generate a new basis
    auto temp_basis = basis->generateNewBasis();
    dressedBases.push_back(
        std::make_tuple(simplex.front().coeffs, std::move(basis)));
    basis = std::move(temp_basis);
    // clear the simplex and reinitialise
    this->generateSimplex();

    steps_since_improvement = 0;
}