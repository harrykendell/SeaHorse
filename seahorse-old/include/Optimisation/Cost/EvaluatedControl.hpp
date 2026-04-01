#pragma once

#include "src/Physics/Vectors.hpp"
#include "src/Json/nlohmann_json.hpp"

// This object contains the information from evolving a state with a control
struct EvaluatedControl {
    RVec control;
    double cost;
    double fid;
    double norm;

    inline bool operator<(const EvaluatedControl& other) const
    {
        return cost < other.cost;
    }
public:
    friend void to_json(nlohmann ::json& j,
        const EvaluatedControl& ec)
    {
        j["control"] = ec.control;
        j["cost"] = ec.cost;
        j["fid"] = ec.fid;
        j["norm"] = ec.norm;
    }
};
