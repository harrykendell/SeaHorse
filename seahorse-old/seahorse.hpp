#pragma once

// standard libs
#include <complex>
// #include <format>
#include <iostream>

// Be careful with Vectors.hpp - both declaration and definition to allow templated Eigen returns
#include "include/Optimisation/Basis/Basis.hpp"
#include "include/Optimisation/Cost/Cost.hpp"
#include "include/Optimisation/Optimiser.hpp"
#include "include/Optimisation/Stopper/Stopper.hpp"
#include "include/Optimisation/dCRAB.hpp"
#include "include/Physics/Hamiltonian.hpp"
#include "include/Physics/HilbertSpace.hpp"
#include "include/Physics/Potential.hpp"
#include "include/Physics/Spline.hpp"
#include "include/Physics/SplitStepper.hpp"
#include "include/Physics/Stepper.hpp"
#include "include/Utils/Random.hpp"
#include "include/Utils/Timer.hpp"
#include "src/Json/json.hpp"
#include "src/Physics/Time.hpp"
#include "src/Physics/Vectors.hpp"
#include "src/Utils/Globals.hpp"
#include "src/Utils/Includer.hpp"
#include "src/Utils/Logger.hpp"