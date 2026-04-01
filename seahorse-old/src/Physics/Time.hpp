// Holds a series of time points, primarily for use in a control.
// Provided to try and avoid off by one issues etc.

#pragma once

#include "src/Json/nlohmann_json.hpp"
#include "src/Physics/Vectors.hpp"

// Note numSteps is the number of steps, not the number of points (which is
// numSteps + 1)
// We therefore run from 0 to numSteps*dt inclusive
struct Time {
    const double dt;
    const int numSteps;

public:
    inline Time(double dt, int numSteps)
        : dt(dt)
        , numSteps(numSteps)
    {
    }

  public:
    // allow implicit conversion to its RVec representation
    inline operator RVec() const
    {
        return RVec::LinSpaced(numSteps + 1, 0, dt * numSteps);
    }

    
    inline RVec& operator()() const
    {
        static RVec t = RVec::LinSpaced(numSteps + 1, 0, dt * numSteps);
        return t;
    }

    friend void to_json(nlohmann ::json& j,
        const Time& t)
    {
        j = t();
    }
};

inline Time makeTime(double dt, int numSteps)
{
    return Time(dt, numSteps);
}