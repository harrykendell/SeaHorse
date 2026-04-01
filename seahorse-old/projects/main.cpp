#include "seahorse.hpp"
INCLUDE_sourceFile;

int main()
{
    SET_RAND_SEED;
    json j;
    j["source"] = std::string((char*)sourceFile);
    Timer timer;
    timer.Start();

    const int dim = pow(2, 9);
    const auto k = sqrt(2);
    const auto xlim = PI / k / 2 * 4;
    constexpr double dt = 0.001;
    constexpr int numSteps = 20e3;
    // constexpr double totalTime = dt * numSteps;

    auto hs = HilbertSpace(dim, xlim);
    const RVec x = hs.x();
    const auto depth = 1500;

    Potential V = ShakenPotential(hs, RVec(depth - 0.5 * depth * (cos(2 * k * x) + 1) * box(x, -PI / k / 2, PI / k / 2)));
    HamiltonianFn H(hs, V);
    Hamiltonian H0 = H(0);

    SplitStepper stepper = SplitStepper(dt, H);

    const RVec gkp0 = (0.6050400769 * H0[0] - 0.3173056838 * H0[2] + 0.3959563443 * H0[4] + 0.160848361 * H0[6] + 0.5265145419 * H0[8] - 0.021791715 * H0[10] - 0.1011218466 * H0[12] - 0.1188482425 * H0[14] + 0.1748682351 * H0[16] - 0.0512410687 * H0[18] - 0.0191934719 * H0[20] + 0.0444604485 * H0[22]).normalized();
    j["gkp0"] = gkp0;
    const RVec gkp1 = (0.250810461 * H0[0] + 0.7666386775 * H0[2] + 0.1641378762 * H0[4] - 0.3886239077 * H0[6] + 0.2182588559 * H0[8] + 0.0526507164 * H0[10] - 0.0419185735 * H0[12] + 0.287147896 * H0[14] + 0.0724890537 * H0[16] + 0.123802967 * H0[18] - 0.0079563714 * H0[20] - 0.1074203873 * H0[22]).normalized();
    j["gkp1"] = gkp1;
    const RVec gkpplus = (gkp0 + gkp1).normalized();
    j["gkpplus"] = gkpplus;
    const RVec gkpminus = (gkp0 - gkp1).normalized();
    j["gkpminus"] = gkpminus;

    Cost cost = StateTransfer(stepper, gkp0, gkp1) + StateTransfer(stepper, gkp1, gkp0)
         + 1e-5 * makeRegularisation() ;//+ 1e2 * makeBoundaries(PI / k / 2);

    Stopper stopper = FidStopper(0.99) + IterStopper(10000) + StallStopper(75);

    const Time t = makeTime(dt, numSteps);

    j["x"] = x;
    j["t"] = t;
    j["V"] = V;
    double bestFid = 0;
    while (bestFid < 0.99) {
        // 4.2 freq is about 0.1MHz at Rb or using harmonic oscillator approx we have sqrt(2*depth)*k as our limit
        Basis basis = Basis::TRIG(t, 4.2 * 5, Basis::Amplitude, 10)
                          // Basis basis = Basis::RESONANT(t, H0.eigenvalues(5), 10)
                          .setMaxAmp(PI / k / 2);

        dCRAB optimiser = dCRAB(basis, stopper, cost);
        optimiser.optimise(10);

        if (optimiser.bestControl.fid > bestFid) {
            bestFid = optimiser.bestControl.fid;

            stepper.reset(gkp0);
            std::vector<CVec> psis = { stepper.state() };
            for (auto phase : optimiser.bestControl.control) {
                stepper.step(phase);
                psis.push_back(stepper.state());
            }
            j["psis"] = psis;

            j["bestControl"] = optimiser.bestControl;
            S_LOG("Best fidelity: ", std::to_string(bestFid), " at iteration ", std::to_string(optimiser.num_iterations), " in ", timer.Elapsed(), "s");
        }
        j["fids"] += optimiser.bestControl.fid;
        save("test.json", j);
    }

    timer.Stop("(Main)");
    save("test.json", j);
    return 0;
}
