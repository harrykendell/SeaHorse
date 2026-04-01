#include "include/Physics/Potential.hpp"
#include "src/Utils/Logger.hpp"

RVec Potential::AmplitudeModulatedV(double amp) const { return m_V * amp; }
// we use cubic spline interpolation which results in low error ~1e-5 for nice potentials
RVec Potential::ShakenV(double x0) const
{
    return spline.resample_shifted(x0);
}

RVec Potential::operator()(double control) const
{
    switch (m_type) {
    case Type::CONSTANT:
        return m_V;
    case Type::AMPLITUDE:
        return AmplitudeModulatedV(control);
    case Type::SHAKEN:
        return ShakenV(control);
    case Type::CUSTOM:
        return m_Vfn(control);
    default:
        S_FATAL("Unknown potential type");
    };
}

void Potential::initSpline()
{
    if (m_type == Type::SHAKEN) {
        // extend the domain of the spline
        int xSize = m_x.size();

        // RVec tempx(xSize * 3);
        double range = m_x[xSize - 1] - m_x[0] + (m_x[1] - m_x[0]);

        std::vector<double> vectorx;
        RVec tempx = m_x - range;
        vectorx.insert(vectorx.end(), tempx.begin(), tempx.end());
        vectorx.insert(vectorx.end(), m_x.begin(), m_x.end());
        tempx = m_x + range;
        vectorx.insert(vectorx.end(), tempx.begin(), tempx.end());

        std::vector<double> vectorv;
        RVec tempv = RVec::Constant(xSize, m_V[0]);
        vectorv.insert(vectorv.end(), tempv.begin(), tempv.end());
        vectorv.insert(vectorv.end(), m_V.begin(), m_V.end());
        tempv = RVec::Constant(xSize, m_V[xSize - 1]);
        vectorv.insert(vectorv.end(), tempv.begin(), tempv.end());

        spline.set_points(vectorx, vectorv);

        return;
    }
}

// copy constructor
Potential::Potential(const Potential& other)
    : m_V(other.m_V)
    , m_type(other.m_type)
    , m_x(other.m_x)
    , m_Vfn(other.m_Vfn)
{
    initSpline();
}

// move constructor
Potential::Potential(Potential&& other)
    : m_V(other.m_V)
    , m_type(other.m_type)
    , m_x(other.m_x)
    , m_Vfn(other.m_Vfn)
{
    initSpline();
}

// Specific control methods
Potential::Potential(const RVec& x, const RVec& V, Type type)
    : m_V(V)
    , m_type(type)
    , m_x(x)
{
    initSpline();
}

Potential ConstantPotential(HilbertSpace& hs, const RVec& V)
{
    return Potential(hs.x(), V, Potential::Type::CONSTANT);
}
Potential AmplitudePotential(HilbertSpace& hs, const RVec& V)
{
    return Potential(hs.x(), V, Potential::Type::AMPLITUDE);
}
Potential ShakenPotential(HilbertSpace& hs, const RVec& V)
{
    return Potential(hs.x(), V, Potential::Type::SHAKEN);
}