#pragma once

#include "include/Physics/HilbertSpace.hpp"
#include "include/Physics/Spline.hpp"
#include "src/Json/nlohmann_json.hpp"
#include "src/Physics/Vectors.hpp"

// Potentials hold some potential function V(x) defined on real space
// They have three flavours currently - Constant, Shaken, and AmplitudeModulated
// Otherwise a custom one can be supplied

// Non-custom ones support serialisation
// This is implemented by the operator() which transforms and returns their internal V(x)
class Potential {
public:
    enum class Type {
        // Always V
        CONSTANT,
        // V * control
        AMPLITUDE,
        // V(x + x0)
        SHAKEN,
        // Defined by provided std::function<RVec(double)>
        CUSTOM
    };

    friend void to_json(nlohmann ::json& j,
        const Potential& pot)
    {
        j = pot.m_V;
    }

private:
    RVec m_V;
    Type m_type;
    // Shaken potentials use splines to interpolate
    RVec m_x;
    tk::spline spline;

    // Slow fallback to this for custom potentials
    std::function<RVec(double)> m_Vfn = nullptr;

private:
    RVec AmplitudeModulatedV(double control) const;
    // we use cubic spline interpolation to translate the vector
    RVec ShakenV(double x0) const;

    void initSpline();

public:
    RVec operator()(double control) const;

    // copy constructor
    Potential(const Potential& other);
    // move constructor
    Potential(Potential&& other);
    // Custom control methods
    Potential(std::function<RVec(double)> V)
        : m_type(Type::CUSTOM)
        , m_Vfn(V) {};
    // Specific control methods
    Potential(const RVec& x, const RVec& V, Type type);
};

// helpers to avoid specifying the type
Potential ConstantPotential(HilbertSpace& hs, const RVec& V);
Potential AmplitudePotential(HilbertSpace& hs, const RVec& V);
Potential ShakenPotential(HilbertSpace& hs, const RVec& V);