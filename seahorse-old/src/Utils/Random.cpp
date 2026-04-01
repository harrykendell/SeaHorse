#include "include/Utils/Random.hpp"

uint64_t get_rand_seed()
{
    auto since_epoch = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto since_epoch_nano = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch);
    uint64_t rand_seed = since_epoch_nano.count();
    return rand_seed;
}

randGen::randGen(double start, double stop)
    : dist(std::uniform_real_distribution<double>(start, stop))
{
    gen = std::mt19937_64(get_rand_seed());
}

randGen::randGen()
    : randGen(0.0, 1.0)
{
}

double randGen::operator()() { return dist(gen); }

randGen rands = randGen(0.0, 1.0);
