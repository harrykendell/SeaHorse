#include "seahorse.hpp"

#include "gui/GuiComponents.cpp"
#include "libs/raylib/src/raylib.h"

float fontSize = 15;
float padding = 8;
float itemSize = 24;

struct SettingsState {

    // positions of windows
    Rectangle rect_performance = { -1, 0, 142, 131 };
    Rectangle rect_settings = { 140, 0, 211, 131 };
    Rectangle rect_spectrum = { -1, 130, 352, 501 };
    Rectangle rect_evolution = { 350, 130, 651, 501 };

    // We MUST return an RVec not an Eigen::Expression or the memory is freed and we get a segfault in some cases
    std::function<RVec(double)> V0;

    SettingsState()
    {
        HilbertSpace hs = HilbertSpace(dim, xlim);
        const RVec x = hs.x();
        V0 = [=, this](double phase) {
            return RVec(depth -0.5 * depth * (cos(2 * k * (x - phase)) + 1) * box(x - phase, -PI / k / 2, PI / k / 2));
        };

        HamiltonianFn H(hs, V0);
        Hamiltonian H0 = H(0);
        psi_t = H0[2];
    }

    // spectrum variables
    graph plot_spectrum = graph({ rect_spectrum.x, rect_spectrum.y + 23, rect_spectrum.width, rect_spectrum.height - 23 }, "Position (a.u.)", "Energy (E_r)");
    bool spectrum_legend = true; // show legend?
    void spectrumPlot()
    {
        auto hs = HilbertSpace(dim, xlim);
        const RVec x = hs.x();

        HamiltonianFn H(hs, V0);
        Hamiltonian H0 = H(0);
        H0.calcSpectrum(15, 0);

        plot_spectrum.clearLines();
        plot_spectrum.plot(x, H0.V, "V (0)", BLACK, 1, 4, 0);
        auto scaling = ((H0.eigenvalue(1) - H0.eigenvalue(0)) / 2) / (H0.spectrum.eigenvectors.cwiseAbs().maxCoeff());
        for (auto i = 0; i < H0.spectrum.numEigvs; i++) {
            if (H0.eigenvalue(i) > depth) {
                break;
            } // only plot bound states
            plot_spectrum.plot(x, H0[i] * scaling + H0.eigenvalue(i), TextFormat("H [%d]", i));
        }
        plot_spectrum.setLims();
    }

    // performance variables
    PerfStats perfStats = PerfStats(); // gets/holds performance stats

    // settings variables
    int dim = 1 << 11;
    const int k = sqrt(2);
    double xlim = PI / k / 2 * 4;

    double dt = 0.002;
    int num_steps = 1.5e3;

    int depth = 400;

    std::string psi_0 = "H[0];H[1];H[2];H[0]+H[1]";
    int psi_0_active = 0;
    bool psi_0_edit = false;

    // CVec psi_0;
    CVec psi_t;
};

struct EvolutionState {
public:
    SettingsState* state;
    SplitStepper stepper;
    RVec control;
    RVec t;
    RVec x;

    int currentStep = 0;
    float progress = 0;

    int numSteps = 1;
    bool numStepsEdit = false;
    bool evolve = false;

    std::string fid = "Fid: NaN";

    std::string controls = "ZERO;SIN;COS;RAMP";
    std::vector<RVec> controls_vec;
    int scroll = 0;
    int scroll_selected = 0;
    int scroll_old_selected = 0;

    graph plotSpace;
    std::shared_ptr<graphLine> psireal;
    std::shared_ptr<graphLine> psiimag;
    std::shared_ptr<graphLine> psiabs;
    std::shared_ptr<graphLine> vline;
    graph plotControl;

    EvolutionState(SettingsState* state)
        : state(state)
    {
        auto hs = HilbertSpace(state->dim, state->xlim);
        x = hs.x();
        t = RVec::LinSpaced(state->num_steps, 0, state->num_steps * state->dt);

        controls_vec = { t * 0, planck_taper(sin(t).eval()), planck_taper(cos(t)), planck_taper(RVec::LinSpaced(t.size(), 0, 10)) };

        HamiltonianFn H(hs, state->V0);
        Hamiltonian H0 = H(0);

        CVec psi_0 = state->psi_0_active == 0 ? H0[0] : state->psi_0_active == 1 ? H0[1]
            : state->psi_0_active == 2                                           ? H0[2]
                                                                                 : H0[0] + H0[1];

        plotSpace = graph({ state->rect_evolution.x + 0, state->rect_evolution.y + 24, 440, 240 });
        plotControl = graph({ state->rect_evolution.x + 0, state->rect_evolution.y + 304, 312, 128 });
        stepper = SplitStepper(state->dt, H);
        control = RVec::Zero(state->num_steps);

        psireal = plotSpace.plot(x, stepper.state().real(), "Real", RED, 0.5);
        psiimag = plotSpace.plot(x, stepper.state().imag(), "Imag", BLUE, 0.5);
        psiabs = plotSpace.plot(x, stepper.state().cwiseAbs(), "Abs2", VIOLET);
        vline = plotSpace.plot(x, state->V0(control[currentStep]), "V", BLACK, 1, 4, 0);
        vline->ignoreGraphLims();
        plotControl.plot(t, control, "", BLACK);
    }

    void optimise()
    {
        auto hs = HilbertSpace(state->dim, state->xlim);
        x = hs.x();
        HamiltonianFn H(hs, state->V0);
        Hamiltonian H0 = H(0);

        CVec psi_0 = state->psi_0_active == 0 ? H0[0]
            : state->psi_0_active == 1        ? H0[1]
            : state->psi_0_active == 2        ? H0[2]
                                              : H0[0] + H0[1];

        SplitStepper stepper = SplitStepper(state->dt, H);
        Basis basis = Basis::TRIG(t, 5.0, Basis::AmpFreqPhase,20)
                          .setMaxAmp(PI / state->k / 2);
        Stopper stopper = FidStopper(0.99) + IterStopper(100) + StallStopper(20);
        Cost cost = StateTransfer(stepper, H0[0], H0[1]);

        dCRAB optimiser = dCRAB(basis, stopper, cost);
        optimiser.optimise(5);

        controls_vec.push_back(optimiser.bestControl.control);
        controls += TextFormat(";dCRAB: %.2f", optimiser.bestControl.fid);
    }

    void reset()
    {
        currentStep = 0;
        progress = 0;

        auto hs = HilbertSpace(state->dim, state->xlim);

        HamiltonianFn H(hs, state->V0);
        Hamiltonian H0 = H(control[0]);
        CVec psi_0 = state->psi_0_active == 0 ? H0[0] : state->psi_0_active == 1 ? H0[1]
            : state->psi_0_active == 2                                           ? H0[2]
                                                                                 : H0[0] + H0[1];

        stepper.reset(psi_0);
        psireal->updateYData(stepper.state().real());
        psiimag->updateYData(stepper.state().imag());
        psiabs->updateYData(stepper.state().cwiseAbs());
        vline->updateYData(state->V0(control[0]));
    }
    void setControl()
    {
        control = controls_vec[scroll_selected];
        plotControl.clearLines();
        plotControl.plot(t, control, "", BLACK);
        reset();

        auto hs = HilbertSpace(state->dim, state->xlim);

        HamiltonianFn H(hs, state->V0);
        Hamiltonian H0 = H(control[0]);
        CVec psi_0 = state->psi_0_active == 0 ? H0[0] : state->psi_0_active == 1 ? H0[1]
            : state->psi_0_active == 2                                           ? H0[2]
                                                                                 : H0[0] + H0[1];

        stepper.evolve(psi_0, control);
        fid = TextFormat("Fid: %.2f", fidelity(state->psi_t, stepper.state()));
    }
    void step()
    {
        if (currentStep + numSteps < t.size()) {
            for (auto i = 0; i < numSteps; i++) {
                stepper.step(control[currentStep]);
                currentStep++;
            }
            psireal->updateYData(stepper.state().real());
            psiimag->updateYData(stepper.state().imag());
            psiabs->updateYData(stepper.state().cwiseAbs());
            vline->updateYData(state->V0(control[currentStep]));
            progress = (float)currentStep / (float)t.size();
        } else {
            reset();
        }
    }
    void update()
    {
        // If we reselect the current one, don't do anything
        if (scroll_selected == -1) {
            scroll_selected = scroll_old_selected;
        }
        if (scroll_selected != scroll_old_selected) {
            setControl();
            scroll_old_selected = scroll_selected;
        }
        if (evolve) {
            step();
        }
    }
};

int main()
{
    // RAYGUI Initialization
    //---------------------------------------------------------------------------------------

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT); //| FLAG_WINDOW_TOPMOST);
    InitWindow(1000, 630, "Seahorse");
    SetExitKey(KEY_NULL);
    SetTargetFPS(30);
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    // EnableEventWaiting();

    Font fontTtf = LoadFontFromMemory(".ttf", UbuntuMonoBold, (sizeof(UbuntuMonoBold) / sizeof(*UbuntuMonoBold)), fontSize, 0, 0);
    GuiSetStyle(DEFAULT, LINE_COLOR, 255); // Black lines
    GuiSetFont(fontTtf);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);

    //--------------------------------------------------------------------------------------
    // MY Initialization
    //---------------------------------------------------------------------------------------
    auto settings = SettingsState {};
    auto evolution = EvolutionState(&settings);
    settings.spectrumPlot();
    //--------------------------------------------------------------------------------------
    // GUI Loop
    //---------------------------------------------------------------------------------------
    while (!WindowShouldClose()) // Detect window close button
    {
        // Update
        //----------------------------------------------------------------------------------
        settings.perfStats.update();
        evolution.update();
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // Performance window
        GuiPanel(settings.rect_performance, "Stats");
        DrawTextEx(fontTtf, "FPS:", Vector2 { settings.rect_performance.x + padding, settings.rect_performance.y + padding + itemSize }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, "THR:", Vector2 { settings.rect_performance.x + padding, settings.rect_performance.y + padding + itemSize * 2 }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, "CPU:", Vector2 { settings.rect_performance.x + padding, settings.rect_performance.y + padding + itemSize * 3 }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, "RAM:", Vector2 { settings.rect_performance.x + padding, settings.rect_performance.y + padding + itemSize * 4 }, fontSize, 0, DARKGRAY);

        DrawTextEx(fontTtf, TextFormat(" %d", settings.perfStats.fps), Vector2 { settings.rect_performance.x + itemSize * 2, settings.rect_performance.y + padding + itemSize }, fontSize, 0, settings.perfStats.fps_c);
        DrawTextEx(fontTtf, TextFormat(" %d/%d", settings.perfStats.thr, settings.perfStats.totalthreads), Vector2 { settings.rect_performance.x + itemSize * 2, settings.rect_performance.y + padding + itemSize * 2 }, fontSize, 0, settings.perfStats.thr_c);
        DrawTextEx(fontTtf, TextFormat(" %.2f %%", settings.perfStats.cpu), Vector2 { settings.rect_performance.x + itemSize * 2, settings.rect_performance.y + padding + itemSize * 3 }, fontSize, 0, settings.perfStats.cpu_c);
        DrawTextEx(fontTtf, TextFormat(" %.2f GB", settings.perfStats.ram), Vector2 { settings.rect_performance.x + itemSize * 2, settings.rect_performance.y + padding + itemSize * 4 }, fontSize, 0, settings.perfStats.ram_c);

        // Spectrum window
        GuiPanel(settings.rect_spectrum, "Eigenspectrum");
        settings.plot_spectrum.draw();

        // Settings window
        GuiPanel(settings.rect_settings, "Settings");

        DrawTextEx(fontTtf, "Dim:", Vector2 { settings.rect_settings.x + padding, settings.rect_settings.y + padding + itemSize }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, TextFormat(" %d", settings.dim), Vector2 { settings.rect_settings.x + itemSize * 2, settings.rect_settings.y + padding + itemSize }, fontSize, 0, BLACK);
        DrawTextEx(fontTtf, "XLim:", Vector2 { settings.rect_settings.x + itemSize * (float)4.5, settings.rect_settings.y + padding + itemSize }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, TextFormat(" %.3f", settings.xlim), Vector2 { settings.rect_settings.x + itemSize * (float)6.5, settings.rect_settings.y + padding + itemSize }, fontSize, 0, BLACK);

        DrawTextEx(fontTtf, "dT:", Vector2 { settings.rect_settings.x + padding, settings.rect_settings.y + padding + itemSize * 2 }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, TextFormat(" %.3f", settings.dt), Vector2 { settings.rect_settings.x + itemSize * 2, settings.rect_settings.y + padding + itemSize * 2 }, fontSize, 0, BLACK);
        DrawTextEx(fontTtf, "Steps:", Vector2 { settings.rect_settings.x + itemSize * (float)4.5, settings.rect_settings.y + padding + itemSize * 2 }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, TextFormat(" %d", settings.num_steps), Vector2 { settings.rect_settings.x + itemSize * (float)6.5, settings.rect_settings.y + padding + itemSize * 2 }, fontSize, 0, BLACK);

        DrawTextEx(fontTtf, "Depth:", Vector2 { settings.rect_settings.x + padding, settings.rect_settings.y + padding + itemSize * 3 }, fontSize, 0, DARKGRAY);
        DrawTextEx(fontTtf, TextFormat(" %d", settings.depth), Vector2 { settings.rect_settings.x + itemSize * 2, settings.rect_settings.y + padding + itemSize * 3 }, fontSize, 0, BLACK);

        DrawTextEx(fontTtf, "Psi_0:", Vector2 { settings.rect_settings.x + padding, settings.rect_settings.y + padding + itemSize * 4 }, fontSize, 0, DARKGRAY);
        if (GuiDropdownBox(Rectangle { settings.rect_settings.x + padding + itemSize * 2, settings.rect_settings.y + padding + itemSize * 4 - 6, 86, 24 }, settings.psi_0.c_str(), &settings.psi_0_active, settings.psi_0_edit))
            settings.psi_0_edit = !settings.psi_0_edit;

        // Evolution window
        GuiPanel(settings.rect_evolution, "Evolution");
        evolution.plotSpace.draw();
        evolution.plotControl.draw();

        if (GuiSpinner(Rectangle { settings.rect_evolution.x + padding, settings.rect_evolution.y + 272, 88, 24 }, NULL, &evolution.numSteps, 0, 1000, evolution.numStepsEdit))
            evolution.numStepsEdit = !evolution.numStepsEdit;
        GuiCheckBox(Rectangle { settings.rect_evolution.x + 88 + padding * 2, settings.rect_evolution.y + 272, 24, 24 }, "Evolve", &evolution.evolve);

        if (GuiButton(Rectangle { settings.rect_evolution.x + 200, settings.rect_evolution.y + 272, 56, 24 }, "Reset"))
            evolution.reset();

        if (GuiButton(Rectangle { settings.rect_evolution.x + 270,
                          settings.rect_evolution.y + 272, 56, 24 },
                "dCRAB"))
            evolution.optimise();

        GuiLabel(Rectangle { settings.rect_evolution.x + 340, settings.rect_evolution.y + 272, 112, 24 }, evolution.fid.c_str());

        GuiProgressBar(Rectangle { settings.rect_evolution.x + 8, settings.rect_evolution.y + 440, 304, 12 }, NULL, NULL, &evolution.progress, 0, 1);

        GuiListView(Rectangle { settings.rect_evolution.x + 320, settings.rect_evolution.y + 304, 112, 152 }, evolution.controls.c_str(), &evolution.scroll, &evolution.scroll_selected);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}