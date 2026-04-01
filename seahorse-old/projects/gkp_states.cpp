#include "seahorse.hpp"
INCLUDE_sourceFile;

int main()
{
    Timer timer;
    timer.Start();

    const int dim = 1 << 11;
    const auto k = sqrt(2);
    const auto xlim = PI / k / 2 * 4;
    const double dt = 0.001;
    const int numSteps = 3e3;
    const RVec t = RVec::LinSpaced(numSteps, 0, dt * numSteps);

    auto hs = HilbertSpace(dim, xlim);
    const RVec x = hs.x();
    const auto depth = 400;

    RVec V0 = RVec(-0.5 * depth * (cos(2 * k * x) + 1) * box(x, -PI / k / 2, PI / k / 2));
    auto V = ShakenPotential(hs, V0);

    HamiltonianFn H(hs, V);
    Hamiltonian H0 = H(0);

    SplitStepper stepper = SplitStepper(dt, H);

    Cost cost = StateTransfer(stepper, H0[0], H0[1]) + StateTransfer(stepper, H0[1], H0[0]) + 1e3 * makeBoundaries(-1, 1) + 1e-6 * makeRegularisation();

    Basis basis = Basis::TRIG(t, 8.5, Basis::AmpFreq);

    Stopper stopper = FidStopper(0.99) + IterStopper(100) + StallStopper(20);

    SaveFn saver = [](const Optimiser& opt) {
        S_LOG(opt.num_iterations, "\tfid= ", opt.bestControl.fid);
    };

    dCRAB optimiser = dCRAB(basis, stopper, cost, saver);
    optimiser.optimise(5);

    timer.Stop("(GKP)");
    return 0;
}
