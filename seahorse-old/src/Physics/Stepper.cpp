#include "include/Physics/Stepper.hpp"

// Constructor
Stepper::Stepper() { }
Stepper::Stepper(double dt, HamiltonianFn& H)
    : m_dt(dt)
    , m_dx(H.hs.dx())
    , m_psi_f(CVec::Zero(H.hs.dim()))
{
}

std::unique_ptr<Stepper> Stepper::clone() const { return std::unique_ptr<Stepper>(this->clone_impl()); }

CVec Stepper::state() const { return m_psi_f; }

double Stepper::dt() const { return m_dt; }
double Stepper::dx() const { return m_dx; }
