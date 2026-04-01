#pragma once
#include <chrono>
#include <string>

// Global timer
class Timer {
private:
    std::chrono::system_clock::time_point start;

public:
    Timer();
    void Start();
    void Stop(std::string msg = "");
    double Elapsed();
};