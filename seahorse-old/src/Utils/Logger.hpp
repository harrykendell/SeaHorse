#pragma once
#include "src/Utils/TypeName.hpp"
#include <iostream>
#include <sstream>
#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    using namespace std;
    os << "std::vector<" << TypeName<T>() << ">[";
    copy(v.begin(), v.end() - 1, ostream_iterator<T>(os, ","));
    copy(v.end() - 1, v.end(), ostream_iterator<T>(os, ""));
    os << "]";
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<T>>& v)
{
    using namespace std;

    // NOTE: for some reason std::copy doesn't work here, so I use manual loop
    // copy(v.begin(), v.end(), ostream_iterator<std::vector<T>>(os, "\n"));

    for (size_t i = 0; i < v.size(); ++i)
        os << v[i] << "\n";
    return os;
}

#define S_LOG(...) S_LOG_IMPL(__VA_ARGS__);

#define S_ERROR(...) \
    S_ERR_IMPL("\033[1;91m[ERROR]\033[1;0m", __LINE__, __FILE__, __VA_ARGS__);

#define S_FATAL(...)                                                      \
    {                                                                     \
        S_ERR_IMPL("\033[1;91m[FATAL]", __LINE__, __FILE__, __VA_ARGS__); \
        exit(EXIT_FAILURE);                                               \
    };

template <typename... Args>
void S_ERR_IMPL(const char* type, int line, const char* fileName,
    Args&&... args)
{
    std::ostringstream stream;
    std::cout << type << " " << fileName << ":" << line << " -> ";
    (std::cout << ... << std::forward<Args>(args));
    std::cout << std::endl;
}

template <typename... Args>
void S_LOG_IMPL(Args&&... args)
{
    std::ostringstream stream;
    std::cout << "\033[1;92m[INFO] \033[1;0m";
    (std::cout << ... << std::forward<Args>(args));
    std::cout << std::endl;
}