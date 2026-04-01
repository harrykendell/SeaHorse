// #include "seahorse.hpp"
#include "src/Json/json.hpp"
#include "src/Utils/Logger.hpp"
#include <iostream>
#include <string>

int main()
{

    json j;
    j["a"] = 1;
    j["b"] = 2;
    j["c"] = 3;
    save("test.json", j);

    auto j2 = load("test.json");
    S_LOG("dc2: ", j2.dump(4));
    return 0;
}