#include "include/Optimisation/Basis/Basis.hpp"
#include "include/Utils/Random.hpp"
#include "src/Physics/Vectors.hpp"
#include "src/Utils/Logger.hpp"

Basis::Basis(BasisGenerator basis, int num_coeffs, int num_basis_vectors)
    : m_basis_gen(basis)
    , m_num_coeffs_per_basisFn(num_coeffs)
{
    for (int i = 0; i < num_basis_vectors; i++) {
        m_basis.push_back(m_basis_gen());
    }
    // infer the size of the basis from the first basis function
    m_t_size = m_basis.front()(std::vector<double>(num_coeffs, 0)).size();
}

// Amplitude/Freq/Phase controllable
Basis Basis::TRIG(const RVec& t, double maxFreq, Basis::Controls controls,
    int num_basis_vectors)
{
    static constexpr int freqPower = 5;
    if (controls == Basis::Amplitude) {
        // A function that returns a BasisFn with set phase and frequency
        return Basis([t, maxFreq]() {
          double freq = pow(rands(),freqPower) * maxFreq;
          double phase = rands();
          return [t, freq, phase](std::vector<double> coeffs) {
              return planck_taper(coeffs[0] * cos(2 * PI*(freq * t.array() + phase)));
          }; }, 1, num_basis_vectors);
    } else if (controls == Basis::AmpFreq) {
        // A function that returns a BasisFn with set phase only
        return Basis([t, maxFreq]() {
          double phase = rands();
          return [t, maxFreq, phase](std::vector<double> coeffs) {
              return planck_taper(coeffs[0] * cos(2 * PI * maxFreq * (pow(coeffs[1], freqPower) * t.array() + phase)));
          }; }, 2, num_basis_vectors);
    } else if (controls == Basis::AmpPhase) {
        // A function that returns a BasisFn with set frequency only
        return Basis([t, maxFreq]() {
          double freq = pow(rands(),freqPower) * maxFreq;
          return [t, freq](std::vector<double> coeffs) {
              return planck_taper(coeffs[0] * cos(2 * PI * (freq * t.array() + coeffs[1])));
          }; }, 2, num_basis_vectors);
    } else if (controls == Basis::AmpFreqPhase) {
        // A function that returns a BasisFn with all parameters free
        return Basis([t, maxFreq]() { return [t, maxFreq](std::vector<double> coeffs) {
                                          return planck_taper(coeffs[0] * cos(2 * PI * (maxFreq * pow(coeffs[1], freqPower) * t.array() + coeffs[2])));
                                      }; }, 3, num_basis_vectors);
    } else {
        S_FATAL("Invalid controls");
    }
}
Basis Basis::TRIG(const Time& t, double maxFreq, Basis::Controls controls,
    int num_basis_vectors)
{
    return TRIG(t(), maxFreq, controls, num_basis_vectors);
}

Basis Basis::RESONANT(const RVec& t, RVec freqs, int num_basis_vectors)
{
    // A function that returns a BasisFn with a resonant frequency to a state transfer
    return Basis([t, freqs]() {
      // choose a state transfer at random (E_i-E_j = hbar w_ij) so we then use that as the angular frequency cos(wt+phase)
                  double freq = abs(freqs[(int)(rands()*freqs.size())]-freqs[rands()*freqs.size()]);
                  return [t, freq](std::vector<double> coeffs) {
                      return planck_taper(coeffs[0] * cos(freq * t.array() + 2 * PI * coeffs[1]));
                  }; }, 2, num_basis_vectors);
}

RVec Basis::control(RVec coeffs)
{
    // check that the number of coefficients is correct
    if (coeffs.size() != num_coeffs()) {
        S_FATAL("Invalid number of coefficients, expected ", num_coeffs(), " got ", coeffs.size(), " instead");
    }
    // accumulate the control from each basis function
    RVec control = RVec::Zero(m_t_size);
    for (size_t i = 0; i < m_basis.size(); i++) {
        std::vector<double> coeffs_i(coeffs.data() + 1 + i * m_num_coeffs_per_basisFn, coeffs.data() + 1 + (i + 1) * m_num_coeffs_per_basisFn);
        control += m_basis[i](coeffs_i);
    }
    // scale the control by the amplitude
    double controlMax = control.cwiseAbs().maxCoeff();
    if (controlMax == 0.0) {
        return 0.0 * control;
    }
    double scale = m_maxAmp * coeffs[0] / control.cwiseAbs().maxCoeff();
    return scale * control;
}

int Basis::num_basis_vectors() { return m_basis.size(); }
int Basis::num_coeffs() { return 1 + m_num_coeffs_per_basisFn * m_basis.size(); }

RVec Basis::randomCoeffs() { return RVec::Random(num_coeffs()); }

// chainable setters
Basis& Basis::setMaxAmp(double maxAmp)
{
    this->m_maxAmp = maxAmp;
    return *this;
}

std::unique_ptr<Basis> Basis::generateNewBasis()
{
    return std::make_unique<Basis>(m_basis_gen, m_num_coeffs_per_basisFn, m_basis.size());
}