#include "include/Optimisation/Cost/Cost.hpp"

EvaluatedControl Cost::operator()(const RVec& u)
{
    EvaluatedControl eval = { .control = u, .cost = 0, .fid = 0, .norm = 1 };

    // Evaluate the cost of each transfer
    std::complex<double> fid = 0.0;
    for (auto& transfer : transfers) {
        transfer(u);
        fid += transfer.pseudofid;
        eval.norm = std::min(eval.norm, transfer.eval.norm);
        fpp++;
    }
    // fidelity is 1/d^2 |sum(pseudofids)|^2; with pseudofids the piecewise product of matrix elements of the transfer and goal operator
    eval.fid = std::norm(fid) / transfers.size() / transfers.size();
    eval.cost = -eval.fid;

    // Any control costs we want to add
    for (auto& component : components) {
        eval.cost += component(u);
    }

    // Return the EvaluatedControl
    return eval;
}

Cost Cost::operator+(Cost& other)
{
    for (auto& transfer : other.transfers) {
        this->transfers.push_back(std::move(transfer));
    }
    for (auto& component : other.components) {
        this->components.push_back(std::move(component));
    }
    this->fpp += other.fpp;
    return *this;
}

Cost Cost::operator+(StateTransfer st)
{
    this->transfers.push_back(st);
    return *this;
}
Cost operator+(StateTransfer st, Cost c)
{
    return c + st;
}

Cost Cost::operator+(ControlCost cc)
{
    this->components.push_back(cc);
    return *this;
}
Cost operator+(ControlCost cc, Cost c)
{
    return c + cc;
}

Cost operator+(StateTransfer st1, StateTransfer st2)
{
    Cost c = Cost(st1);
    c.addTransferCost(st2);
    return c;
}

Cost operator+(ControlCost cc, StateTransfer st)
{
    Cost c = Cost(cc);
    c.addTransferCost(st);
    return c;
}

Cost operator+(StateTransfer st, ControlCost cc)
{
    Cost c = Cost(cc);
    c.addTransferCost(st);
    return c;
}

Cost operator+(ControlCost cc1, ControlCost cc2)
{
    Cost c = Cost(cc1);
    c.addControlCost(cc2);
    return c;
}