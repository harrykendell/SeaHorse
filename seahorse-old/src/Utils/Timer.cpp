#include "include/Utils/Timer.hpp"
#include "src/Utils/Logger.hpp"

Timer::Timer()
    : start(std::chrono::system_clock::now()) {};
void Timer::Start() { start = std::chrono::system_clock::now(); };
void Timer::Stop(std::string msg)
{
    S_LOG(Elapsed(), " seconds, ", msg);
};
double Timer::Elapsed()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start).count() / 1e6;
}