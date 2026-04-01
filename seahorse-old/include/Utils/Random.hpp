#pragma once

#include <chrono>
#include <random>

#define SET_RAND_SEED srand(get_rand_seed());

uint64_t get_rand_seed();

struct randGen {
    // construct a mersenne twister random generator with an associated uniform distribution to draw from [0-1)
    std::mt19937_64 gen;
    std::uniform_real_distribution<double> dist;

    randGen(double start, double stop);

    randGen();

    double operator()();
};

// global random generator [0-1]
extern randGen rands;