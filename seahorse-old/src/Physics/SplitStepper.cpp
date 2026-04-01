#include "include/Physics/SplitStepper.hpp"
#include "src/Physics/Vectors.hpp"

// Constructor
SplitStepper::SplitStepper() { }

SplitStepper::SplitStepper(double dt, HamiltonianFn& H, bool use_imag_pot)
    : Stepper(dt, H)
    , m_V(H.V)
{
    m_fft.fwd(m_mombuf, m_psi_f);
    m_T_exp_2 = (-0.5i * dt * H.T_p.array()).exp();
    // m_T_exp = m_T_exp_2.array().square(); uses e^2a = (e^a)^2 to avoid exp again
    m_T_exp = (-1.0i * dt * H.T_p.array()).exp();

    // Absorb wavefunction in the outer 1/8ths of the domain
    if (use_imag_pot) {
        RVec imagPotstrength = 100 * (1 - planck_taper(RVec::Ones(H.hs.dim()), 1.0 / 8.0));
        imagPot = (-m_dt * imagPotstrength.array()).exp();
    } else {
        imagPot = RVec::Ones(H.hs.dim());
    }
}

void SplitStepper::reset(const CVec& psi_0)
{
    m_psi_f = psi_0.normalized();
}

void SplitStepper::step(double u) // Move forward a single step
{
    m_fft.fwd(m_mombuf, m_psi_f);
    m_mombuf = m_mombuf.array().cwiseProduct(m_T_exp_2.array());
    m_fft.inv(m_psi_f, m_mombuf);
    m_psi_f = m_psi_f.array()
                  .cwiseProduct((-1.0i * m_dt * m_V(u).array()).exp())
                  .cwiseProduct(imagPot.array());
    m_fft.fwd(m_mombuf, m_psi_f);
    m_mombuf = m_mombuf.array().cwiseProduct(m_T_exp_2.array());
    m_fft.inv(m_psi_f, m_mombuf);
}

// Optimised steps but can't provide intermediate step wavefunctions
// This combines T/2 ifft fft T/2 between steps to save computation.
void SplitStepper::evolve(const CVec& psi_0, const RVec& control)
{
    reset(psi_0);

    // Timer timer;
    // Initial half step T/2
    m_fft.fwd(m_mombuf, m_psi_f);
    m_mombuf = m_mombuf.array()
                   .cwiseProduct(m_T_exp_2.array());

    // Main loop V,T full steps
    for (int i = 0; i < control.size() - 1; i++) {
        m_fft.inv(m_psi_f, m_mombuf);
        m_psi_f = m_psi_f.array()
                      .cwiseProduct((-1.0i * m_dt * m_V(control[i]).array()).exp())
                      .cwiseProduct(imagPot.array());
        m_fft.fwd(m_mombuf, m_psi_f);
        m_mombuf = m_mombuf.array()
                       .cwiseProduct(m_T_exp.array());
    }

    // Finishing out the last V,T/2
    m_fft.inv(m_psi_f, m_mombuf);
    m_psi_f = m_psi_f.array()
                  .cwiseProduct((-1.0i * m_dt * m_V(control[control.size() - 1]).array()).exp())
                  .cwiseProduct(imagPot.array());
    m_fft.fwd(m_mombuf, m_psi_f);
    m_mombuf = m_mombuf.array()
                   .cwiseProduct(m_T_exp_2.array());
    m_fft.inv(m_psi_f, m_mombuf);

    //S_LOG("Stepped ", control.size(), " times in ", timer.Elapsed(), " seconds");
}